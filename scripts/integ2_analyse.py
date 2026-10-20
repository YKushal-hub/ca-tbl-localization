#!/usr/bin/env python3
"""INTEG2 analysis (report/PREREG_INTEG2.md). usage: integ2_analyse.py fit <outdir> | eval <outdir>
fit: FIXES-N26's procedure, unchanged (scripts/fixes_n26_analyse.py fit). eval: CAMP-SPEC's fault table on base/ and
fit/, then I1-I3."""
import json, os, re, sys
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import fixes_n26_analyse as fx  # noqa: E402
import camp_spec_analyse as cs  # noqa: E402
import numpy as np  # noqa: E402


def table(out):
    md = []; cs.faults(out, md); rows = {}
    for l in md:
        m = re.match(r"\| (\w+) \| downweight \| ca_tbl_full \| ([\d.na]+) \| ([\d.na]+) \|", l)
        if m:
            rows[m.group(1)] = (float(m.group(2)), float(m.group(3)))
    return md, rows


def main():
    mode, out = sys.argv[1], sys.argv[2]
    if mode == "fit":
        return fx.fit(out)
    chi = json.load(open(os.path.join(out, "fit.json")))["integrityChi2"]
    mb, rb = table(os.path.join(out, "base")); mf, rf = table(os.path.join(out, "fit"))
    pb = np.nanmean([v[0] for v in rb.values()]); pf = np.nanmean([v[0] for v in rf.values()])
    cb = np.nanmean([v[1] for v in rb.values()]); cf = np.nanmean([v[1] for v in rf.values()])
    v = {1: chi < 1992.9, 2: cf > cb, 3: pf >= pb - 0.05}
    L = ["# INTEG2 results (exploratory)", "", f"Fitted integrityChi2 = {chi} (preset 1992.9).", "", "## base (1992.9)", ""] + mb + \
        ["## fit", ""] + mf + ["", f"mean over faults, ca_tbl_full: precision {pb:.3f} -> {pf:.3f}; recall {cb:.3f} -> {cf:.3f}", "", "## Predictions", ""]
    L += [f"- **I{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    L += ["", "Registered recommendation (I1-I3 all hold): " + ("RECOMMEND the fit (owner adopts)" if all(v.values()) else "do not recommend")]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "INTEG2.md"), "w").write(txt); print(txt)
    return 0


if __name__ == "__main__":
    sys.exit(main())
