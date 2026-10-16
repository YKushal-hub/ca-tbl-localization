#!/usr/bin/env python3
"""R1-REINIT analysis (report/PREREG_R1_REINIT.md).

usage: r1_reinit_analyse.py <outdir>
Writes <outdir>/R1.md, prints it, exits 1 if a registered abort condition holds.
"""
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import s2c_analyse as s2  # noqa: E402

ARMS = ("ca_tbl_full", "fixed_threshold", "static_2p4")
KEYS = ("w25", "w25acq", "med", "first", "never", "c95", "av", "J")


def fmt(iv):
    m, lo, hi = iv
    return f"{m:+.3f} [{lo:+.3f}, {hi:+.3f}]"


def main():
    out = sys.argv[1]
    problems, lines = [], ["# R1-REINIT results (exploratory)", ""]
    data = {}
    for cell in ("U0_A", "U0_B", "U1_A", "U1_B"):
        dirs = cs.seeds(out, cell)
        if len(dirs) < 20:
            problems.append(f"{cell}: {len(dirs)} seeds")
        for d in dirs:
            if not cs.ok(d):
                problems.append(f"{d}: not ok")
        for arm in ARMS:
            data[(cell, arm)] = s2.collect(out, cell, arm)
    verdict = {}
    for cfg in ("A", "B"):
        lines += [f"## Config {cfg}: U1 − U0, paired over seeds", "", "| arm | " + " | ".join(KEYS) + " | n |",
                  "|---|" + "---|" * (len(KEYS) + 1)]
        for arm in ARMS:
            a, b = data[(f"U1_{cfg}", arm)], data[(f"U0_{cfg}", arm)]
            row = []
            n = 0
            for k in KEYS:
                iv, n = s2.paired(a, b, k)
                row.append(fmt(iv))
                verdict[(cfg, arm, k)] = (iv, n)
            lines.append(f"| {arm} | " + " | ".join(row) + f" | {n} |")
        lines.append("")
    def above0(iv): return not math.isnan(iv[1]) and iv[1] > 0
    def below0(iv): return not math.isnan(iv[2]) and iv[2] < 0
    def ev(cfg, arm, k):
        iv, n = verdict[(cfg, arm, k)]
        return iv, n >= 20
    pred = []
    iv, okn = ev("A", "ca_tbl_full", "w25"); pred.append(("R1a", okn, above0(iv)))
    iv, okn = ev("A", "static_2p4", "w25"); pred.append(("R1b", okn, above0(iv)))
    r = [ev("A", a, "med") for a in ARMS]; pred.append(("R1c", all(x[1] for x in r), all(below0(x[0]) for x in r)))
    r = [ev(c, a, "first") for c in ("A", "B") for a in ARMS]; pred.append(("R1d", all(x[1] for x in r), all(s2.inc0(x[0]) for x in r)))
    r = [ev("B", a, "w25") for a in ARMS]; pred.append(("R1e", all(x[1] for x in r), all((not math.isnan(x[0][1])) and x[0][1] > -0.05 for x in r)))
    iv, n = s2.paired(data[("U1_A", "ca_tbl_full")], data[("U1_A", "fixed_threshold")], "w25")
    pred.append(("R1f", n >= 20, s2.inc0(iv)))
    lines += ["## Predictions", ""] + [f"- **{p}**: " + ("not evaluable" if not okn else ("held" if held else "not held")) for p, okn, held in pred]
    lines += ["", f"R1f interval (`ca_tbl_full` − `fixed_threshold`, U1_A, w25): {fmt(iv)}"]
    if problems:
        lines += ["", "## Abort conditions", ""] + [f"- {p}" for p in problems]
    txt = "\n".join(lines) + "\n"
    open(os.path.join(out, "R1.md"), "w").write(txt)
    print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
