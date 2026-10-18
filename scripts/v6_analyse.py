#!/usr/bin/env python3
"""V6-ONDEMAND analysis (report/PREREG_V6_ONDEMAND.md). usage: v6_analyse.py <outdir>"""
import csv, glob, os, random, re, statistics as st, sys


def summ(d):
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == "ca_tbl_full":
            return float(r["totalJ"]), float(r["cep95M"])


def boot(x, n=4000):
    random.seed(1)
    m = sorted(st.mean(random.choices(x, k=len(x))) for _ in range(n))
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def main():
    out = sys.argv[1]; L = ["# V6-ONDEMAND results (exploratory)", ""]; problems = []; S = {}
    for cfg in ("A", "B"):
        for v in ("ALWAYS", "RAW", "CAL"):
            ds = sorted(glob.glob(os.path.join(out, f"{cfg}_{v}", "s*")))
            if len(ds) < 20:
                problems.append(f"{cfg}_{v}: {len(ds)} seeds")
            for d in ds:
                if open(os.path.join(d, "rc")).read().strip() != "0":
                    problems.append(f"{d}: rc")
                if "dirty 0" not in open(os.path.join(d, "run_id.txt")).read():
                    problems.append(f"{d}: dirty")
            S[(cfg, v)] = {int(d.rsplit("s", 1)[1]): d for d in ds}
    L += ["| config | variant | tag-0 J | CEP95 (m) | 2.4 attempted share | sleeps (share of decided) |", "|---|---|---|---|---|---|"]
    J, C = {}, {}
    for cfg in ("A", "B"):
        for v in ("ALWAYS", "RAW", "CAL"):
            js, cs, att, slp = [], [], [], []
            for s, d in S[(cfg, v)].items():
                j, c = summ(d); js.append(j); cs.append(c); J[(cfg, v, s)] = j; C[(cfg, v, s)] = c
                rows = list(csv.DictReader(open(glob.glob(os.path.join(d, "ondemand_*.csv"))[0])))
                att.append(sum(int(r["attempted24"]) for r in rows) / max(1, len(rows)))
                m = re.findall(r"tag-0 cycles decided (\d+), 2.4 GHz radio asleep (\d+)", open(os.path.join(d, "run.log")).read())
                slp.append(int(m[-1][1]) / max(1, int(m[-1][0])) if m else 0.0)
            L.append(f"| {cfg} | {v} | {st.mean(js):.2f} | {st.mean(cs):.1f} | {st.mean(att):.3f} | {st.mean(slp):.4f} |")
            S[(cfg, v, "sleep")] = st.mean(slp)
    # V6-1
    P, G = [], []
    for s, d in S[("A", "CAL")].items():
        for r in csv.DictReader(open(glob.glob(os.path.join(d, "ondemand_*.csv"))[0])):
            if r["attempted24"] == "1" and float(r["t"]) >= 300 and float(r["pCal"]) >= 0:
                P.append(float(r["pCal"])); G.append(1.0 if int(r["n24"]) > 0 else 0.0)
    gap = abs(st.mean(P) - st.mean(G)) if P else float("nan")
    res = {}
    for cfg in ("A", "B"):
        dj = [(J[(cfg, "ALWAYS", s)] - J[(cfg, "CAL", s)]) / J[(cfg, "ALWAYS", s)] for s in range(1, 21)]
        dc = [(C[(cfg, "CAL", s)] - C[(cfg, "ALWAYS", s)]) / C[(cfg, "ALWAYS", s)] for s in range(1, 21)]
        res[cfg] = (boot(dj), boot(dc))
        L += [f"", f"{cfg}: CAL energy saving {100*res[cfg][0][0]:+.2f} % [{100*res[cfg][0][1]:+.2f}, {100*res[cfg][0][2]:+.2f}]; "
              f"CEP95 change {100*res[cfg][1][0]:+.1f} % [{100*res[cfg][1][1]:+.1f}, {100*res[cfg][1][2]:+.1f}]"]
    L += ["", f"V6-1 data: A CAL attempted cycles after 300 s: n {len(P)}, mean pCal {st.mean(P):.3f}, observed {st.mean(G):.3f}", "", "## Predictions", "",
          f"- **V6-1**: {'held' if gap <= 0.05 else 'not held'} (gap {gap:.3f})",
          f"- **V6-2**: {'held' if all(res[c][0][0] < 0.05 for c in res) else 'not held'}",
          f"- **V6-3**: {'held' if all(abs(res[c][1][0]) <= 0.25 for c in res) else 'not held'}",
          f"- **V6-4**: {'held' if S[('B','RAW','sleep')] < 0.02 else 'not held'} ({S[('B','RAW','sleep')]:.4f})"]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems)]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "V6.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
