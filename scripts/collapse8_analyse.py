#!/usr/bin/env python3
"""COLLAPSE-8 analyser (report/PREREG_COLLAPSE8.md). Committed with the registration, before any cell.
usage: collapse8_analyse.py OUT"""
import os
import statistics as st
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import collapse5_analyse as C5  # noqa: E402

CELL_J, COMPUTE_JH = 16740.0, 1.82  # BATT-CR123A; Phase 0 compute (as COLLAPSE-3/4)


def cell(o, st_, br, n, cfg, arm):
    v = C5.arm_stats(os.path.join(o, f"s{st_}_{br}_n{n}_{cfg}"), arm)
    L2 = C5.mean([x[0] for x in v.values()])
    J = [x[1] for x in v.values()]
    hours = st.median(CELL_J / (j * 2.0 + COMPUTE_JH) for j in J) if J else float("nan")  # 1800 s run: J/h = 2 J
    return L2, hours, len(v)


def main():
    o = sys.argv[1]
    res = {"H33": [], "H34": [], "H35": [], "H36": []}
    for st_ in (5, 8):
        for br in ("low", "central"):
            lone = cell(o, st_, br, 1, "lone", "ca_tbl_full")
            base = cell(o, st_, br, 1, "base", "ca_tbl_full")
            s868 = cell(o, st_, br, 1, "lone", "static_868")
            c_l = cell(o, st_, br, 20, "lone", "ca_tbl_full")
            c_b = cell(o, st_, br, 20, "base", "ca_tbl_full")
            print(f"s{st_} {br:7}: N=1 lone {lone[0]:.2f} ({lone[1]:.0f} h) base {base[0]:.2f} ({base[1]:.0f} h) "
                  f"868 alone {s868[0]:.2f}; N=20 lone {c_l[0]:.2f} base {c_b[0]:.2f}  seeds {lone[2]}/{base[2]}/{c_l[2]}/{c_b[2]}")
            res["H33"].append(lone[0] >= s868[0] - 0.10)
            res["H34"].append(lone[0] >= base[0])
            res["H35"].append(abs(c_l[0] - c_b[0]) <= 0.05)
            res["H36"].append(lone[1] >= 72.0)
    for h, v in res.items():
        print(f"{h}: {sum(v)}/{len(v)} {'HOLDS' if all(v) else 'FAILS'}")


if __name__ == "__main__":
    main()
