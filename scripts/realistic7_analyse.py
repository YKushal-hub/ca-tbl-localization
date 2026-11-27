#!/usr/bin/env python3
"""REALISTIC-7 (report/PREREG_REALISTIC7.md): DD 54 --sparseRangeGeometry. Committed with the registration, before any cell.

REALISTIC-6's cells on seeds 621-640 with the flag on (E_*), the five arms of the main cells with it off (OFF_*), and the
8 h quiet shift (seeds 641-650) on (Q_*) and off (QOFF_*, five arms). Measures: realistic_analyse.py; crew coverage = share of
crew cycles with errM <= 1.73 sqrt(traceSigma). Paired 95 % bootstrap over seeds (camp_spec_analyse.ci), "never" = 4200 s.

  H1-H3  REALISTIC-6's rules, every design with the flag on
  K1  CLUS3: ours, on - off, crew coverage: paired lower bound > 0.10
  K2  every other main cell: ours, on - off, crew coverage: paired lower bound >= -0.05
  K3  every main cell: ours, on - off, A(25 m) over the crew: paired lower bound >= -0.02
  K4  every main cell: ours, on - off, J/h: paired upper bound <= 1.0
  T1  (reported) ours - flat threshold (both on): treach, t25, A25, A250, J/h, coverage
  Q1  (reported) quiet shift: ours - better single A250 (on), and on - off for coverage, A25, A250

usage: realistic7_analyse.py <outdir>
"""
import csv
import glob
import math
import os
import re
import sys

import numpy as np

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import realistic_analyse as ra  # noqa: E402

E_SITES, Q_SITES = ["ROAD1", "ROAD3", "CLUS3", "RAND3", "ROAD3F4"], []
X_SITES = ["CREW10", "CREW15", "ROAD3MUL", "CLUS3MUL"]
E_SEEDS, Q_SEEDS = list(range(621, 641)), list(range(641, 651))
DESIGNS = [("ARMS", "ca_tbl_full"), ("ARMS", "fixed_threshold"), ("ARMS", "static_868"), ("ARMS", "static_2p4"),
           ("ARMS", "poucet_fusion"), ("TII2", "fixed_threshold")]
LAB = {("ARMS", "ca_tbl_full"): "ours", ("ARMS", "fixed_threshold"): "threshold", ("ARMS", "static_868"): "868 only",
       ("ARMS", "static_2p4"): "2.4 only", ("ARMS", "poucet_fusion"): "all on", ("TII2", "fixed_threshold"): "TII-2"}
CENS = 4200.0
SINGLE = ("868 only", "2.4 only")


def cov(d, arm):
    seed = re.search(r"s(\d+)/?$", d).group(1)
    n = k = 0
    for f in glob.glob(os.path.join(d, f"trace_{arm}_seed{seed}.csv")) + glob.glob(os.path.join(d, f"subjtrace_{arm}_seed{seed}_subj*.csv")):
        for r in csv.DictReader(open(f)):
            n += 1
            k += float(r["errM"]) <= 1.73 * max(float(r["traceSigma"]), 1e-9) ** 0.5
    return k / max(n, 1)


def extra(out, D, problems):
    X = {}  # (part, site, design) -> seed -> dict
    Qs = ["ROAD1", "ROAD3", "CLUS3", "RAND3"]
    def load(part, st, lab, arm, v, seeds, emergency):
        key = (part, st, lab)
        X[key] = {}
        for s in seeds:
            d = os.path.join(out, f"{part}_{st}_{v}", f"s{s}")
            if not cs.ok(d):
                problems.append(f"{d}: missing or rc not 0")
                continue
            m = re.search(r"^source (\S+) dirty (\d+)", open(os.path.join(d, "run_id.txt")).read(), re.M)
            if not m or m.group(2) != "0":
                problems.append(f"{d}: not dirty 0")
            if ("DD54 sparseRangeGeometry" in open(os.path.join(d, "run.log"), errors="replace").read()) != (part in ("E", "Q")):
                problems.append(f"{d}: DD54 line does not match the cell")
            x = dict(ra.quiet(d, arm))
            if emergency:
                x.update(ra.emergency(d, arm))
                for k in ("t25_s", "treach_s"):
                    x[k] = CENS if math.isnan(x[k]) else x[k]
            x["cov"] = cov(d, arm)
            X[key][s] = x
    for st in E_SITES:
        for lab, arm in (("ours", "ca_tbl_full"), ("threshold", "fixed_threshold"), ("2.4 only", "static_2p4")):
            load("E", st, lab, arm, "ARMS", E_SEEDS, True)
        load("OFF", st, "ours", "ca_tbl_full", "ARMS", E_SEEDS, True)
    for st in Qs:
        for lab, arm in (("ours", "ca_tbl_full"), ("868 only", "static_868"), ("2.4 only", "static_2p4")):
            load("Q", st, lab, arm, "ARMS", Q_SEEDS, False)
        load("QOFF", st, "ours", "ca_tbl_full", "ARMS", Q_SEEDS, False)
    for key, x in X.items():
        want = len(Q_SEEDS if key[0].startswith("Q") else E_SEEDS)
        if len(x) != want:
            problems.append(f"{key}: {len(x)} seeds, not {want}")
    def pr(a, b, k):
        s = sorted(set(X[a]) & set(X[b]))
        return cs.ci([X[a][i][k] - X[b][i][k] for i in s]) if s else (float("nan"),) * 3
    def mean(a, k):
        return float(np.mean([r[k] for r in X[a].values()])) if X[a] else float("nan")
    md = ["", "## K (DD 54 on minus off, ours)", "", "| cell | coverage off -> on | coverage | A25 crew | A250 crew | J/h |", "|---|---|---|---|---|---|"]
    k1 = k2 = k3 = k4 = True
    for st in E_SITES:
        a, b = ("E", st, "ours"), ("OFF", st, "ours")
        c, q, h, j = (pr(a, b, k) for k in ("cov", "A25_five", "A250_five", "subjJ_per_h"))
        if st == "CLUS3":
            k1 = c[1] > 0.10
        else:
            k2 = k2 and c[1] >= -0.05
        k3 = k3 and q[1] >= -0.02
        k4 = k4 and j[2] <= 1.0
        md.append(f"| {st} | {mean(b, 'cov'):.3f} -> {mean(a, 'cov'):.3f} | {cs.fmt(*c)} | {cs.fmt(*q)} | {cs.fmt(*h)} | {cs.fmt(*j, p=1)} |")
    md += ["", *(f"**{n}: {'HOLDS' if v else 'FAILS'}**" for n, v in (("K1", k1), ("K2", k2), ("K3", k3), ("K4", k4)))]
    md += ["", "## T1 (reported): ours - flat threshold, both with the flag on", "", "| cell | treach (s) | t25 (s) | A25 | A250 | J/h | coverage |", "|---|---|---|---|---|---|---|"]
    for st in E_SITES:
        a, b = ("E", st, "ours"), ("E", st, "threshold")
        md.append("| " + st + " | " + " | ".join(cs.fmt(*pr(a, b, k), p=p) for k, p in (("treach_s", 0), ("t25_s", 0), ("A25_five", 3), ("A250_five", 3), ("subjJ_per_h", 1), ("cov", 3))) + " |")
    md += ["", "## Q1 (reported)", "", "| cell | ours - better single A250 (on) | coverage off -> on | on - off A25 | on - off A250 |", "|---|---|---|---|---|"]
    for st in Qs:
        a, b = ("Q", st, "ours"), ("QOFF", st, "ours")
        best = max(("868 only", "2.4 only"), key=lambda lab: mean(("Q", st, lab), "A250_five"))
        md.append(f"| {st} | {cs.fmt(*pr(a, ('Q', st, best), 'A250_five'))} ({best}) | {mean(b, 'cov'):.3f} -> {mean(a, 'cov'):.3f} | {cs.fmt(*pr(a, b, 'A25_five'))} | {cs.fmt(*pr(a, b, 'A250_five'))} |")
    return md


def main():
    if len(sys.argv) < 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems, sources, D = [], set(), {}
    plan = [("E", st, s) for st in E_SITES + X_SITES for s in E_SEEDS]
    for part, st, s in plan:
        for v, arm in DESIGNS:
            if st in X_SITES and v != "ARMS":
                continue
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
            if "DD35 incident" not in open(os.path.join(d, "run.log"), errors="replace").read():
                problems.append(f"{d}: DD35 not announced")
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

    md = ["# REALISTIC-7 (scripts/realistic7_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
    for st in E_SITES + X_SITES:
        md += [f"## E {st}", "", "| design | t25 (s) [never] | t5 (s) [never] | treach (s) [never] | A250 | J/h |", "|---|---|---|---|---|---|"]
        for v, arm in DESIGNS:
            k = ("E", st, LAB[(v, arm)])
            if not D.get(k):
                continue
            md.append(f"| {k[2]} | {mean(k, 't25_s'):.0f} [{never(k, 't25_s'):.2f}] | {mean(k, 't5_s'):.0f} [{never(k, 't5_s'):.2f}] | "
                      f"{mean(k, 'treach_s'):.0f} [{never(k, 'treach_s'):.2f}] | {mean(k, 'A250_five'):.3f} | {mean(k, 'subjJ_per_h'):.1f} |")
        md.append("")
    g1a, g1b, g2, g3 = True, False, True, True
    md += ["## H1-H3", "", "| cell | ours - 2.4 only treach (s) | ours - better single t25 (s) | ours - better single A250 |", "|---|---|---|---|"]
    for st in E_SITES:
        d1 = pr(("E", st, "ours"), ("E", st, "2.4 only"), "treach_s")
        b2 = min(SINGLE, key=lambda lab: mean(("E", st, lab), "t25_s"))
        d2 = pr(("E", st, "ours"), ("E", st, b2), "t25_s")
        b3 = max(SINGLE, key=lambda lab: mean(("E", st, lab), "A250_five"))
        d3 = pr(("E", st, "ours"), ("E", st, b3), "A250_five")
        g1a = g1a and d1[2] < 120.0
        g1b = g1b or d1[2] < -300.0  # H1
        g2 = g2 and d2[2] < 120.0
        g3 = g3 and d3[1] >= -0.02
        md.append(f"| {st} | {cs.fmt(*d1, p=0)} | {cs.fmt(*d2, p=0)} ({b2}) | {cs.fmt(*d3)} ({b3}) |")
    md += ["", f"**H1: {'HOLDS' if g1a and g1b else 'FAILS'}** (non-inferior: {g1a}; clearly better somewhere: {g1b})  "
               f"**H2: {'HOLDS' if g2 else 'FAILS'}**  **H3: {'HOLDS' if g3 else 'FAILS'}**", "",
           "## H4 (reported): crew size and field-measured 2.4 GHz error", "",
           "| cell | ours - 2.4 only treach (s) | ours - 2.4 only t25 (s) | ours - 2.4 only A250 |", "|---|---|---|---|"]
    for st in X_SITES:
        md.append(f"| {st} | {cs.fmt(*pr(('E', st, 'ours'), ('E', st, '2.4 only'), 'treach_s'), p=0)} | "
                  f"{cs.fmt(*pr(('E', st, 'ours'), ('E', st, '2.4 only'), 't25_s'), p=0)} | "
                  f"{cs.fmt(*pr(('E', st, 'ours'), ('E', st, '2.4 only'), 'A250_five'))} |")
    md.append("")
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
    md += extra(out, D, problems)
    md += ["", f"Problems: {len(problems)}" + ("" if problems else " (none)")]
    md += [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REALISTIC7.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
