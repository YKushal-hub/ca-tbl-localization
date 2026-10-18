#!/usr/bin/env python3
"""S2-C-ACQ analysis (report/PREREG_S2C_ACQ.md).

usage: s2c_acq_analyse.py <outdir of s2c_acq_run.sh> <outdir of s2c_run.sh>
Writes <outdir>/S2C_ACQ.md, prints it, exits 1 on any registered abort condition.
"""
import filecmp
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import numpy as np  # noqa: E402
import s2c_analyse as sa  # noqa: E402

ARMS = ("ca_tbl_full", "static_2p4")
KEYS = ("first", "never", "av", "w25", "w25acq", "med", "c95", "rec", "J")


def main():
    out, s2c = sys.argv[1:]
    problems = []
    stamps = set()
    rcs = glob.glob(os.path.join(out, "*", "s*", "rc"))
    if len(rcs) != 12:
        problems.append(f"{len(rcs)} runs, not 12")
    for p in rcs:
        d = os.path.dirname(p)
        if not cs.ok(d):
            problems.append(f"non-zero exit: {d}")
        rid = open(os.path.join(d, "run_id.txt")).read()
        if " dirty 0 " not in rid:
            problems.append(f"run_id not dirty 0: {d}")
        m = re.search(r"^build ([^|]+)\|", rid, re.M)
        stamps.add(m.group(1).strip() if m else None)
    if len(stamps) != 1 or None in stamps:
        problems.append(f"build stamps: {sorted(map(str, stamps))}")
    good = 0
    for s in (1, 2):
        for arm in ARMS:
            a, b = os.path.join(out, "R", f"s{s}"), os.path.join(s2c, "U_A", f"s{s}")
            files = sorted(os.path.basename(p) for p in glob.glob(os.path.join(a, f"*_{arm}_seed{s}*")))
            ok = bool(files) and all(os.path.exists(os.path.join(b, f)) and
                                     filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False) for f in files)
            for summ in ("arm_summary.csv", "subject_summary.csv"):
                ok = ok and sa.arm_lines(os.path.join(a, summ), arm) == sa.arm_lines(os.path.join(b, summ), arm) != []
            good += ok
    reuse = good == 4
    md = ["# S2-C-ACQ", "", f"Reuse check: {good} of 4 seed-arm pairs byte-identical to S2-C U_A.", ""]
    Q = {a: sa.collect(out, "Q3", a) for a in ARMS}
    U = {a: {k: v for k, v in sa.collect(s2c, "U_A", a).items() if k <= 10} for a in ARMS}
    for a in ARMS:
        if len(Q[a]) < 10:
            problems.append(f"Q3 {a}: {len(Q[a])} seeds")
    md += ["| arm | metric | U_A (off) | Q3 (acquire at 3) | Q3 − U_A |", "|---|---|---|---|---|"]
    iv = {}
    for a in ARMS:
        for k in KEYS:
            v, n = sa.paired(Q[a], U[a], k)
            iv[(a, k)] = v
            mu = np.nanmean([x[k] for x in U[a].values()])
            mq = np.nanmean([x[k] for x in Q[a].values()])
            md.append(f"| {a} | {k} | {mu:.3f} | {mq:.3f} | {cs.fmt(*v) if reuse else 'not reported (reuse failed)'} |")
    nev_q = np.mean([x["never"] for x in Q["static_2p4"].values()])
    nev_u = np.mean([x["never"] for x in U["static_2p4"].values()])
    verdicts = [
        ("S1", "ca_tbl_full w25 Q3 − U_A above 0", reuse and iv[("ca_tbl_full", "w25")][1] > 0),
        ("S2", "ca_tbl_full median error Q3 − U_A below 0", reuse and iv[("ca_tbl_full", "med")][2] < 0),
        ("S3", "ca_tbl_full first fix Q3 − U_A mean ≥ 0", reuse and iv[("ca_tbl_full", "first")][0] >= 0),
        ("S4", "static 2.4 GHz never-fix share Q3 ≥ U_A", bool(nev_q >= nev_u)),
        ("S5", "reuse check 4 of 4", reuse),
    ]
    md += ["", "| # | prediction | verdict |", "|---|---|---|"]
    md += [f"| {n} | {t} | {'held' if v else 'failed'} |" for n, t, v in verdicts]
    md += ["", f"Problems: {len(problems)}"] + [f"- {p}" for p in problems]
    text = "\n".join(md) + "\n"
    print(text)
    open(os.path.join(out, "S2C_ACQ.md"), "w").write(text)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
