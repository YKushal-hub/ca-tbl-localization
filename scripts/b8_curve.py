#!/usr/bin/env python3
"""B8 delivery-curve diagnosis (report/B8_DELIVERY_CURVE.md). usage: b8_curve.py <dir with s*/probe_terms.csv>"""
import collections, csv, glob, math, os, sys
T = ("pathloss", "shadowing", "fading", "nlos", "foliage", "terrain", "atmospheric", "interference", "hwtx", "frontend")
Phi = lambda x: 0.5 * (1 + math.erf(x / math.sqrt(2)))
acc = collections.defaultdict(list)
for p in sorted(glob.glob(os.path.join(sys.argv[1], "s*", "probe_terms.csv"))):
    for r in csv.DictReader(open(p)):
        acc[(r["tier"], float(r["distance_m"]))].append(r)
print("| tier | d (m) | delivered | P(margin > 0) | Phi(M/sd_margin) | Phi(M/sd_shadowing) | sd_margin | sd_shadowing | sd_nlos | sd_foliage | sd_fading | NLOS share of variance |")
print("|---|---|---|---|---|---|---|---|---|---|---|---|")
for k in sorted(acc):
    rs = acc[k]; f = lambda c: sum(float(r[c]) for r in rs) / len(rs)
    if not 0.01 < f("pdr") < 0.995:
        continue
    M, sd = f("mean_margin"), f("sd_margin"); q = sum(f("sd_" + c) ** 2 for c in T)
    print(f"| {k[0]} | {k[1]:g} | {f('pdr'):.3f} | {f('p_margin_pos'):.3f} | {Phi(M / sd):.3f} | {Phi(M / f('sd_shadowing')):.3f} | "
          f"{sd:.1f} | {f('sd_shadowing'):.1f} | {f('sd_nlos'):.1f} | {f('sd_foliage'):.1f} | {f('sd_fading'):.1f} | {f('sd_nlos') ** 2 / q:.2f} |")
