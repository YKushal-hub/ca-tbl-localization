#!/usr/bin/env python3
"""Post-hoc diagnosis of ca_tbl_full against static_2p4 in ACC-A's traces
(report/ACC_A_STATIC2P4_DIAGNOSIS.md): where each arm's fix rate and within-25 m share come from.
Reads only; exploratory, not registered.

usage: acc_a_static_diag.py <outdir written by scripts/acc_a_run.sh>
"""
import csv
import os
import statistics as st
import sys
from collections import defaultdict

ARMS = ("ca_tbl_full", "static_2p4")
CELLS = ("M_A_k10", "M_B_k10")
CYCLE_S = 2.5
GAPS = ((0, 60, "< 60 s"), (60, 300, "60-300 s"), (300, 900, "300-900 s"), (900, 1e9, "> 900 s"))


def rows(p):
    with open(p) as fh:
        return list(csv.DictReader(fh))


def q(v, p):
    v = sorted(v)
    return v[int(p * (len(v) - 1))] if v else float("nan")


def share(v, f):
    return sum(1 for x in v if f(x)) / len(v) if v else float("nan")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    for cell in CELLS:
        print(f"## {cell} (seeds 1-20)")
        for arm in ARMS:
            t0 = defaultdict(list)
            sb = defaultdict(list)
            for s in range(1, 21):
                d = os.path.join(out, cell, f"s{s}")
                tr = rows(os.path.join(d, f"trace_{arm}_seed{s}.csv"))
                e = [float(r["errM"]) for r in tr]
                t0["e0"].append(e[0])
                t0["fix"].append(share(tr, lambda r: r["hadFix"] == "1"))
                t0["w25"].append(share(e, lambda x: x <= 25.0))
                for k in range(2, 6):
                    sr = rows(os.path.join(d, f"subjtrace_{arm}_seed{s}_subj{k}.csv"))
                    se = [float(r["errM"]) for r in sr]
                    sb["e0"].append(se[0])
                    sb["fix"].append(share(sr, lambda r: r["hadFix"] == "1"))
                    sb["w25"].append(share(se, lambda x: x <= 25.0))
                    first = next((i for i, r in enumerate(sr) if r["hadFix"] == "1"), None)
                    if first is None:
                        sb["never"].append(1)
                    else:
                        sb["first_s"].append(first * CYCLE_S)
                    last = None
                    for i, r in enumerate(sr):
                        if r["hadFix"] == "1":
                            if i > 0:
                                sb["dfix_" + r["fixTier"]].append(se[i] - se[i - 1])
                            last = i
                        elif last is not None:
                            g = (i - last) * CYCLE_S
                            lab = next(lb for lo, hi, lb in GAPS if lo <= g < hi)
                            sb["coast_" + lab].append(se[i])
            n = len(sb["e0"])
            print(f"### {arm}")
            print(f"- tag 0: initial error median {st.median(t0['e0']):.1f} m; fix share {st.mean(t0['fix']):.3f}; "
                  f"within 25 m {st.mean(t0['w25']):.3f}")
            print(f"- subjects 2-5: initial error median {st.median(sb['e0']):.1f} m; fix share {st.mean(sb['fix']):.3f}; "
                  f"within 25 m {st.mean(sb['w25']):.3f}")
            print(f"- subjects' first fix: median {st.median(sb['first_s']):.1f} s, p90 {q(sb['first_s'], 0.9):.1f} s; "
                  f"no fix in the run: {len(sb['never'])} of {n}")
            for t in ("868MHz", "2.4GHz", "UWB"):
                v = sb["dfix_" + t]
                if v:
                    print(f"- subject fix solved on {t}: n {len(v)}, error change median {st.median(v):+.2f} m, "
                          f"worse after the fix {100 * share(v, lambda x: x > 0):.0f} %")
            for _, _, lab in GAPS:
                v = sb["coast_" + lab]
                if v:
                    print(f"- subjects coasting {lab} since the last fix: n {len(v)}, median error "
                          f"{st.median(v):.1f} m, within 25 m {share(v, lambda x: x <= 25.0):.2f}")
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
