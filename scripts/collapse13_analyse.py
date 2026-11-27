#!/usr/bin/env python3
"""COLLAPSE-13 (report/PREREG_COLLAPSE13.md): DD 75 --sigmaShadow off/on, seeds 2101-2110, 20 workers, second half of the run.
Per run, over the 20 workers' traces: coverage (errM <= 1.73 sqrt(traceSigma)), share within 2 m, median error, median bound 1.73 sqrt(traceSigma).
Paired 95 % bootstrap over seeds (camp_spec_analyse.ci). Four-station cells F = K5 K8 H5 H8; K2 reported.
  H60  mean over F of coverage, on - off: point >= 0.03 and lower end > 0
  H61  in every F cell, on's coverage >= off's - 0.02 (point)
  H62  in every F cell, |share within 2 m on - off| <= 0.03 (point)
  H63  in every F cell, on's coverage >= 0.85
usage: collapse13_analyse.py <outdir>"""
import csv, glob, os, re, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402

CELLS, F, SEEDS = ("K5", "K8", "H5", "H8", "K2"), ("K5", "K8", "H5", "H8"), range(2101, 2111)


def stats(d):
    c, w, e, b, n = [], [], [], [], 0
    for f in glob.glob(os.path.join(d, "trace_ca_tbl_full_seed*.csv")) + glob.glob(os.path.join(d, "subjtrace_ca_tbl_full_seed*_subj*.csv")):
        a = np.array([[float(r["t"]), float(r["errM"]), float(r["traceSigma"])] for r in csv.DictReader(open(f))])
        if not len(a):
            continue
        n += 1
        h = a[a[:, 0] > a[:, 0].max() * 0.5]
        bd = 1.73 * np.sqrt(np.maximum(h[:, 2], 0.0))
        c.append(np.mean(h[:, 1] <= bd)); w.append(np.mean(h[:, 1] <= 2.0)); e += list(h[:, 1]); b += list(bd)
    return (float(np.mean(c)), float(np.mean(w)), float(np.median(e)), float(np.median(b))) if n == 20 else None


def main():
    out = sys.argv[1]
    problems, X, src = [], {}, set()
    for c in CELLS:
        for x in ("off", "on"):
            for s in SEEDS:
                d = os.path.join(out, f"{c}_{x}", f"seed{s}")
                if not os.path.exists(d + "/rc") or open(d + "/rc").read().strip() != "0":
                    problems.append(f"{d}: missing or rc not 0"); continue
                m = re.search(r"^source (\S+) dirty (\d+)", open(d + "/run_id.txt").read(), re.M)
                if not m or m.group(2) != "0":
                    problems.append(f"{d}: not dirty 0")
                else:
                    src.add(m.group(1))
                if ("DD75 sigmaShadow" in open(d + "/run.log", errors="replace").read()) != (x == "on"):
                    problems.append(f"{d}: DD75 line does not match {x}")
                v = stats(d)
                if v is None:
                    problems.append(f"{d}: not 20 crew traces")
                else:
                    X[(c, x, s)] = v
    if len(src) > 1:
        problems.append(f"more than one source {sorted(src)}")
    md = ["# COLLAPSE-13 (scripts/collapse13_analyse.py)", "", f"Sources: {', '.join(sorted(src))}.", "",
          "| cell | coverage off | on | on - off | within 2 m off | on | median error | median bound off | on |", "|---|---|---|---|---|---|---|---|---|"]
    dc = {}
    h61 = h62 = h63 = True
    for c in CELLS:
        k = [s for s in SEEDS if (c, "off", s) in X and (c, "on", s) in X]
        if not k:
            md.append(f"| {c} | no data |"); h61 = h62 = h63 = False if c in F else h61; continue
        m = lambda x, i: float(np.mean([X[(c, x, s)][i] for s in k]))
        md_ = lambda x, i: float(np.median([X[(c, x, s)][i] for s in k]))
        d = cs.ci([X[(c, "on", s)][0] - X[(c, "off", s)][0] for s in k])
        if c in F:
            dc[c] = {s: X[(c, "on", s)][0] - X[(c, "off", s)][0] for s in k}
            h61 = h61 and m("on", 0) >= m("off", 0) - 0.02
            h62 = h62 and abs(m("on", 1) - m("off", 1)) <= 0.03
            h63 = h63 and m("on", 0) >= 0.85
        md.append(f"| {c} | {m('off',0):.3f} | {m('on',0):.3f} | {cs.fmt(*d)} | {m('off',1):.3f} | {m('on',1):.3f} | {md_('off',2):.2f} | {md_('off',3):.2f} | {md_('on',3):.2f} |")
    ks = [s for s in SEEDS if all(s in dc.get(c, {}) for c in F)]
    h60 = False
    if ks and len(dc) == 4:
        d = cs.ci([np.mean([dc[c][s] for c in F]) for s in ks])
        h60 = d[0] >= 0.03 and d[1] > 0.0
        md += ["", f"Mean over K5 K8 H5 H8 of the coverage gain: {cs.fmt(*d)} ({len(ks)} seeds)."]
    else:
        h61 = h62 = h63 = False
    md += ["", " ".join(f"**{n}: {'HOLDS' if v else 'FAILS'}**" for n, v in (("H60", h60), ("H61", h61), ("H62", h62), ("H63", h63))), "", f"Problems: {len(problems)}"] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "COLLAPSE13.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()
