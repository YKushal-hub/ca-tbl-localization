#!/usr/bin/env python3
"""L.2.5 -- the proxy staleness bound, judged by the ages it is actually asked to judge.

Registered in report/PREREG_L25_PROXY_AGE.md, committed with it before the first
registered cell ran. Reads the runs scripts/l25_proxy_age_run.sh writes:

    python3 scripts/l25_proxy_age.py [<root>]      default results/l25_proxy_age_20260929

<root>/<cell>/seed_<n>/ must hold run_id.txt, stdout.txt, and the run's
proxy_checks_ca_tbl_full_seed<n>.csv and trace_ca_tbl_full_seed<n>.csv (traces are not
tracked; regenerate them from the invocation in run_id.txt, whose SHA-256 block vouches
for them).

Exit 0 when every registered cell was analysed without an abort, 1 otherwise. An abort
prints its reason and turns that cell's verdict into CANNOT DECIDE -- it never falls back
to a verdict computed from what survived.

Pure standard library (python 3.8), because the analysis has to run wherever the
simulator does.
"""

import csv
import hashlib
import math
import os
import re
import sys

BOUND_S = 120.0  # the spec's proxy staleness bound, the number under test
MIN_HELD = 20  # A3: fewer pooled held ages than this and a cell is not classified
SEEDS = (7, 11, 13)
# SELF-TEST ONLY. Other seeds, or a build that is not clean, exercise the code path on runs
# that are not the registered ones. Either override prints a banner and forces exit 3, so
# nothing produced under one can be mistaken for the registered analysis.
SELFTEST = bool(os.environ.get("L25_SEEDS") or os.environ.get("L25_ALLOW_DIRTY"))
if os.environ.get("L25_SEEDS"):
    SEEDS = tuple(int(s) for s in os.environ["L25_SEEDS"].split(","))
PRIMARY = ("searcher_t1", "sparse_t1")
SECONDARY = ("searcher_t2", "sparse_t2")
CELLS = PRIMARY + SECONDARY

COMMON = ("--arms=ca_tbl_full --duration=1800 --trace=true --queryProxy=true "
          "--identityQueryPeriodS=30")
FINDERS = ("--nFinders=4 --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 "
           "--sendHandshakeFrames=true --finderDigestPeriodS=20")
ARC = "--boxHalf=2000 --nGateways=5 --gwArcDeg=140 --gwArcCentreDeg=180"
GRID = "--gwGrid=2 --boxHalf=5000"
T1 = "--identityQueryTarget=1"
T2 = "--nTags=2 --identityQueryTarget=2"
CELL_FLAGS = {
    "searcher_t1": " ".join((COMMON, ARC, FINDERS, T1)),
    "sparse_t1": " ".join((COMMON, GRID, FINDERS, T1)),
    "searcher_t2": " ".join((COMMON, ARC, FINDERS, T2)),
    "sparse_t2": " ".join((COMMON, GRID, FINDERS, T2)),
}
BIN_SUFFIX = "/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
AGE_BINS = ((0.0, 30.0), (30.0, 60.0), (60.0, 120.0), (120.0, 300.0), (300.0, float("inf")))
LAGS_S = (30.0, 60.0, 120.0, 300.0, 600.0)


class Abort(Exception):
    pass


def nearest_rank(sorted_vals, p):
    """The same quantile the simulator prints: nearest rank, 1-based, ceil(p*n)."""
    if not sorted_vals:
        return float("nan")
    r = int(math.ceil(p * len(sorted_vals)))
    return sorted_vals[max(r, 1) - 1]


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for block in iter(lambda: f.read(1 << 20), b""):
            h.update(block)
    return h.hexdigest()


def load_run(root, cell, seed):
    d = os.path.join(root, cell, "seed_%d" % seed)
    rid_path = os.path.join(d, "run_id.txt")
    if not os.path.isfile(rid_path):
        raise Abort("A1 %s seed %d: no run_id.txt" % (cell, seed))
    lines = open(rid_path).read().splitlines()
    # A1: provenance. Line 2 is "build <stamp> | <argv>", line 3 "source <sha> dirty <n>
    # profile <p>", then a "digest sha256" block of "<hash>  <name>".
    if len(lines) < 4 or not lines[2].startswith("source "):
        raise Abort("A1 %s seed %d: run_id.txt has no source line" % (cell, seed))
    src = lines[2].split()
    clean = len(src) >= 6 and src[2] == "dirty" and src[3] == "0"
    if (len(src) < 6 or src[5] != "optimized" or
            (not clean and not os.environ.get("L25_ALLOW_DIRTY"))):
        raise Abort("A1 %s seed %d: not a clean optimized build: %r" % (cell, seed, lines[2]))
    argv = lines[1].split("|", 1)[1].strip() if "|" in lines[1] else ""
    toks = argv.split()
    if not toks or not toks[0].endswith(BIN_SUFFIX):
        raise Abort("A1 %s seed %d: binary is not the registered one: %r" % (cell, seed, toks[:1]))
    want = CELL_FLAGS[cell].split() + ["--seed=%d" % seed]
    got = [t for t in toks[1:] if not t.startswith("--outdir=")]
    if sorted(got) != sorted(want):
        raise Abort("A1 %s seed %d: invocation differs from the registered one:\n  got  %s\n"
                    "  want %s" % (cell, seed, " ".join(got), " ".join(want)))
    outdir = [t for t in toks if t.startswith("--outdir=")]
    if not outdir or os.path.realpath(outdir[0].split("=", 1)[1]) != os.path.realpath(d):
        raise Abort("A1 %s seed %d: recorded outdir is not this directory" % (cell, seed))
    if "digest sha256" not in lines:
        raise Abort("A1 %s seed %d: no digest block -- the run did not finish" % (cell, seed))
    digest = {}
    for ln in lines[lines.index("digest sha256") + 1:]:
        m = re.match(r"^([0-9a-f]{64})  (\S+)$", ln)
        if m:
            digest[m.group(2)] = m.group(1)
    pc_name = "proxy_checks_ca_tbl_full_seed%d.csv" % seed
    tr_name = "trace_ca_tbl_full_seed%d.csv" % seed
    for name in (pc_name, tr_name, "arm_summary.csv"):
        p = os.path.join(d, name)
        if name not in digest:
            raise Abort("A1 %s seed %d: %s is not in the run's digest" % (cell, seed, name))
        if not os.path.isfile(p):
            raise Abort("A1 %s seed %d: %s missing (traces are untracked: regenerate from "
                        "run_id.txt)" % (cell, seed, name))
        if sha256(p) != digest[name]:
            raise Abort("A1 %s seed %d: %s does not match its digest" % (cell, seed, name))

    rows = list(csv.DictReader(open(os.path.join(d, pc_name))))
    stdout = open(os.path.join(d, "stdout.txt")).read()
    m = re.search(r"NET4 proxy checks: (\d+) arrivals of (\d+) distinct queries .*?, (\d+) held"
                  r" a matching entry .*?\), (\d+) inside the bound", stdout)
    if not m:
        raise Abort("A4 %s seed %d: no NET4 proxy-checks line on stdout" % (cell, seed))
    n_chk, n_q, n_held, n_ans = (int(g) for g in m.groups())
    held = [r for r in rows if r["held"] == "1"]
    # A4: two paths to the same count -- the summary the simulator printed and the rows it
    # wrote -- must agree, or one of them is not describing the run.
    if (len(rows), len(held), sum(r["answered"] == "1" for r in rows)) != (n_chk, n_held, n_ans):
        raise Abort("A4 %s seed %d: stdout says %d checks / %d held / %d answered, the CSV "
                    "has %d / %d / %d" % (cell, seed, n_chk, n_held, n_ans, len(rows),
                                          len(held),
                                          sum(r["answered"] == "1" for r in rows)))
    if len({r["queryId"] for r in rows}) != n_q:
        raise Abort("A4 %s seed %d: distinct query ids disagree with stdout" % (cell, seed))
    for r in held:
        age = float(r["t"]) - float(r["entryTimeS"])
        if abs(age - float(r["ageS"])) > 1e-5:
            raise Abort("A4 %s seed %d: ageS column is not t - entryTimeS" % (cell, seed))
        if r["manifestId"] != cell[-1]:
            raise Abort("A4 %s seed %d: a check names manifest %s" % (cell, seed,
                                                                      r["manifestId"]))
    qm = re.search(r"NET4 queries \(all radios\): transmitted (\d+) \(relayed (\d+)\), "
                   r"received (\d+), refused by duty (\d+), CAD-exhausted (\d+), lost to "
                   r"collision (\d+)", stdout)
    tm = re.search(r"TABLE: (\d+)/(\d+) finders hold the casualty entry, mean age ([0-9.]+) s",
                   stdout)
    trace = list(csv.DictReader(open(os.path.join(d, tr_name))))
    return {
        "rows": rows,
        "held": held,
        "n_q": n_q,
        "queries": tuple(int(g) for g in qm.groups()) if qm else None,
        "table_age": float(tm.group(3)) if tm else None,
        "trace": trace,
    }


def interp_truth(trace_t, trace_xy, t):
    """True position at t, linear between the cycles either side of it."""
    lo, hi = 0, len(trace_t) - 1
    if t <= trace_t[0]:
        return trace_xy[0]
    if t >= trace_t[hi]:
        return trace_xy[hi]
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if trace_t[mid] <= t:
            lo = mid
        else:
            hi = mid
    f = (t - trace_t[lo]) / (trace_t[hi] - trace_t[lo])
    return (trace_xy[lo][0] + f * (trace_xy[hi][0] - trace_xy[lo][0]),
            trace_xy[lo][1] + f * (trace_xy[hi][1] - trace_xy[lo][1]))


def fmt(v, nd=1):
    return "-" if v is None or (isinstance(v, float) and math.isnan(v)) else ("%.*f" % (nd, v))


def median(v):
    s = sorted(v)
    return nearest_rank(s, 0.5) if s else float("nan")


def classify(ages):
    """The registered supply-side rule. Ages are the pooled held-entry ages of one cell."""
    if not ages:
        return "CANNOT DECIDE (A2: no held entry -- the observable is unreachable here)"
    if len(ages) < MIN_HELD:
        return "CANNOT DECIDE (A3: %d held ages < %d)" % (len(ages), MIN_HELD)
    s = sorted(ages)
    within = sum(a <= BOUND_S for a in s) / len(s)
    if within >= 0.90:
        return "S-a: the bound does not bind (%.1f%% within %.0f s)" % (100 * within, BOUND_S)
    if nearest_rank(s, 0.5) > BOUND_S:
        return "S-b: the bound is too tight (median %.1f s > %.0f s)" % (nearest_rank(s, 0.5),
                                                                        BOUND_S)
    return "S-c: the bound binds on a tail (%.1f%% within %.0f s, median %.1f s)" % (
        100 * within, BOUND_S, nearest_rank(s, 0.5))


def supply_table(cell, runs):
    out = []
    out.append("| seed | queries reaching a proxy radio | checks | held | answered | p10 | p50 "
               "| p90 | max | within 120 s |")
    out.append("|---|---|---|---|---|---|---|---|---|---|")
    pooled = []
    tot = [0, 0, 0, 0]
    for sd in SEEDS:
        r = runs[sd]
        ages = sorted(float(x["ageS"]) for x in r["held"])
        pooled += ages
        nans = sum(x["answered"] == "1" for x in r["rows"])
        tot[0] += r["n_q"]
        tot[1] += len(r["rows"])
        tot[2] += len(r["held"])
        tot[3] += nans
        out.append("| %d | %d | %d | %d | %d | %s | %s | %s | %s | %d/%d |" % (
            sd, r["n_q"], len(r["rows"]), len(r["held"]), nans,
            fmt(nearest_rank(ages, 0.1)), fmt(nearest_rank(ages, 0.5)),
            fmt(nearest_rank(ages, 0.9)), fmt(ages[-1] if ages else None),
            sum(a <= BOUND_S for a in ages), len(ages)))
    pooled.sort()
    out.append("| **pooled** | %d | %d | %d | %d | %s | %s | %s | %s | %d/%d |" % (
        tot[0], tot[1], tot[2], tot[3], fmt(nearest_rank(pooled, 0.1)),
        fmt(nearest_rank(pooled, 0.5)), fmt(nearest_rank(pooled, 0.9)),
        fmt(pooled[-1] if pooled else None), sum(a <= BOUND_S for a in pooled), len(pooled)))
    return out, pooled


def who_held(runs):
    """Checks and held entries by the class and tier of the radio that was asked."""
    by = {}
    for sd in SEEDS:
        for x in runs[sd]["rows"]:
            k = (x["identityClass"], x["tier"])
            c = by.setdefault(k, [0, 0])
            c[0] += 1
            c[1] += x["held"] == "1"
    return ", ".join("%s tier %s: %d/%d" % (k[0], k[1], v[1], v[0]) for k, v in sorted(by.items()))


def demand_tables(runs):
    """Descriptive only (registered as such): what a held entry is worth at its age."""
    rows_out = []
    per_bin = [[] for _ in AGE_BINS]
    align = [0, 0, 0]  # entry == trace est at te, at te - dt, neither
    lag = {a: ([], []) for a in LAGS_S}
    lag0 = []
    for sd in SEEDS:
        tr = runs[sd]["trace"]
        tt = [float(x["t"]) for x in tr]
        truth = [(float(x["trueX"]), float(x["trueY"])) for x in tr]
        est = [(float(x["estX"]), float(x["estY"])) for x in tr]
        idx = {round(t, 4): i for i, t in enumerate(tt)}
        dt = tt[1] - tt[0]
        for x in runs[sd]["held"]:
            tq, te = float(x["t"]), float(x["entryTimeS"])
            ex, ey = float(x["entryX"]), float(x["entryY"])
            gq = interp_truth(tt, truth, tq)
            ge = interp_truth(tt, truth, te)
            age = tq - te
            eq = math.hypot(ex - gq[0], ey - gq[1])
            fe = math.hypot(ex - ge[0], ey - ge[1])
            ds = math.hypot(gq[0] - ge[0], gq[1] - ge[1])
            for b, (lo, hi) in enumerate(AGE_BINS):
                if lo < age <= hi or (b == 0 and age <= hi):
                    per_bin[b].append((eq, fe, ds))
                    break
            i = idx.get(round(te, 4))
            j = idx.get(round(te - dt, 4))
            if i is not None and math.hypot(ex - est[i][0], ey - est[i][1]) < 1e-2:
                align[0] += 1
            elif j is not None and math.hypot(ex - est[j][0], ey - est[j][1]) < 1e-2:
                align[1] += 1
            else:
                align[2] += 1
        # The age cost with no proxy in it: every cycle, the estimate held for a lag.
        for i in range(len(tt)):
            lag0.append(math.hypot(est[i][0] - truth[i][0], est[i][1] - truth[i][1]))
            for a in LAGS_S:
                k = i + int(round(a / dt))
                if k < len(tt):
                    lag[a][0].append(math.hypot(truth[k][0] - truth[i][0],
                                                truth[k][1] - truth[i][1]))
                    lag[a][1].append(math.hypot(est[i][0] - truth[k][0],
                                                est[i][1] - truth[k][1]))
    rows_out.append("| held-entry age at query | n | median error vs truth at query (m) | "
                    "median error at its own timestamp (m) | median displacement since (m) |")
    rows_out.append("|---|---|---|---|---|")
    for (lo, hi), v in zip(AGE_BINS, per_bin):
        label = "(%g, %g] s" % (lo, hi) if hi != float("inf") else "> %g s" % lo
        rows_out.append("| %s | %d | %s | %s | %s |" % (
            label, len(v), fmt(median([a for a, _, _ in v])), fmt(median([b for _, b, _ in v])),
            fmt(median([c for _, _, c in v]))))
    rows_out.append("")
    rows_out.append("Entry position against the trace estimate: equal to the estimate of its own "
                    "timestamp's cycle %d, of the cycle before it %d, neither %d." % tuple(align))
    rows_out.append("")
    rows_out.append("| lag held (s) | n cycles | median true displacement (m) | p90 (m) | "
                    "median error of the held estimate vs truth then (m) |")
    rows_out.append("|---|---|---|---|---|")
    rows_out.append("| 0 | %d | 0.0 | 0.0 | %s |" % (len(lag0), fmt(median(lag0))))
    for a in LAGS_S:
        d, e = lag[a]
        rows_out.append("| %g | %d | %s | %s | %s |" % (a, len(d), fmt(median(d)),
                                                         fmt(nearest_rank(sorted(d), 0.9)),
                                                         fmt(median(e))))
    return rows_out


def main(argv):
    root = argv[1] if len(argv) > 1 else "results/l25_proxy_age_20260929"
    cells = CELLS
    if SELFTEST:
        print("**SELF-TEST MODE (L25_SEEDS / L25_ALLOW_DIRTY set): NOT the registered "
              "analysis, nothing below is citable; exit status is forced to 3.**\n")
        if os.environ.get("L25_CELLS"):
            cells = tuple(os.environ["L25_CELLS"].split(","))
    ok = True
    verdicts = {}
    for cell in cells:
        print("## %s%s\n" % (cell, "" if cell in PRIMARY else " (secondary: reported, decides "
                                                                   "nothing)"))
        print("`%s`, seeds %s\n" % (CELL_FLAGS[cell], ", ".join(str(s) for s in SEEDS)))
        try:
            runs = {sd: load_run(root, cell, sd) for sd in SEEDS}
        except Abort as e:
            print("**ABORT** %s\n" % e)
            verdicts[cell] = "CANNOT DECIDE (%s)" % str(e).split(":")[0]
            ok = False
            continue
        table, pooled = supply_table(cell, runs)
        print("\n".join(table))
        print("")
        print("Checks / held by the class and tier of the radio asked: %s.\n" % who_held(runs))
        q = [runs[sd]["queries"] for sd in SEEDS]
        if all(q):
            print("QUERY frames, all radios, summed over seeds: transmitted %d (relayed %d), "
                  "received %d, refused by duty %d, CAD-exhausted %d, lost to collision %d.\n"
                  % tuple(sum(v[i] for v in q) for i in range(6)))
        verdicts[cell] = classify(pooled)
        if cell.endswith("_t1") and pooled:
            # C1, descriptive: the same quantity by a different code path. The TABLE line
            # reads each finder's 868 table once, at t = duration, through GetTableEntry --
            # not through the proxy record. Twelve samples at most, so it can only say the
            # two are the same order of magnitude; it decides nothing.
            ta = [runs[sd]["table_age"] for sd in SEEDS]
            if all(v is not None for v in ta):
                print("C1 (descriptive): mean held age at query %.1f s; mean age of the "
                      "finders' casualty entry at end of run (stdout TABLE line), seeds %s: "
                      "%s s.\n" % (sum(pooled) / len(pooled),
                                   "/".join(str(s) for s in SEEDS),
                                   "/".join("%.1f" % v for v in ta)))
            else:
                print("C1 (descriptive): no TABLE line in at least one seed "
                      "(no finder held the casualty entry at end of run).\n")
            print("\n".join(demand_tables(runs)))
            print("")
        print("**Verdict:** %s\n" % verdicts[cell])
    print("## Registered decision\n")
    for cell in cells:
        print("- %s%s: %s" % (cell, "" if cell in PRIMARY else " (secondary)", verdicts[cell]))
    if SELFTEST:
        return 3
    prim = [verdicts[c] for c in PRIMARY]
    print("")
    if any(v.startswith("CANNOT") for v in prim):
        print("Action: none decided -- a primary cell could not be classified.")
    elif any(v.startswith("S-b") for v in prim):
        print("Action (registered): record that the spec bound is too tight for this topology; "
              "the default stays 120 s; faster dissemination is filed as the change that "
              "would address it, not built here.")
    elif all(v.startswith("S-a") for v in prim):
        print("Action (registered): record that the bound does not bind for the casualty in "
              "either registered regime; the default stays 120 s; L.2.5's premise is refuted "
              "for these regimes.")
    else:
        print("Action (registered): record the tail the bound refuses; the default stays 120 s.")
    return 0 if ok else 1


if __name__ == "__main__":
    sys.exit(main(sys.argv))
