#!/usr/bin/env python3
"""COLLAPSE-11 (report/PREREG_COLLAPSE11.md): DD 54 + 56 + 57 off and on in the collapse pipeline. Committed with the registration, before any cell.
L2 = share of workers with cep50M <= 2 m (collapse5_analyse.arm_stats). usage: collapse11_analyse.py <outdir>"""
import csv
import glob
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import collapse5_analyse as C5  # noqa: E402

SEEDS = range(1701, 1711)
CFG = {"g": ("ca_tbl_full", "static_868", "static_2p4", "static_uwb", "poucet_fusion"), "muller": ("ca_tbl_full", "static_2p4"),
       "st2": ("ca_tbl_full",), "n10": ("ca_tbl_full",), "n1": ("ca_tbl_full", "static_868", "static_2p4")}
CREW = ("g", "muller", "st2", "n10")
O = sys.argv[1]
bad = []


def stats(s, c, x, a):
    return C5.arm_stats(os.path.join(O, f"s{s}_{c}_{x}"), a)


def coverage(s, c, x):
    out = {}
    for d in C5.ok_runs(os.path.join(O, f"s{s}_{c}_{x}")):
        n = k = 0
        for f in glob.glob(d + "/trace_ca_tbl_full_seed*.csv") + glob.glob(d + "/subjtrace_ca_tbl_full_seed*.csv"):
            for r in csv.DictReader(open(f)):
                n += 1
                k += float(r["errM"]) <= 1.73 * max(float(r["traceSigma"]), 1e-9) ** 0.5
        if n:
            out[os.path.basename(d)] = k / n
    return out


def duty(s, c, x):
    v = []
    for d in C5.ok_runs(os.path.join(O, f"s{s}_{c}_{x}")):
        for a in re.findall(r"(?:DD56 device|DD60 finder) \d+ ca_tbl_full: .*audited peak ([0-9.e+-]+)", open(d + "/run.log", errors="replace").read()):
            v.append(float(a))
    return v


def pd(a, b):
    k = sorted(set(a) & set(b))
    return cs.ci([a[i] - b[i] for i in k]) if k else (float("nan"),) * 3


for s in (5, 8):
    for c in CFG:
        for x in ("off", "on"):
            for i in SEEDS:
                d = os.path.join(O, f"s{s}_{c}_{x}", f"seed{i}")
                if not os.path.exists(d + "/rc") or open(d + "/rc").read().strip() != "0":
                    bad.append(f"{d}: missing or rc not 0")
                    continue
                m = re.search(r"^source \S+ dirty (\d+)", open(d + "/run_id.txt").read(), re.M)
                if not m or m.group(1) != "0":
                    bad.append(f"{d}: not dirty 0")
                t = open(d + "/run.log", errors="replace").read()
                if not all(k in t for k in ("DD54 sparseRangeGeometry", "DD56 sharedDuty868", "DD57 peerSplitCi", "DD60 sharedDuty868Finders", "DD63 tdoaRubbleDelay", "DD62 tdoaHeightCorrect")):
                    bad.append(f"{d}: a DD54/56/57/60/62/63 line is missing")
                if bool(re.search(r"DD49 tofRubbleCorrect ranges corrected=[1-9]", t)) != (x == "on"):
                    bad.append(f"{d}: the DD49 line does not match '{x}'")
print("# COLLAPSE-11\n\n| cell | arm | L2 off | L2 on | on - off |\n|---|---|---|---|---|")
h44 = True
for s in (5, 8):
    for c, arms in CFG.items():
        for a in arms:
            off, on = ({k: v[0] for k, v in stats(s, c, x, a).items()} for x in ("off", "on"))
            p = pd(on, off)
            print(f"| s{s} {c} | {a} | {C5.mean(list(off.values())):.3f} | {C5.mean(list(on.values())):.3f} | {cs.fmt(*p)} |")
            if c in CREW and a == "ca_tbl_full":
                h44 = h44 and p[1] >= -0.05
print(f"\n**H44: {'HOLDS' if h44 else 'FAILS'}** (every crew cell: lower end of ours on - off >= -0.05)\n")
print("| cell | device peaks > 2.5 % off | on | largest off | on |\n|---|---|---|---|---|")
h45 = True
for s in (5, 8):
    for c in CFG:
        a, b = duty(s, c, "off"), duty(s, c, "on")
        h45 = h45 and bool(a) and bool(b) and max(a + b) <= 0.025
        print(f"| s{s} {c} | {sum(q > .025 for q in a)}/{len(a)} | {sum(q > .025 for q in b)}/{len(b)} | {max(a or [float('nan')]):.5f} | {max(b or [float('nan')]):.5f} |")
print(f"\n**H45: {'HOLDS' if h45 else 'FAILS'}** (off and on: every worker's and searcher's device's audited trailing-hour 868 MHz airtime <= 2.5 %)\n")
h46 = True
for s in (5, 8):
    best = max(("static_868", "static_2p4", "static_uwb"), key=lambda a: C5.mean([v[0] for v in stats(s, "g", "on", a).values()]))
    p = pd({k: v[0] for k, v in stats(s, "g", "on", "ca_tbl_full").items()}, {k: v[0] for k, v in stats(s, "g", "on", best).items()})
    h46 = h46 and p[1] >= -0.05
    print(f"s{s} g on: ours - {best} L2 {cs.fmt(*p)}")
print(f"\n**H46: {'HOLDS' if h46 else 'FAILS'}** (four stations, 20 workers: lower end of ours - best single radio >= -0.05)\n")
h47, h47b, h48 = True, True, True
print("| cell | coverage off | on | on - off |\n|---|---|---|---|")
for s in (5, 8):
    for c in CFG:
        a, b = coverage(s, c, "off"), coverage(s, c, "on")
        z = pd(b, a)
        if c in CREW:
            h47 = h47 and z[1] >= -0.02
        if (s, c) in ((5, "g"), (5, "st2")):
            h47b = h47b and z[1] > 0.0
        if c == "g":
            h48 = h48 and C5.mean(list(b.values())) >= 0.85
        print(f"| s{s} {c} | {C5.mean(list(a.values())):.3f} | {C5.mean(list(b.values())):.3f} | {cs.fmt(*pd(b, a))} |")
print(f"\n**H47: {'HOLDS' if h47 and h47b else 'FAILS'}** (coverage on - off: lower end >= -0.02 in every crew cell, > 0 at 5 storeys with four and with two stations)")
print(f"\n**H48: {'HOLDS' if h48 else 'FAILS'}** (on, four stations, 20 workers: mean coverage >= 0.85 at both storey counts)\n")
print(f"\nProblems: {len(bad)}")
print("\n".join("- " + b for b in bad[:30]))
