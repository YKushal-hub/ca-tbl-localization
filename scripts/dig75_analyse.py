#!/usr/bin/env python3
"""DIG-75 (report/PREREG_DIG75.md): digging order on COLLAPSE-13's traces, DD 75 off/on. usage: dig75_analyse.py <collapse13 outdir>"""
import hashlib, importlib.util, os, sys
import numpy as np
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
spec = importlib.util.spec_from_file_location("score_dig", os.path.join(ROOT, "scripts", "collapse", "score_dig.py"))
sd = importlib.util.module_from_spec(spec); spec.loader.exec_module(sd)
CELLS, SEEDS = ("K5", "K8", "H5", "H8", "K2"), range(2101, 2111)
GRID = [(t, d, r) for t in (1, 2) for d in (0.5, 2.0, 8.0) for r in (1.0, 2.0)]


def main():
    out = sys.argv[1]; problems = []
    man = {}
    for l in open(os.path.join(ROOT, "results", "collapse13_2026-11-26", "RAW_MANIFEST.sha256")):
        h, p = l.split(None, 1); man[p.strip()] = h
    A = {}  # (cell, x, seed, combo, policy) -> alive
    for c in CELLS:
        for x in ("off", "on"):
            for s in SEEDS:
                d = os.path.join(out, f"{c}_{x}", f"seed{s}")
                if not os.path.exists(d + "/rc") or open(d + "/rc").read().strip() != "0":
                    problems.append(f"{d}: rc"); continue
                for f in os.listdir(d):
                    if "trace_ca_tbl_full" in f:
                        rel = f"{c}_{x}/seed{s}/{f}"
                        if man.get(rel) != hashlib.sha256(open(os.path.join(d, f), "rb").read()).hexdigest():
                            problems.append(f"{rel}: hash")
                W = sd.load(d, "ca_tbl_full", "END")
                if len(W) != 20:
                    problems.append(f"{d}: {len(W)} workers")
                start = max(w["t"] for w in W.values()) / 3600.0
                for k, (t, dg, r) in enumerate(GRID):
                    P = sd.policies(W, dg, r, (0.0, 0.0))
                    for pol in ("id", "spt", "oracle"):
                        A[(c, x, s, k, pol)] = sd.schedule(P[pol], W, t, dg, r, start)[0]
    if problems:
        print("INCOMPLETE:", problems[:10]); return 1
    m = lambda c, x, k, p: float(np.mean([A[(c, x, s, k, p)] for s in SEEDS]))
    print("# DIG-75 (scripts/dig75_analyse.py)\n")
    print("| cell | teams | t_dig h | r m | id | spt off | spt on | oracle | spt on - id | spt on - off |")
    print("|---|---|---|---|---|---|---|---|---|---|")
    d2, d3 = [], []
    for c in CELLS:
        for k, (t, dg, r) in enumerate(GRID):
            i, so, sn, o = m(c, "on", k, "id"), m(c, "off", k, "spt"), m(c, "on", k, "spt"), m(c, "on", k, "oracle")
            d2.append(sn - i); d3.append(sn - so)
            print(f"| {c} | {t} | {dg} | {r} | {i:.2f} | {so:.2f} | {sn:.2f} | {o:.2f} | {sn - i:+.2f} | {sn - so:+.2f} |")
    k0 = GRID.index((2, 2.0, 2.0))
    d1 = {c: m(c, "on", k0, "spt") - m(c, "on", k0, "id") for c in CELLS}
    D1 = all(v >= 0 for v in d1.values())
    share = float(np.mean([v >= 0 for v in d2])); D2 = share >= 0.80
    D3 = float(np.mean(d3)) >= -0.10
    w = int(np.argmin(d3))
    print(f"\nD1 (teams 2, t_dig 2 h, r 2 m) spt - id with DD 75: " + ", ".join(f"{c} {v:+.2f}" for c, v in d1.items()))
    print(f"D2 spt >= id in {sum(v >= 0 for v in d2)} of {len(d2)} ({share:.2f}); largest gain {max(d2):+.2f}")
    print(f"D3 spt on - off: mean {np.mean(d3):+.3f}, worst {d3[w]:+.2f} ({CELLS[w // 12]} teams {GRID[w % 12][0]} t_dig {GRID[w % 12][1]} r {GRID[w % 12][2]}), best {max(d3):+.2f}")
    print(f"\n**D1: {'HOLDS' if D1 else 'FAILS'}**  **D2: {'HOLDS' if D2 else 'FAILS'}**  **D3: {'HOLDS' if D3 else 'FAILS'}**\n\nProblems: 0")
    return 0


if __name__ == "__main__":
    sys.exit(main())
