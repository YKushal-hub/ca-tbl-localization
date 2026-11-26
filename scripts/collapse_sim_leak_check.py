#!/usr/bin/env python3
"""G183 (plan "No-leak check"): the system-side functions the collapse extension added to the simulator must not
name any World identifier. usage: collapse_sim_leak_check.py [SIM_SOURCE]"""
import re
import sys

FUNCS = [r"static void CollapseSolveDepth\(", r"static std::set<size_t> RelayChainKNearest\(",
         r"auto tofRubbleFix = \[", r"HorizontalRange\(double r, double anchorZ, double nodeZ, uint32_t anchorId\)\n\{"]
WORLD = ["g_collapse.", "CollapseBuriedPos", "CollapseStandZ", "collapseWorld", "CaTblRubbleWorld", "truth"]


def body(src, pat):
    m = re.search(pat, src)
    if not m:
        return None
    i = src.index("{", m.start())
    depth = 0
    for j in range(i, len(src)):
        depth += {"{": 1, "}": -1}.get(src[j], 0)
        if depth == 0:
            return src[i:j + 1]
    return None


def main():
    path = sys.argv[1] if len(sys.argv) > 1 else "scratch/ca_tbl_causal_sim.cc"
    src = open(path).read()
    fails = []
    for f in FUNCS:
        b = body(src, f)
        if b is None:
            fails.append(f"function not found: {f}")
            continue
        code = "\n".join(l.split("//")[0] for l in b.splitlines())
        for w in WORLD:
            if w in code:
                fails.append(f"{f}: names World identifier {w!r}")
    if fails:
        print("G183 FAIL")
        for x in fails:
            print("  " + x)
        return 1
    print(f"G183 PASS ({len(FUNCS)} system-side functions name no World identifier)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
