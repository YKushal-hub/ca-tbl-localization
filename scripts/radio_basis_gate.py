#!/usr/bin/env python3
"""G61: the LR1121 radio basis (docs/SPEC_CONFORMANCE_PLAN.md SY-2, --radioBasis).

usage: radio_basis_gate.py <dir written by scripts/radio_basis_gate.sh>

Runs read (each pair differs only in --radioBasis):
  sx/, lr/          the default configuration, all six arms, seed 7, 900 s, traces on.
  sub_sx/, sub_lr/  five measured subjects (full arm, 600 s): subject_summary.csv.
  gos_sx/, gos_lr/  G16's grant-gossip regime: the gateways' gossip energy lines.
  pre/              --specProfile: spec_profile.txt and the SY-2 line.
  r_bad             refusal: an unknown basis.

What a firing looks like on disk. The basis changes currents and nothing else: every timing
field is kept from the SX profiles, and at defaults energy feeds no decision. So between a
pair every non-energy column is identical, and each energy bucket of each tier is the SX
bucket times the ratio of the two currents (charge = I x V x t over the same times). A
basis applied to the wrong tier, to some accounts only, or with a moved time shows as a
ratio off the current ratio, or as a non-energy column moving.

Checks (each prints "G61 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     every run the checks read exited 0.
  SAME     sx vs lr: arm_summary.csv and every trace_*.csv identical outside the energy
           columns (totalJ, j_*, accuracyPerJoule, tx_/rx_/cad_/idle_/sleep_*; cumJ).
  RATIO    every arm's tx/rx/cad/idle/sleep bucket on 868 MHz and 2.4 GHz is the SX one times
           the current ratio (to the CSV's 9 decimals), UWB unchanged, and j_<tier> and totalJ
           are the sums; every ratio is exercised (a bucket > 0) on at least one arm.
  SUBJ     the same ratios on every subject row of subject_summary.csv, for the buckets it
           carries (tx and sleep), with j_uwb unchanged and totalJ the tier sum (subjects 2..5
           carry the basis too); non-energy columns identical.
  GOSSIP   every gateway's gossip energy line: tx and rx the SX values times 28/45 and
           7.8/5.3 (to the 6 decimals printed), with gossip sent > 0 somewhere.
  LINE     the SY-2 line is printed under lr1121 with the profile's currents, and never
           under sx.
  PRESET   the preset sets --radioBasis=sx (spec_profile.txt; DESIGN_DECISIONS 11) and prints no SY-2 line.
  R        an unknown basis exits 1 with the reason.
"""
import csv
import glob
import os
import re
import sys

nok = nfail = nmiss = 0
# (sx current, lr1121 current) per (tier, bucket): ca-tbl-energy-model.cc
CUR = {
    ("868", "tx"): (45.0, 28.0), ("868", "rx"): (5.3, 7.8), ("868", "cad"): (5.3, 7.8),
    ("868", "idle"): (1.5, 1.25), ("868", "sleep"): (0.0012, 0.0016),
    ("2p4", "tx"): (24.0, 26.0), ("2p4", "rx"): (7.0, 7.5), ("2p4", "cad"): (7.0, 7.5),
    ("2p4", "idle"): (1.4, 1.25), ("2p4", "sleep"): (0.0012, 0.0016),
}
BUCKETS = ("tx", "rx", "cad", "idle", "sleep")
ENERGY = {"totalJ", "accuracyPerJoule", "j_868", "j_2p4", "j_uwb"} | {
    f"{b}_{t}" for b in BUCKETS for t in ("868", "2p4", "uwb")}


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G61 ok      {label:<7}{msg}")
    else:
        nfail += 1
        print(f"  G61 FAIL    {label:<7}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G61 MISSING {label:<7}{msg}")


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


def rows(p):
    try:
        return list(csv.DictReader(open(p)))
    except OSError:
        return None


def close(a, b, tol=1e-9):
    return abs(a - b) <= tol * max(abs(a), abs(b), 1e-300)


def near(a, b, absj=3e-9):
    """The CSVs carry joules to 9 decimals: equal up to that rounding."""
    return abs(a - b) <= absj + 1e-9 * abs(b)


def ratio_bad(rs, rl, key, used):
    """Ratio checks for one pair of rows; returns a list of offending (key, column)."""
    bad = []
    full = all(f"{b}_868" in rs for b in BUCKETS)  # subject_summary carries tx and sleep only
    for t in ("868", "2p4", "uwb"):
        jsx = jlr = 0.0
        for b in BUCKETS:
            c = f"{b}_{t}"
            if c not in rs:
                continue
            vs, vl = float(rs[c]), float(rl[c])
            jsx += vs
            jlr += vl
            want = vs if t == "uwb" else vs * CUR[(t, b)][1] / CUR[(t, b)][0]
            if not near(vl, want):
                bad.append((key, c, vs, vl))
            if t != "uwb" and vs > 0:
                used.add((t, b))
        if full and (not near(float(rl[f"j_{t}"]), jlr, 6e-9) or not near(float(rs[f"j_{t}"]), jsx, 6e-9)):
            bad.append((key, f"j_{t} not the bucket sum"))
        if t == "uwb" and not near(float(rl["j_uwb"]), float(rs["j_uwb"])):
            bad.append((key, "j_uwb moved"))
    tl = sum(float(rl[f"j_{t}"]) for t in ("868", "2p4", "uwb"))
    if not near(float(rl["totalJ"]), tl, 6e-9):
        bad.append((key, "totalJ not the tier sum"))
    return bad


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    names = ("sx", "lr", "sub_sx", "sub_lr", "gos_sx", "gos_lr", "pre")
    bad = [n for n in names if rc(d, n) != 0]
    report(not bad, "RUNS", f"{len(names)} runs exited 0" if not bad else f"non-zero exit: {bad}")

    a = rows(os.path.join(d, "sx", "arm_summary.csv"))
    b = rows(os.path.join(d, "lr", "arm_summary.csv"))
    if not a or not b:
        missing("SAME", "no arm_summary.csv in sx/ or lr/")
        missing("RATIO", "no arm_summary.csv in sx/ or lr/")
    else:
        moved = []
        if [r["arm"] for r in a] != [r["arm"] for r in b] or list(a[0]) != list(b[0]):
            moved.append("rows or columns differ")
        for rs, rl in zip(a, b):
            for c in rs:
                if c not in ENERGY and rs[c] != rl[c]:
                    moved.append((rs["arm"], c))
        tr = sorted(os.path.basename(p) for p in glob.glob(os.path.join(d, "sx", "trace_*.csv")))
        ntr = 0
        for f in tr:
            ts, tl = rows(os.path.join(d, "sx", f)), rows(os.path.join(d, "lr", f))
            if tl is None or len(ts) != len(tl):
                moved.append((f, "rows"))
                continue
            ntr += 1
            for x, y in zip(ts, tl):
                for c in x:
                    if c != "cumJ" and x[c] != y[c]:
                        moved.append((f, c, x["t"]))
                        break
        report(not moved and ntr == 6 and len(a) == 6, "SAME",
               f"{len(a)} arms and {ntr} traces compared; non-energy values moved {len(moved)} (need 0)"
               + (f": {moved[:3]}" if moved else ""))
        used = set()
        rb = []
        for rs, rl in zip(a, b):
            rb += ratio_bad(rs, rl, rs["arm"], used)
        unused = sorted(set(CUR) - used)
        report(not rb and not unused, "RATIO",
               f"every bucket the SX one times the current ratio on {len(a)} arms; off {len(rb)} (need 0); "
               f"ratios never exercised {unused} (need none)" + (f": {rb[:3]}" if rb else ""))

    s1 = rows(os.path.join(d, "sub_sx", "subject_summary.csv"))
    s2 = rows(os.path.join(d, "sub_lr", "subject_summary.csv"))
    if not s1 or not s2:
        missing("SUBJ", "no subject_summary.csv")
    else:
        used = set()
        sb = []
        subj = set()
        for rs, rl in zip(s1, s2):
            key = (rs["arm"], rs["subjectId"])
            subj.add(rs["subjectId"])
            for c in rs:
                if c not in ENERGY and rs[c] != rl[c]:
                    sb.append((key, c))
            sb += ratio_bad(rs, rl, key, used)
        # 868 MHz sleep is 0 on a hybrid arm: the floor keeps that radio awake every cycle.
        need = {("868", "tx"), ("2p4", "tx"), ("2p4", "sleep")}
        report(not sb and len(s1) == len(s2) == 5 and len(subj) == 5 and need <= used, "SUBJ",
               f"{len(s1)} subject rows ({sorted(subj)}): tx and sleep (the buckets the file carries) at the "
               f"current ratios, UWB and totalJ consistent, other columns identical; off {len(sb)} (need 0); "
               f"tx/sleep ratios never exercised {sorted(need - used)} (need none)" + (f": {sb[:3]}" if sb else ""))

    gl = {}
    for n in ("gos_sx", "gos_lr"):
        t = read(os.path.join(d, n + ".log")) or ""
        gl[n] = [(float(m.group(1)), float(m.group(2)))
                 for m in re.finditer(r"gossip energy \(tier 1, gateway account\): tx (\S+) J, rx (\S+) J", t)]
        gl[n + "_sent"] = sum(int(x) for x in re.findall(r"gossip sent (\d+)", t))
    if not gl["gos_sx"] or len(gl["gos_sx"]) != len(gl["gos_lr"]):
        missing("GOSSIP", f"gossip energy lines sx {len(gl['gos_sx'])}, lr {len(gl['gos_lr'])}")
    else:
        gb = []
        for (txs, rxs), (txl, rxl) in zip(gl["gos_sx"], gl["gos_lr"]):
            # printed fixed to 6 decimals: each value carries up to 5e-7 J of rounding
            if abs(txl - txs * 28.0 / 45.0) > 1.5e-6 or abs(rxl - rxs * 7.8 / 5.3) > 1.5e-6:
                gb.append(((txs, rxs), (txl, rxl)))
        report(not gb and gl["gos_sx_sent"] > 0 and any(x[0] > 0 for x in gl["gos_sx"]), "GOSSIP",
               f"{len(gl['gos_sx'])} gateway gossip accounts, frames sent {gl['gos_sx_sent']}: tx x 28/45, rx x 7.8/5.3; "
               f"off {len(gb)} (need 0)" + (f": {gb[:2]}" if gb else ""))

    pat = r"SY-2 radio basis lr1121 \(DS\.LR1121\.W\.APP Rev 2\.0, Tables 3-4\.\.3-6\): 868 MHz tx 28 rx 7\.8 cad 7\.8 idle 1\.25 sleep 0\.0016 mA; 2\.4 GHz tx 26 rx 7\.5 cad 7\.5 idle 1\.25 sleep 0\.0016 mA"
    lrl = [read(os.path.join(d, n + ".log")) or "" for n in ("lr", "sub_lr", "gos_lr")]
    sxl = [read(os.path.join(d, n + ".log")) or "" for n in ("sx", "sub_sx", "gos_sx")]
    report(all(re.search(pat, t) for t in lrl) and not any("SY-2 radio basis" in t for t in sxl), "LINE",
           f"the SY-2 line with the profile's currents in {sum(1 for t in lrl if re.search(pat, t))} of 3 lr1121 runs, "
           f"in {sum(1 for t in sxl if 'SY-2 radio basis' in t)} of 3 sx runs (need 0)")

    sp = read(os.path.join(d, "pre", "spec_profile.txt")) or ""
    pl = read(os.path.join(d, "pre.log")) or ""
    # DESIGN_DECISIONS 11 (2026-10-17): the preset is SX1262 + SX1280 (--radioBasis=sx), so it sets sx and prints no SY-2 line.
    report(re.search(r"^radioBasis=sx ", sp, re.M) is not None and "SY-2 radio basis" not in pl, "PRESET",
           f"the preset sets --radioBasis=sx {bool(re.search(r'^radioBasis=sx ', sp, re.M))}, "
           f"prints no SY-2 line {'SY-2 radio basis' not in pl}")

    t = read(os.path.join(d, "r_bad.log")) or ""
    report(rc(d, "r_bad") == 1 and "--radioBasis must be sx or lr1121" in t, "R",
           f"--radioBasis=lr1110: exit {rc(d, 'r_bad')} (need 1)")

    print(f"  G61: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
