#!/usr/bin/env python3
"""COLLAPSE-6 analyser (report/PREREG_COLLAPSE6.md). Committed with the registration, before any cell.
usage: collapse6_analyse.py COLLAPSE6_OUT COLLAPSE5_PIPE_OUT"""
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import collapse5_analyse as C5  # noqa: E402


def cellmean(o, s, cfg, arm):
    v = C5.arm_stats(os.path.join(o, f"s{s}_{cfg}"), arm)
    return C5.mean([x[0] for x in v.values()]), C5.mean([x[1] for x in v.values()]), len(v)


def main():
    o6, o5 = sys.argv[1], sys.argv[2]
    print("## COLLAPSE-6 with the fix: COLLAPSE-5's tests re-applied (H17-H22 rules)\n")
    C5.pipe(o6)
    print("\n## H30: L2 with the fix minus L2 with the leak (paired seeds 821-825)\n")
    for s in (5, 8):
        for cfg, arms in (("g", ("ca_tbl_full", "static_868", "static_2p4", "static_uwb", "poucet_fusion")),
                          ("rubble", ("ca_tbl_full",)), ("muller", ("ca_tbl_full", "static_2p4")),
                          ("st2", ("ca_tbl_full",)), ("n10", ("ca_tbl_full",))):
            for a in arms:
                f, l = cellmean(o6, s, cfg, a), cellmean(o5, s, cfg, a)
                print(f"s{s} {cfg:6} {a:14} fix {f[0]:.2f} leak {l[0]:.2f} diff {f[0] - l[0]:+.2f}")
    print("\n## H31: lone worker (N = 1)\n")
    for s in (5, 8):
        ours = cellmean(o6, s, "n1", "ca_tbl_full")
        best = max(cellmean(o6, s, "n1", a)[0] for a in ("static_868", "static_2p4"))
        print(f"H31 s{s}: lone worker L2 {ours[0]:.2f} vs best single {best:.2f} "
              f"{'HOLDS' if ours[0] >= best - 0.10 else 'FAILS'} (J {ours[1]:.1f}, seeds {ours[2]})")


if __name__ == "__main__":
    main()
