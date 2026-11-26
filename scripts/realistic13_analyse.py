#!/usr/bin/env python3
"""REALISTIC-13 (report/PREREG_REALISTIC13.md): quiet shift, random site with 3 (RAND3) or 5 (RAND5) stations, seeds 2001-2020.
Crew = the five workers' devices (trace + subjtrace), arm ca_tbl_full. Per run: share of crew cycles within 25 m (A25) and coverage
(errM <= 1.73 sqrt(traceSigma)). Paired 95 % bootstrap over seeds (camp_spec_analyse.ci).
  T1  A25, RAND5 - RAND3: point estimate >= 0.20 and lower end > 0
  T2  RAND5 mean crew coverage >= 0.85
usage: realistic13_analyse.py <outdir>"""
import csv, glob, os, re, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402

SEEDS, NS = range(2001, 2021), (3, 5)


def crew(d):
    e = []
    for f in glob.glob(os.path.join(d, "trace_ca_tbl_full_seed*.csv")) + glob.glob(os.path.join(d, "subjtrace_ca_tbl_full_seed*_subj*.csv")):
        e += [(float(r["errM"]), float(r["traceSigma"])) for r in csv.DictReader(open(f))]
    a = np.array(e)
    return (float(np.mean(a[:, 0] <= 25.0)), float(np.mean(a[:, 0] <= 1.73 * np.sqrt(np.maximum(a[:, 1], 1e-9))))) if len(a) else None


def main():
    out = sys.argv[1]
    problems, X, src = [], {}, set()
    for n in NS:
        for s in SEEDS:
            d = os.path.join(out, f"Q_RAND{n}", f"s{s}")
            if not os.path.exists(d + "/rc") or open(d + "/rc").read().strip() != "0":
                problems.append(f"{d}: missing or rc not 0"); continue
            rid = open(d + "/run_id.txt").read()
            m = re.search(r"^source (\S+) dirty (\d+)", rid, re.M)
            if not m or m.group(2) != "0":
                problems.append(f"{d}: not dirty 0")
            else:
                src.add(m.group(1))
            if f"rand_n{n}_k{s % 10}.csv" not in rid or f"--nGateways={n}" not in rid:
                problems.append(f"{d}: command line does not match the cell")
            v = crew(d)
            if v is None:
                problems.append(f"{d}: no crew traces")
            else:
                X[(n, s)] = v
    if len(src) > 1:
        problems.append(f"more than one source {sorted(src)}")
    k = [s for s in SEEDS if (3, s) in X and (5, s) in X]
    md = ["# REALISTIC-13 (scripts/realistic13_analyse.py)", "", f"Sources: {', '.join(sorted(src))}. Paired seeds: {len(k)}.", ""]
    t1 = t2 = False
    if k:
        m = lambda n, i: float(np.mean([X[(n, s)][i] for s in k]))
        d = cs.ci([X[(5, s)][0] - X[(3, s)][0] for s in k])
        t1 = d[0] >= 0.20 and d[1] > 0.0
        t2 = m(5, 1) >= 0.85
        md += ["| | RAND3 | RAND5 | RAND5 - RAND3 |", "|---|---|---|---|",
               f"| within 25 m | {m(3,0):.3f} | {m(5,0):.3f} | {cs.fmt(*d)} |",
               f"| coverage | {m(3,1):.3f} | {m(5,1):.3f} | |", "",
               "Per seed, within 25 m (RAND3 / RAND5): " + ", ".join(f"{s} {X[(3,s)][0]:.2f}/{X[(5,s)][0]:.2f}" for s in k)]
    md += ["", f"**T1: {'HOLDS' if t1 else 'FAILS'}**  **T2: {'HOLDS' if t2 else 'FAILS'}**", "", f"Problems: {len(problems)}"] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REALISTIC13.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()
