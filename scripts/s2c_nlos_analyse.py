#!/usr/bin/env python3
"""S2-C-NLOS analysis (report/PREREG_S2C_NLOS.md): --nlosRSource none and detector against the
operative (oracle) tag, ca_tbl_full, paired over seeds 1-10.

usage: s2c_nlos_analyse.py <outdir of s2c_nlos_run.sh>
Writes <outdir>/S2C_NLOS.md, prints it, exits 1 on any registered abort condition.
"""
import csv
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import numpy as np  # noqa: E402
import s2c_analyse as sa  # noqa: E402

MODES = ("oper", "none", "det")
KEYS = ("av", "w25", "med", "c95", "J", "first", "nis", "anees", "nis0")


def tag0_nis(d):
    s = int(d.rsplit("s", 1)[1])
    v = [float(r["mahalanobis"]) for r in sa.rows(os.path.join(d, f"trace_ca_tbl_full_seed{s}.csv")) or []
         if r["hadFix"] == "1" and float(r["t"]) >= sa.WARM and float(r["mahalanobis"]) > 0]
    return float(np.mean([x > sa.NIS99 for x in v])) if v else float("nan")


def collect(out, cell):
    res = {}
    for d in cs.seeds(out, cell):
        if cs.ok(d):
            m = sa.seed_metrics(d, "ca_tbl_full")
            if m is not None:
                m["nis0"] = tag0_nis(d)
                res[int(d.rsplit("s", 1)[1])] = m
    return res


def main():
    out = sys.argv[1]
    problems = []
    stamps = set()
    rcs = glob.glob(os.path.join(out, "*", "s*", "rc"))
    if len(rcs) != 60:
        problems.append(f"{len(rcs)} runs, not 60")
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
    R = {(c, m): collect(out, f"N_{c}_{m}") for c in ("A", "B") for m in MODES}
    for k, v in R.items():
        if len(v) < 10:
            problems.append(f"{k}: {len(v)} seeds")
    md = ["# S2-C-NLOS", ""]
    iv = {}
    for c in ("A", "B"):
        md += [f"## {c}", "", "| metric | oper | none | det | none − oper | det − oper | det − none |", "|---|---|---|---|---|---|---|"]
        for k in KEYS:
            means = [np.nanmean([x[k] for x in R[(c, m)].values()]) for m in MODES]
            cells = []
            for a, b in (("none", "oper"), ("det", "oper"), ("det", "none")):
                v, _ = sa.paired(R[(c, a)], R[(c, b)], k)
                iv[(c, a, b, k)] = v
                cells.append(cs.fmt(*v))
            md.append(f"| {k} | " + " | ".join(f"{x:.3f}" for x in means) + " | " + " | ".join(cells) + " |")
        md.append("")
    inc0 = sa.inc0
    v = [
        ("Q1", "none − oper: CEP95 and w25 contain 0, A and B", all(inc0(iv[(c, "none", "oper", k)]) for c in ("A", "B") for k in ("c95", "w25"))),
        ("Q2", "det − oper: CEP95 not entirely below 0, A and B", all(not (iv[(c, "det", "oper", "c95")][2] < 0) for c in ("A", "B"))),
        ("Q3", "det − none: CEP95 and w25 contain 0, A and B", all(inc0(iv[(c, "det", "none", k)]) for c in ("A", "B") for k in ("c95", "w25"))),
        ("Q4", "none − oper: tag 0 NIS share mean ≥ 0, A and B", all(iv[(c, "none", "oper", "nis0")][0] >= 0 for c in ("A", "B"))),
    ]
    md += ["## Registered predictions", "", "| # | prediction | verdict |", "|---|---|---|"]
    for n, t, ok in v:
        md.append(f"| {n} | {t} | {'held' if ok else 'failed'} |")
    md += ["", f"Problems: {len(problems)}"] + [f"- {p}" for p in problems]
    text = "\n".join(md) + "\n"
    print(text)
    open(os.path.join(out, "S2C_NLOS.md"), "w").write(text)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
