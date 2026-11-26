#!/usr/bin/env python3
"""REALISTIC-5 (report/PREREG_REALISTIC5.md). Committed with the registration, before any cell.

Part E: E_<SITE>_<V>/s<seed>, SITE in ROAD1, ROAD3, CLUS3, RAND3, ROAD3F4, seeds 561-580. Part Q (8 h, no incident, no
searchers): Q_<SITE>_<V>/s<seed>, SITE in ROAD1, ROAD3, CLUS3, RAND3, seeds 581-590. V: ARMS (ca_tbl_full,
fixed_threshold, static_868, static_2p4, poucet_fusion), TII2. Every run with --reacquireSigmaM=500 and REALISTIC-4's
flags. Measures from scripts/realistic_analyse.py ("never" = 4200 s), paired 95 % bootstrap (camp_spec_analyse.ci).

  G1  every E cell: ours - 2.4 only treach, paired upper bound < 120 s; and lower bound < -300 s in at least one cell
  G2  every E cell: ours - the better single-radio design t25, paired upper bound < 120 s
  G3  every E cell: ours - the better single-radio design A250, paired lower bound >= -0.02
  G4  every Q cell: ours - the better single-radio design A250, paired lower bound >= -0.02
  G5  (reported) J/h of every design, E and Q; TII-2 - ours J/h

usage: realistic5_analyse.py <outdir>
"""
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import realistic_analyse as ra  # noqa: E402

E_SITES, Q_SITES = ["ROAD1", "ROAD3", "CLUS3", "RAND3", "ROAD3F4"], ["ROAD1", "ROAD3", "CLUS3", "RAND3"]
E_SEEDS, Q_SEEDS = list(range(561, 581)), list(range(581, 591))
DESIGNS = [("ARMS", "ca_tbl_full"), ("ARMS", "fixed_threshold"), ("ARMS", "static_868"), ("ARMS", "static_2p4"),
           ("ARMS", "poucet_fusion"), ("TII2", "fixed_threshold")]
LAB = {("ARMS", "ca_tbl_full"): "ours", ("ARMS", "fixed_threshold"): "threshold", ("ARMS", "static_868"): "868 only",
       ("ARMS", "static_2p4"): "2.4 only", ("ARMS", "poucet_fusion"): "all on", ("TII2", "fixed_threshold"): "TII-2"}
CENS = 4200.0
SINGLE = ("868 only", "2.4 only")


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources, D = [], set(), {}
    plan = [("E", st, s) for st in E_SITES for s in E_SEEDS] + [("Q", st, s) for st in Q_SITES for s in Q_SEEDS]
    for part, st, s in plan:
        for v, arm in DESIGNS:
            key = (part, st, LAB[(v, arm)])
            D.setdefault(key, {})
            d = os.path.join(out, f"{part}_{st}_{v}", f"s{s}")
            if not cs.ok(d):
                problems.append(f"{d}: missing or rc not 0")
                continue
            m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
            if not m or m.group(2) != "0":
                problems.append(f"{d}: not dirty 0")
            else:
                sources.add(m.group(1))
            if "DD34 reacquireSigmaM" not in open(os.path.join(d, "run.log"), errors="replace").read():
                problems.append(f"{d}: DD34 not announced")
            try:
                r = dict(ra.quiet(d, arm))
                if part == "E":
                    r.update(ra.emergency(d, arm))
                    if not abs(r["alarm_s"] - 3000.0) < 3.0:
                        problems.append(f"{d} {arm}: alarm at {r['alarm_s']}")
                    for k in ("t25_s", "t5_s", "treach_s"):
                        r[k + "_never"] = math.isnan(r[k])
                        r[k] = CENS if math.isnan(r[k]) else r[k]
            except Exception as e:  # noqa: BLE001
                problems.append(f"{d} {arm}: {e}")
                continue
            D[key][s] = r
    for key, x in D.items():
        n = len(E_SEEDS if key[0] == "E" else Q_SEEDS)
        if len(x) != n:
            problems.append(f"{key}: {len(x)} seeds, not {n}")
    if len(sources) > 1:
        problems.append(f"more than one source: {sorted(sources)}")

    def mean(key, k):
        x = D.get(key, {})
        return float(np.mean([r[k] for r in x.values()])) if x else float("nan")

    def never(key, k):
        x = D.get(key, {})
        return float(np.mean([r[k + "_never"] for r in x.values()])) if x else float("nan")

    def pr(a, b, k):
        x, y = D.get(a, {}), D.get(b, {})
        s = sorted(set(x) & set(y))
        return cs.ci([x[i][k] - y[i][k] for i in s]) if s else (float("nan"),) * 3

    md = ["# REALISTIC-5 (scripts/realistic5_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    for st in E_SITES:
        md += [f"## E {st}", "", "| design | t25 (s) [never] | t5 (s) [never] | treach (s) [never] | A250 | J/h |", "|---|---|---|---|---|---|"]
        for v, arm in DESIGNS:
            k = ("E", st, LAB[(v, arm)])
            md.append(f"| {k[2]} | {mean(k, 't25_s'):.0f} [{never(k, 't25_s'):.2f}] | {mean(k, 't5_s'):.0f} [{never(k, 't5_s'):.2f}] | "
                      f"{mean(k, 'treach_s'):.0f} [{never(k, 'treach_s'):.2f}] | {mean(k, 'A250_five'):.3f} | {mean(k, 'subjJ_per_h'):.1f} |")
        md.append("")
    g1a, g1b, g2, g3 = True, False, True, True
    md += ["## G1-G3", "", "| cell | ours - 2.4 only treach (s) | ours - better single t25 (s) | ours - better single A250 |", "|---|---|---|---|"]
    for st in E_SITES:
        d1 = pr(("E", st, "ours"), ("E", st, "2.4 only"), "treach_s")
        b2 = min(SINGLE, key=lambda lab: mean(("E", st, lab), "t25_s"))
        d2 = pr(("E", st, "ours"), ("E", st, b2), "t25_s")
        b3 = max(SINGLE, key=lambda lab: mean(("E", st, lab), "A250_five"))
        d3 = pr(("E", st, "ours"), ("E", st, b3), "A250_five")
        g1a = g1a and d1[2] < 120.0
        g1b = g1b or d1[2] < -300.0
        g2 = g2 and d2[2] < 120.0
        g3 = g3 and d3[1] >= -0.02
        md.append(f"| {st} | {cs.fmt(*d1, p=0)} | {cs.fmt(*d2, p=0)} ({b2}) | {cs.fmt(*d3)} ({b3}) |")
    md += ["", f"**G1: {'HOLDS' if g1a and g1b else 'FAILS'}** (non-inferior: {g1a}; clearly better somewhere: {g1b})  "
               f"**G2: {'HOLDS' if g2 else 'FAILS'}**  **G3: {'HOLDS' if g3 else 'FAILS'}**", ""]
    g4 = True
    for st in Q_SITES:
        md += [f"## Q {st} (8 h)", "", "| design | A25 | A250 | J/h | ours - design J/h |", "|---|---|---|---|---|"]
        for v, arm in DESIGNS:
            k = ("Q", st, LAB[(v, arm)])
            md.append(f"| {k[2]} | {mean(k, 'A25_five'):.3f} | {mean(k, 'A250_five'):.3f} | {mean(k, 'subjJ_per_h'):.1f} | "
                      f"{cs.fmt(*pr(('Q', st, 'ours'), k, 'subjJ_per_h'), p=1)} |")
        b = max(SINGLE, key=lambda lab: mean(("Q", st, lab), "A250_five"))
        d = pr(("Q", st, "ours"), ("Q", st, b), "A250_five")
        g4 = g4 and d[1] >= -0.02
        md += ["", f"ours - {b} A250: {cs.fmt(*d)}; TII-2 - ours J/h: {cs.fmt(*pr(('Q', st, 'TII-2'), ('Q', st, 'ours'), 'subjJ_per_h'), p=1)}", ""]
    md += [f"**G4: {'HOLDS' if g4 else 'FAILS'}**", "", f"Problems: {len(problems)}" + ("" if problems else " (none)")]
    md += [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REALISTIC5.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
