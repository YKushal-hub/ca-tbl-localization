#!/usr/bin/env python3
"""Score E3 (report/PREREG_E3_IMU_INTEGRITY.md): CAL-05 with and without the inertial prior.

Written and committed before E3's first cell; not to be edited after it.

The detection scoring is CAL-05's own, copied verbatim from
scratch/python_handoff/analyse_stage2.py so the two conditions are scored on identical
denominators: a contiguous seed-derived fault window, TP = suspect and injected,
FP = suspect and not injected, FN = not suspect, injected, with a fix. Recall is over
injected cycles that had a fix; the false-alarm rate is FP over non-injected cycles that
had a fix.

R1: every cv cell's digests are compared with the tracked CAL-05 sidecar of the same
(bias, fault, seed) -- results/c9_escalate/bounded_<F>_seed<s> for 40 m, else
results/c9_stage2/b<bias>_<F>_seed<s>.

Usage: e3_imu_integrity_analysis.py <root> <outdir>
"""

import csv
import hashlib
import os
import random
import sys

DURATION = 3600.0
FRACTION = 0.2
SEEDS = list(range(1, 21))
BIASES = [10, 20, 40, 80, 160]
FAULTS = ["F1", "F3"]
CONDS = ["cv", "imu"]
ARMS = ["ca_tbl_full", "ca_tbl_no_integrity"]
E1_CELLS = [(b, f) for b in (20, 40, 80) for f in FAULTS]
NBOOT = 10000
REPORT, OUT = [], []


def say(s=""):
    print(s)
    REPORT.append(s)


def bias_start(seed):
    return DURATION * (0.25 + 0.5 * float(seed % 7) / 7.0)


def detect(trace, seed):
    lo = bias_start(seed)
    hi = lo + FRACTION * DURATION
    tp = fp = fn = 0
    inj = inje = nonfix = 0
    with open(trace) as fh:
        for row in csv.DictReader(fh):
            t = float(row["t"])
            injected = (t >= lo) and (t < hi)
            susp = row["suspect"].strip() in ("1", "true", "True")
            hadfix = row["hadFix"].strip() in ("1", "true", "True")
            if injected:
                inj += 1
                if hadfix:
                    inje += 1
            elif hadfix:
                nonfix += 1
            if susp and injected:
                tp += 1
            elif susp and not injected:
                fp += 1
            elif (not susp) and injected and hadfix:
                fn += 1
    return {"tp": tp, "fp": fp, "fn": fn, "inj": inj, "inje": inje, "nonfix": nonfix,
            "trunc": hi > DURATION}


def digests(rid_path):
    text = open(rid_path).read()
    if "digest sha256" not in text:
        return None, text
    d = {}
    for line in text.split("digest sha256", 1)[1].strip().splitlines():
        h, name = line.split(None, 1)
        d[name.strip()] = h
    return d, text


def check_dir(d):
    if not os.path.isdir(d):
        return ["missing directory"], None
    rid = os.path.join(d, "run_id.txt")
    if not os.path.exists(rid):
        return ["no run_id.txt"], None
    dg, text = digests(rid)
    probs = []
    if " dirty 0 " not in text:
        probs.append("not dirty 0")
    if "profile optimized" not in text:
        probs.append("not optimized")
    if dg is None:
        return probs + ["no digest -- run did not finish"], None
    for name, h in dg.items():
        p = os.path.join(d, name)
        if not os.path.exists(p) or hashlib.sha256(open(p, "rb").read()).hexdigest() != h:
            probs.append("digest mismatch or absent: %s" % name)
    with open(os.path.join(d, "arm_summary.csv"), newline="") as f:
        arms = sorted(r["arm"] for r in csv.DictReader(f))
    if arms != sorted(ARMS):
        probs.append("arm_summary rows %s" % arms)
    if len([n for n in dg if n.startswith("trace_")]) != 2:
        probs.append("traces not 2")
    return probs, dg


def boot_mean(vals, rng):
    if not vals:
        return None, None, None
    m = sum(vals) / len(vals)
    bs = sorted(sum(vals[rng.randrange(len(vals))] for _ in vals) / len(vals) for _ in range(NBOOT))
    return m, bs[int(0.025 * NBOOT)], bs[int(0.975 * NBOOT) - 1]


def fmt(v, f="%+.4f"):
    return "nan" if v is None else f % v


def crossing(points):
    """Bias at which pooled recall crosses 0.5, linear between bracketing magnitudes."""
    pts = sorted(points)
    for (b0, r0), (b1, r1) in zip(pts, pts[1:]):
        if r0 < 0.5 <= r1:
            return b0 + (0.5 - r0) * (b1 - b0) / (r1 - r0)
    return None


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    root, outdir = argv[1], argv[2]
    os.makedirs(outdir, exist_ok=True)
    rng = random.Random(20261016)

    say("=== completeness")
    cells, excluded, dgs = {}, [], {}
    for cond in CONDS:
        for b in BIASES:
            for f in FAULTS:
                for s in SEEDS:
                    d = os.path.join(root, "%s_b%d_%s_s%d" % (cond, b, f, s))
                    probs, dg = check_dir(d)
                    if probs:
                        excluded.append((d, probs))
                        continue
                    det = detect(os.path.join(d, "trace_ca_tbl_full_seed%d.csv" % s), s)
                    if det["trunc"]:
                        excluded.append((d, ["fault window truncated"]))
                        continue
                    cells[(cond, b, f, s)] = det
                    dgs[(cond, b, f, s)] = dg
    total = len(CONDS) * len(BIASES) * len(FAULTS) * len(SEEDS)
    say("  usable %d of %d" % (len(cells), total))
    for d, p in excluded:
        say("  EXCLUDED %s: %s" % (d, "; ".join(p)))
    if len(excluded) > 0.10 * total:
        say("ABORT: more than 10% excluded")
        return 1

    say("\n=== R1 reproduction: cv cells against the tracked CAL-05 sidecars")
    same_cells = diff_cells = missing = 0
    for b in BIASES:
        for f in FAULTS:
            for s in SEEDS:
                if ("cv", b, f, s) not in dgs:
                    continue
                if b == 40:
                    ref = "results/c9_escalate/bounded_%s_seed%d/run_id.txt" % (f, s)
                else:
                    ref = "results/c9_stage2/b%d_%s_seed%d/run_id.txt" % (b, f, s)
                if not os.path.exists(ref):
                    missing += 1
                    continue
                rd, _ = digests(ref)
                mine = dgs[("cv", b, f, s)]
                if rd is not None and all(mine.get(k) == v for k, v in rd.items()) and \
                        set(rd) == set(mine):
                    same_cells += 1
                else:
                    diff_cells += 1
                    if diff_cells <= 5:
                        bad = [k for k in rd if mine.get(k) != rd.get(k)] if rd else ["no digest"]
                        say("  DIFFERS b%d %s seed %d: %s" % (b, f, s, ",".join(bad)))
    say("  identical %d, differing %d, no CAL-05 sidecar %d" % (same_cells, diff_cells, missing))
    OUT.append({"metric": "R1_identical_cells", "bias": "", "fault": "", "estimate": same_cells,
                "lo": "", "hi": "", "n": same_cells + diff_cells})

    say("\n=== detection, pooled over seeds (CAL-05's presentation)")
    say("  bias fault cond   precision  recall   FA rate    TP    FP    FN")
    pooled = {}
    for f in FAULTS:
        for b in BIASES:
            for cond in CONDS:
                ks = [k for k in cells if k[0] == cond and k[1] == b and k[2] == f]
                TP = sum(cells[k]["tp"] for k in ks)
                FP = sum(cells[k]["fp"] for k in ks)
                FN = sum(cells[k]["fn"] for k in ks)
                NF = sum(cells[k]["nonfix"] for k in ks)
                prec = TP / (TP + FP) if TP + FP else None
                rec = TP / (TP + FN) if TP + FN else None
                far = FP / NF if NF else None
                pooled[(cond, b, f)] = rec
                say("  %4d  %s  %-4s  %9s  %7s  %8s  %5d %5d %5d" % (
                    b, f, cond, fmt(prec, "%.3f"), fmt(rec, "%.3f"), fmt(far, "%.5f"), TP, FP, FN))
                OUT.append({"metric": "pooled_recall", "bias": b, "fault": f + "/" + cond,
                            "estimate": "" if rec is None else "%.6g" % rec, "lo": "", "hi": "",
                            "n": len(ks)})
                OUT.append({"metric": "pooled_far", "bias": b, "fault": f + "/" + cond,
                            "estimate": "" if far is None else "%.6g" % far, "lo": "", "hi": "",
                            "n": len(ks)})

    say("\n=== paired per-seed differences, imu - cv")
    e1 = {}
    for f in FAULTS:
        for b in BIASES:
            dr, dfa = [], []
            for s in SEEDS:
                c, i = cells.get(("cv", b, f, s)), cells.get(("imu", b, f, s))
                if not c or not i:
                    continue
                if c["tp"] + c["fn"] and i["tp"] + i["fn"]:
                    dr.append(i["tp"] / (i["tp"] + i["fn"]) - c["tp"] / (c["tp"] + c["fn"]))
                if c["nonfix"] and i["nonfix"]:
                    dfa.append(i["fp"] / i["nonfix"] - c["fp"] / c["nonfix"])
            mr, lr, hr = boot_mean(dr, rng)
            mf, lf, hf = boot_mean(dfa, rng)
            e1[(b, f)] = (mr, lr, hr)
            say("  %4d %s  recall %s [%s, %s]   FA rate %s [%s, %s]   n=%d" % (
                b, f, fmt(mr), fmt(lr), fmt(hr), fmt(mf, "%+.5f"), fmt(lf, "%+.5f"),
                fmt(hf, "%+.5f"), len(dr)))
            OUT.append({"metric": "recall_diff", "bias": b, "fault": f,
                        "estimate": "" if mr is None else "%.6g" % mr,
                        "lo": "" if lr is None else "%.6g" % lr,
                        "hi": "" if hr is None else "%.6g" % hr, "n": len(dr)})
            OUT.append({"metric": "far_diff", "bias": b, "fault": f,
                        "estimate": "" if mf is None else "%.6g" % mf,
                        "lo": "" if lf is None else "%.6g" % lf,
                        "hi": "" if hf is None else "%.6g" % hf, "n": len(dfa)})

    say("\n=== E1 (registered): recall difference > 0 in all six of 20/40/80 m x F1/F3,")
    say("    CI excluding 0 in at least four")
    pos = sum(1 for k in E1_CELLS if e1[k][0] is not None and e1[k][0] > 0)
    sig = sum(1 for k in E1_CELLS if e1[k][1] is not None and e1[k][1] > 0)
    held = pos == len(E1_CELLS) and sig >= 4
    say("  point estimates > 0: %d of 6;  CIs above 0: %d of 6  ->  E1 %s" % (
        pos, sig, "HOLDS" if held else "FAILS"))

    say("\n=== 50 % recall crossing (descriptive; CAL-05 cv: ~58 m F1, ~33 m F3)")
    for f in FAULTS:
        for cond in CONDS:
            pts = [(b, pooled[(cond, b, f)]) for b in BIASES if pooled.get((cond, b, f)) is not None]
            c = crossing(pts)
            say("  %s %-4s %s" % (f, cond, "not bracketed" if c is None else "%.1f m" % c))

    with open(os.path.join(outdir, "e3_analysis.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=["metric", "bias", "fault", "n", "estimate", "lo", "hi"])
        w.writeheader()
        w.writerows(OUT)
    with open(os.path.join(outdir, "e3_report.txt"), "w") as fh:
        fh.write("\n".join(REPORT) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
