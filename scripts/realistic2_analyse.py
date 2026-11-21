#!/usr/bin/env python3
"""REALISTIC-2 (report/PREREG_REALISTIC2.md). Committed with the registration, before any cell.

Part E cells E_<SITE>_<V>/s<seed>, SITE in RAND3, RAND5, ROAD5, ROAD3F4, seeds 521-540; Part Q cells Q_<SITE>_<V>/s<seed>,
SITE in ROAD1, ROAD3, CLUS3, RAND3, seeds 541-550 (8 h, no incident, no searchers). V in ARMS (ca_tbl_full,
fixed_threshold, static_868, static_2p4, poucet_fusion), TII2 (fixed_threshold under the TII-2 rule). Measures from
scripts/realistic_analyse.py, "never" counted as 4200 s, paired 95 % bootstrap (camp_spec_analyse.ci).

  E1  every E cell: ours - better single-radio (by mean) t25, paired upper bound < 120 s
  E2  every E cell: ours - better single-radio (by mean) A250, paired lower bound >= -0.02
  E3  ROAD3F4: ours - 2.4 only treach, paired upper bound < 120 s
  Q1  every Q cell: ours - better single-radio A250, paired lower bound >= -0.02; J/h reported against every design
  Q2  (reported) TII-2 - ours J/h per Q site

usage: realistic2_analyse.py <outdir>
"""
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import realistic_analyse as ra  # noqa: E402

ESEEDS, QSEEDS = list(range(521, 541)), list(range(541, 551))
ESITES, QSITES = ["RAND3", "RAND5", "ROAD5", "ROAD3F4"], ["ROAD1", "ROAD3", "CLUS3", "RAND3"]
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
    cells = [("E", st, v, arm, s) for st in ESITES for v, arm in DESIGNS for s in ESEEDS]
    cells += [("Q", st, v, arm, s) for st in QSITES for v, arm in DESIGNS for s in QSEEDS]
    for part, st, v, arm, s in cells:
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
        try:
            r = dict(ra.quiet(d, arm))
            if part == "E":
                r.update(ra.emergency(d, arm))
                if not abs(r["alarm_s"] - 3000.0) < 3.0:
                    problems.append(f"{d} {arm}: alarm at {r['alarm_s']}, not 3000 s")
                for k in ("t25_s", "t5_s", "treach_s"):
                    r[k + "_never"] = math.isnan(r[k])
                    r[k] = CENS if math.isnan(r[k]) else r[k]
        except Exception as e:  # noqa: BLE001
            problems.append(f"{d} {arm}: {e}")
            continue
        D[key][s] = r
    for key, x in D.items():
        n = len(ESEEDS if key[0] == "E" else QSEEDS)
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

    md = ["# REALISTIC-2 (scripts/realistic2_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    for st in ESITES:
        md += [f"## E {st}", "", "| design | t to 25 m (s) [never] | t to 5 m (s) [never] | searcher within 10 m (s) [never] | A250 | J/h |",
               "|---|---|---|---|---|---|"]
        for v, arm in DESIGNS:
            k = ("E", st, LAB[(v, arm)])
            md.append(f"| {LAB[(v, arm)]} | {mean(k, 't25_s'):.0f} [{never(k, 't25_s'):.2f}] | {mean(k, 't5_s'):.0f} [{never(k, 't5_s'):.2f}] | "
                      f"{mean(k, 'treach_s'):.0f} [{never(k, 'treach_s'):.2f}] | {mean(k, 'A250_five'):.3f} | {mean(k, 'subjJ_per_h'):.1f} |")
        md.append("")
    e1 = e2 = True
    md += ["## E1 / E2", "", "| cell | ours - better single, t25 (s) | ours - better single, A250 |", "|---|---|---|"]
    for st in ESITES:
        b1 = min(SINGLE, key=lambda lab: mean(("E", st, lab), "t25_s"))
        b2 = max(SINGLE, key=lambda lab: mean(("E", st, lab), "A250_five"))
        d1, d2 = pr(("E", st, "ours"), ("E", st, b1), "t25_s"), pr(("E", st, "ours"), ("E", st, b2), "A250_five")
        e1 = e1 and d1[2] < 120.0
        e2 = e2 and d2[1] >= -0.02
        md.append(f"| {st} | {cs.fmt(*d1, p=0)} ({b1}) | {cs.fmt(*d2)} ({b2}) |")
    d3 = pr(("E", "ROAD3F4", "ours"), ("E", "ROAD3F4", "2.4 only"), "treach_s")
    e3 = d3[2] < 120.0
    md += ["", f"**E1: {'HOLDS' if e1 else 'FAILS'}**  **E2: {'HOLDS' if e2 else 'FAILS'}**", "",
           f"## E3 (ROAD3F4, ours - 2.4 only, searcher within 10 m): {cs.fmt(*d3, p=0)} s -> **{'HOLDS' if e3 else 'FAILS'}**", ""]
    q1 = True
    for st in QSITES:
        md += [f"## Q {st} (8 h)", "", "| design | A25 | A250 | J/h | ours - design J/h |", "|---|---|---|---|---|"]
        for v, arm in DESIGNS:
            k = ("Q", st, LAB[(v, arm)])
            md.append(f"| {LAB[(v, arm)]} | {mean(k, 'A25_five'):.3f} | {mean(k, 'A250_five'):.3f} | {mean(k, 'subjJ_per_h'):.1f} | "
                      f"{cs.fmt(*pr(('Q', st, 'ours'), k, 'subjJ_per_h'), p=1)} |")
        b = max(SINGLE, key=lambda lab: mean(("Q", st, lab), "A250_five"))
        d = pr(("Q", st, "ours"), ("Q", st, b), "A250_five")
        q1 = q1 and d[1] >= -0.02
        md += ["", f"ours - {b} A250: {cs.fmt(*d)}; TII-2 - ours J/h: {cs.fmt(*pr(('Q', st, 'TII-2'), ('Q', st, 'ours'), 'subjJ_per_h'), p=1)}", ""]
    md += [f"**Q1: {'HOLDS' if q1 else 'FAILS'}**", "", f"Problems: {len(problems)}" + ("" if problems else " (none)")]
    md += [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REALISTIC2.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
