#!/usr/bin/env python3
"""G60: Case C's relative shape at the base station (DC-3).

usage: case_c_gate.py <dir written by scripts/case_c_gate.sh>

Runs read:
  on/   --caseCRelative: case_c_ranges_* (one row per peer range, with when it reached the
        base station or -1), case_c_relative_* (one row per connected set the base station
        assembled), trace_* and subjtrace_* (each node's tier and anchor count per cycle),
        responses_* (what the base station's gateways were answered); on.log, the DC-3 line.
  off/  the same without the flag (the control).
  r_nosess   the refusal: the flag without --sessionOverMesh.

Checks (each prints "G60 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     every run the checks read exited 0.
  ZERO     every peer range was taken on a cycle its node had no anchor (nAnchors 0 in its
           own trace), on the tier it ranged on that cycle (sect 6), to another subject.
  DELIVER  a set reached the base station only when a gateway was answered with an entry of
           that node stamped after the cycle, within 10 s, no earlier than the answer; and a
           set dropped had no such answer inside its window.
  SHAPE    the base station's sets are recomputed from the delivered ranges (members,
           measured and completed pairs); a pair's cohesion is its mean range; a 3+ shape is
           centred and reproduces its (completed) distance matrix with the stated stress.
  NOFUSE   control flow: the DC-3 line says nothing was fused into a filter.
  REACH    rule 4: pairs and 3+ shapes both assembled. The shapes' misfit to truth is
           reported, not judged (868 MHz peer ranges are coarse; TECH_DEBT N.13).
  COUNTS   the DC-3 line recounted from the files.
  CONTROL  without the flag: no case_c_ files and no DC-3 line.
  R        the refusal exits 1 with the reason.
"""
import csv
import glob
import math
import os
import re
import sys

TIERN = {"868MHz": "0", "2.4GHz": "1", "UWB": "2"}
WIN = 10.0
LINE = re.compile(r"DC-3 Case C relative \((\S+)\): 0-anchor node-cycles (\d+) \(no peer heard (\d+)\); peer "
                  r"exchanges (\d+), ranged (\d+) \(carried on entries, ([\d.]+) s billed\); reached the base station "
                  r"(\d+), dropped (\d+), still pending (\d+); shapes: pairs (\d+), 3\+ (\d+) \(completed pairs (\d+)\); "
                  r"fused into a filter (\d+)")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G60 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G60 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G60 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def read(p):
    try:
        return open(p).read()
    except OSError:
        return None


def misfit(off, tru):
    k = len(off)
    mx = sum(p[0] for p in tru) / k
    my = sum(p[1] for p in tru) / k
    best = 1e300
    for refl in (0, 1):
        h = [0.0, 0.0, 0.0, 0.0]
        for (sx, sy), (tx, ty) in zip(off, tru):
            sy = -sy if refl else sy
            h[0] += sx * (tx - mx); h[1] += sx * (ty - my); h[2] += sy * (tx - mx); h[3] += sy * (ty - my)
        th = math.atan2(h[1] - h[2], h[0] + h[3])
        e = 0.0
        for (sx, sy), (tx, ty) in zip(off, tru):
            sy = -sy if refl else sy
            rx = math.cos(th) * sx - math.sin(th) * sy
            ry = math.sin(th) * sx + math.cos(th) * sy
            e += (rx - (tx - mx)) ** 2 + (ry - (ty - my)) ** 2
        best = min(best, math.sqrt(e / k))
    return best


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "every run the checks read exited 0" if not bad else f"non-zero exit: {bad}")
    on = os.path.join(d, "on")
    rp = glob.glob(os.path.join(on, "case_c_ranges_*_seed*.csv"))
    sp = glob.glob(os.path.join(on, "case_c_relative_*_seed*.csv"))
    if not rp or not sp:
        missing("ZERO", "no case_c_ files in on/")
        return 1
    ranges = list(csv.DictReader(open(rp[0])))
    shapes = list(csv.DictReader(open(sp[0])))

    # per (t, node): tier and anchor count
    st = {}
    for f in glob.glob(os.path.join(on, "trace_*_seed*.csv")):
        for r in csv.DictReader(open(f)):
            st[(r["t"], "1")] = (TIERN.get(r["activeTier"]), int(r["nAnchors"]))
    for f in glob.glob(os.path.join(on, "subjtrace_*_seed*_subj*.csv")):
        for r in csv.DictReader(open(f)):
            st[(r["t"], r["subjectId"])] = (TIERN.get(r["activeTier"]), int(r["nAnchors"]))
    subjects = {"1", "2", "3", "4", "5"}
    zbad = []
    for r in ranges:
        s = st.get((r["t"], r["nodeId"]))
        if s is None or s[1] != 0 or s[0] != r["tier"] or r["peerId"] not in subjects or r["peerId"] == r["nodeId"]:
            zbad.append((r["t"], r["nodeId"], r["peerId"], r["tier"], s))
    report(not zbad and ranges, "ZERO",
           f"{len(ranges)} peer ranges, each on a 0-anchor cycle of its node, on its tier, to another subject; bad "
           f"{len(zbad)} (need 0)" + (f": {zbad[:3]}" if zbad else ""))

    # DELIVER against the base station's answers
    resp = []
    for f in glob.glob(os.path.join(on, "responses_*_seed*.csv")):
        for r in csv.DictReader(open(f)):
            if 101 <= int(r["requesterId"]) <= 199:
                resp.append((float(r["t"]), r["entryNode"], float(r["entryT"])))
    sets = {}
    for r in ranges:
        sets.setdefault((r["t"], r["nodeId"]), set()).add(r["deliveredAt"])
    dbad, ndel, ndrop = [], 0, 0
    for (t0, node), da in sets.items():
        if len(da) != 1:
            dbad.append((t0, node, "rows of one set disagree"))
            continue
        da = float(next(iter(da)))
        t0f = float(t0)
        q = [rt for (rt, n, et) in resp if n == node and et > t0f + 1e-6]
        if da >= 0:
            ndel += 1
            if not (t0f < da <= t0f + WIN + 1e-6) or not any(rt <= da + 1e-6 for rt in q):
                dbad.append((t0, node, "delivered", da))
        else:
            ndrop += 1
            if any(rt < t0f + WIN - 1e-6 for rt in q):
                dbad.append((t0, node, "dropped though answered", min(q)))
    report(not dbad and ndel > 0, "DELIVER",
           f"sets reached the base station {ndel}, dropped {ndrop}; each against the gateways' answers; bad {len(dbad)} "
           f"(need 0)" + (f": {dbad[:3]}" if dbad else ""))

    # SHAPE: recompute from delivered ranges
    by_t = {}
    for r in ranges:
        if float(r["deliveredAt"]) >= 0:
            by_t.setdefault(r["t"], []).append(r)
    sbad, mis = [], []
    got = {}
    for s in shapes:
        got.setdefault(s["t"], []).append(s)
    for t0, rs in by_t.items():
        pr = {}
        for r in rs:
            u, v = sorted((int(r["nodeId"]), int(r["peerId"])))
            pr.setdefault((u, v), []).append(float(r["rangeM"]))
        root = {}

        def find(x):
            while root[x] != x:
                root[x] = root[root[x]]
                x = root[x]
            return x
        for (u, v) in pr:
            root.setdefault(u, u); root.setdefault(v, v)
            root[find(u)] = find(v)
        comp = {}
        for x in root:
            comp.setdefault(find(x), []).append(x)
        want = {}
        for mem in comp.values():
            mem = sorted(mem)
            want[";".join(map(str, mem))] = mem
        rows = {s["members"]: s for s in got.get(t0, [])}
        if set(rows) != set(want):
            sbad.append((t0, "sets", sorted(rows), sorted(want)))
            continue
        for key, mem in want.items():
            s = rows[key]
            k = len(mem)
            inf = float("inf")
            dm = [[0.0 if i == j else inf for j in range(k)] for i in range(k)]
            meas = 0
            for i in range(k):
                for j in range(i + 1, k):
                    p = pr.get((mem[i], mem[j]))
                    if p:
                        dm[i][j] = dm[j][i] = sum(p) / len(p)
                        meas += 1
            comp_n = sum(1 for i in range(k) for j in range(i + 1, k) if dm[i][j] == inf)
            for m in range(k):
                for i in range(k):
                    for j in range(k):
                        dm[i][j] = min(dm[i][j], dm[i][m] + dm[m][j])
            if int(s["k"]) != k or int(s["measuredPairs"]) != meas or int(s["completedPairs"]) != comp_n:
                sbad.append((t0, key, "counts", s["k"], s["measuredPairs"], s["completedPairs"], k, meas, comp_n))
                continue
            if k == 2:
                if abs(float(s["cohesionM"]) - dm[0][1]) > 1e-3:
                    sbad.append((t0, key, "pair cohesion", s["cohesionM"], round(dm[0][1], 6)))
                continue
            off = [tuple(map(float, p.split(":"))) for p in s["offsets"].split(";")]
            cx = sum(p[0] for p in off) / k
            cy = sum(p[1] for p in off) / k
            ss, n = 0.0, 0
            coh = 0.0
            for i in range(k):
                for j in range(i + 1, k):
                    dd = math.hypot(off[i][0] - off[j][0], off[i][1] - off[j][1])
                    coh = max(coh, dd)
                    ss += (dd - dm[i][j]) ** 2
                    n += 1
            stress = math.sqrt(ss / n)
            tol = 0.01 + 0.002 * max(1.0, coh)  # offsets are printed to 1 mm
            if abs(cx) > 0.01 * k or abs(cy) > 0.01 * k or abs(stress - float(s["stressM"])) > tol or \
                    abs(coh - float(s["cohesionM"])) > tol:
                sbad.append((t0, key, "shape", round(stress, 3), s["stressM"], round(coh, 3), s["cohesionM"]))
            tru = [tuple(map(float, p.split(":"))) for p in s["truth"].split(";")]
            mis.append(misfit(off, tru))
    extra = [t for t in got if t not in by_t]
    if extra:
        sbad.append(("shapes without delivered ranges", extra[:3]))
    report(not sbad and shapes, "SHAPE",
           f"{len(shapes)} sets recomputed from the delivered ranges; bad {len(sbad)} (need 0)"
           + (f": {sbad[:2]}" if sbad else ""))

    log = read(os.path.join(d, "on.log")) or ""
    m = LINE.search(log)
    if not m:
        missing("NOFUSE", "no DC-3 line in on.log")
        missing("COUNTS", "no DC-3 line in on.log")
    else:
        report(m.group(13) == "0", "NOFUSE", f"fused into a filter {m.group(13)} (need 0)")
        pairs = sum(1 for s in shapes if s["k"] == "2")
        big = [s for s in shapes if int(s["k"]) >= 3]
        comp_sum = sum(int(s["completedPairs"]) for s in big)
        want = (int(m.group(7)), int(m.group(8)), int(m.group(10)), int(m.group(11)), int(m.group(12)))
        have = (ndel, ndrop, pairs, len(big), comp_sum)
        ok = want == have and int(m.group(5)) >= len(ranges) and int(m.group(4)) >= int(m.group(5))
        report(ok, "COUNTS", f"DC-3 line (delivered, dropped, pairs, 3+, completed) {want}; recounted {have}; "
               f"ranged {m.group(5)} >= rows {len(ranges)}")
    big = sum(1 for s in shapes if int(s["k"]) >= 3)
    pairs = sum(1 for s in shapes if s["k"] == "2")
    med = sorted(mis)[len(mis) // 2] if mis else -1
    report(big > 0 and pairs > 0, "REACH", f"pairs {pairs}, 3+ shapes {big} (need > 0 each); misfit to truth, "
           f"reported not judged: median {med:.1f} m over {len(mis)}")

    offlog = read(os.path.join(d, "off.log"))
    if offlog is None:
        missing("CONTROL", "no off.log")
    else:
        files = glob.glob(os.path.join(d, "off", "case_c_*"))
        report(not files and "DC-3 Case C" not in offlog, "CONTROL",
               f"without the flag: case_c_ files {len(files)}, DC-3 lines {offlog.count('DC-3 Case C')} (need 0)")
    txt = read(os.path.join(d, "r_nosess.log")) or ""
    report(rc(d, "r_nosess") == 1 and "--caseCRelative needs" in txt, "R",
           f"r_nosess: exit {rc(d, 'r_nosess')} (need 1), reason {'present' if '--caseCRelative needs' in txt else 'absent'}")
    print(f"  G60: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
