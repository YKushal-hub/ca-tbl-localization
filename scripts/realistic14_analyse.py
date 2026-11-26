#!/usr/bin/env python3
"""REALISTIC-14 (report/PREREG_REALISTIC14.md): REALISTIC-11's emergency cells with DD 78 (--finderDigestBothLora) for every design,
on the DD 67 preset. Copied from realistic11_analyse.py; changes: seeds, no quiet shift (no searchers there, so DD 78 cannot act),
the DD78 line checked in every run. Committed with the registration, before any cell.

REALISTIC-6's cells on seeds 1501-1520 (E_*), the 8 h quiet shift on 1521-1530 (Q_*). Crew coverage = share of crew cycles with
errM <= 1.73 sqrt(traceSigma). Paired 95 % bootstrap over seeds (camp_spec_analyse.ci), "never" = 4200 s.

  H1-H3  REALISTIC-6's rules
  L1     every worker device of ours, in every cell including the quiet shift: mean and AUDITED trailing-hour 868 MHz peak <= 2.5 % (DD56 lines)
  C1     every main cell: ours, mean crew coverage >= 0.85
  C2     every main cell: ours, mean crew coverage <= 1.00 and >= 0.90 (the nominal 0.95 band) -- reported, no prediction
  C3     quiet shift, every site: ours, mean crew coverage >= 0.85
  L2     every searcher's device of ours, every emergency cell: mean and audited peak <= 2.5 % (DD60 lines)
  T1     (reported) ours - flat threshold: treach, t25, A25, A250, J/h, coverage
  Q1     (reported) quiet shift: ours - better single A250, coverage

usage: realistic14_analyse.py <outdir>
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
E_SEEDS, Q_SEEDS = list(range(2301, 2321)), []
DESIGNS = [("ARMS", "ca_tbl_full"), ("ARMS", "fixed_threshold"), ("ARMS", "static_868"), ("ARMS", "static_2p4"),
           ("ARMS", "poucet_fusion")]  # REALISTIC-14: no TII2 cells
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
    Qs = []  # REALISTIC-14: no quiet shift
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
            log = open(os.path.join(d, "run.log"), errors="replace").read()
            if not all(k in log for k in ("DD54 sparseRangeGeometry", "DD56 sharedDuty868", "DD57 peerSplitCi", "DD59 ekf5WindowThetaWalk", "DD60 sharedDuty868Finders")):
                problems.append(f"{d}: a DD54/56/57/59/60 line is missing")
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
    for st in Qs:
        for lab, arm in (("ours", "ca_tbl_full"), ("868 only", "static_868"), ("2.4 only", "static_2p4")):
            load("Q", st, lab, arm, "ARMS", Q_SEEDS, False)
    for key, x in X.items():
        want = len(Q_SEEDS if key[0].startswith("Q") else E_SEEDS)
        if len(x) != want:
            problems.append(f"{key}: {len(x)} seeds, not {want}")
    def pr(a, b, k):
        s = sorted(set(X[a]) & set(X[b]))
        return cs.ci([X[a][i][k] - X[b][i][k] for i in s]) if s else (float("nan"),) * 3
    def mean(a, k):
        return float(np.mean([r[k] for r in X[a].values()])) if X[a] else float("nan")
    md = ["", "## L1 (legal 868 MHz duty cycle, ours, every worker device)", "", "| cell | devices | mean > 2.5 % | trailing-hour peak > 2.5 % | largest peak |", "|---|---|---|---|---|"]
    l1 = c1 = True
    for part, st, seeds in [("E", st, E_SEEDS) for st in E_SITES + X_SITES] + [("Q", st, Q_SEEDS) for st in Qs]:
        v = []
        for s in seeds:
            p = os.path.join(out, f"{part}_{st}_ARMS", f"s{s}", "run.log")
            t = open(p, errors="replace").read() if os.path.exists(p) else ""
            v += [(float(a), float(b)) for a, b in re.findall(r"DD56 device \d+ ca_tbl_full: .* of [0-9.]+ s: ([0-9.e+-]+), trailing-hour peak [0-9.e+-]+, .*audited peak ([0-9.e+-]+)", t)]
        n_mean, n_peak = sum(x[0] > 0.025 for x in v), sum(x[1] > 0.025 for x in v)
        want = {"CREW10": 10, "CREW15": 15}.get(st, 5) * len(seeds)
        if len(v) != want:
            problems.append(f"L1 {part} {st}: {len(v)} device lines, not {want}")
        l1 = l1 and len(v) == want and n_mean == 0 and n_peak == 0
        md.append(f"| {part} {st} | {len(v)} | {n_mean} | {n_peak} | {max([x[1] for x in v] or [float('nan')]):.5f} |")
    md += ["", f"**L1: {'HOLDS' if l1 else 'FAILS'}**", "", "## C1 (coverage, ours)", "", "| cell | coverage |", "|---|---|"]
    for st in E_SITES:
        m = mean(("E", st, "ours"), "cov")
        c1 = c1 and m >= 0.85
        md.append(f"| {st} | {m:.3f} |")
    md += ["", f"**C1: {'HOLDS' if c1 else 'FAILS'}**"]
    md += ["", "## T1 (reported): ours - flat threshold, both with the flag on", "", "| cell | treach (s) | t25 (s) | A25 | A250 | J/h | coverage |", "|---|---|---|---|---|---|---|"]
    for st in E_SITES:
        a, b = ("E", st, "ours"), ("E", st, "threshold")
        md.append("| " + st + " | " + " | ".join(cs.fmt(*pr(a, b, k), p=p) for k, p in (("treach_s", 0), ("t25_s", 0), ("A25_five", 3), ("A250_five", 3), ("subjJ_per_h", 1), ("cov", 3))) + " |")
    md += ["", "## Q1, C3: not in this study (no quiet shift)"] if not Qs else ["", "## Q1 (reported)", "", "| cell | ours - better single A250 | coverage |", "|---|---|---|"]
    for st in Qs:
        a = ("Q", st, "ours")
        best = max(("868 only", "2.4 only"), key=lambda lab: mean(("Q", st, lab), "A250_five"))
        md.append(f"| {st} | {cs.fmt(*pr(a, ('Q', st, best), 'A250_five'))} ({best}) | {mean(a, 'cov'):.3f} |")
    md += [] if not Qs else ["", "## C3 (quiet-shift coverage, ours)", "", "| cell | coverage | within 25 m, ours | 2.4 only |", "|---|---|---|---|"]
    c3 = True
    for st in Qs:
        a = ("Q", st, "ours")
        m = mean(a, "cov")
        c3 = c3 and m >= 0.85
        md.append(f"| {st} | {m:.3f} | {mean(a, 'A25_five'):.3f} | {mean(('Q', st, '2.4 only'), 'A25_five'):.3f} |")
    md += ([] if not Qs else ["", f"**C3: {'HOLDS' if c3 else 'FAILS'}**"]) + ["", "## L2 (legal 868 MHz duty cycle, ours, every searcher's device)", "", "| cell | devices | mean > 2.5 % | audited peak > 2.5 % | largest peak |", "|---|---|---|---|---|"]
    l2 = True
    for st in E_SITES + X_SITES:
        v, per = [], set()
        for s in E_SEEDS:
            p = os.path.join(out, f"E_{st}_ARMS", f"s{s}", "run.log")
            t = open(p, errors="replace").read() if os.path.exists(p) else ""
            w = [(float(a), float(b)) for a, b in re.findall(r"DD60 finder \d+ ca_tbl_full: .* of [0-9.]+ s: ([0-9.e+-]+), audited peak ([0-9.e+-]+)", t)]
            per.add(len(w))
            v += w
        if len(per) != 1 or not v:
            problems.append(f"L2 {st}: searcher lines per run {sorted(per)}")
        n_mean, n_peak = sum(x[0] > 0.025 for x in v), sum(x[1] > 0.025 for x in v)
        l2 = l2 and len(per) == 1 and bool(v) and n_mean == 0 and n_peak == 0
        md.append(f"| {st} | {len(v)} | {n_mean} | {n_peak} | {max([x[1] for x in v] or [float('nan')]):.5f} |")
    md += ["", f"**L2: {'HOLDS' if l2 else 'FAILS'}**"]
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
            lg = open(os.path.join(d, "run.log"), errors="replace").read()
            if "DD35 incident" not in lg:
                problems.append(f"{d}: DD35 not announced")
            if f"DD78 finderDigestBothLora ({arm})" not in lg:
                problems.append(f"{d} {arm}: DD78 not announced")
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

    md = ["# REALISTIC-14 (scripts/realistic14_analyse.py)", "", f"Sources: {', '.join(sorted(sources)) or 'none'}.", ""]
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
    open(os.path.join(out, "REALISTIC14.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
