#!/usr/bin/env python3
"""V5-C2B analysis (report/PREREG_V5_C2B.md). usage: v5_analyse.py <outdir>"""
import csv, filecmp, glob, os, random, statistics as st, sys

ARMS = ("static_2p4", "ca_tbl_full", "fixed_threshold")


def row(d, arm):
    for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv"))):
        if r["arm"] == arm:
            return r


def boot(x, n=4000):
    random.seed(1)
    m = sorted(st.mean(random.choices(x, k=len(x))) for _ in range(n))
    return st.mean(x), m[int(0.025 * n)], m[int(0.975 * n)]


def main():
    out = sys.argv[1]; L = ["# V5-C2B results (exploratory)", ""]; problems = []; S = {}
    for cfg in ("A", "B"):
        for v in ("OFF", "C2B"):
            ds = sorted(glob.glob(os.path.join(out, f"{cfg}_{v}", "s*")))
            if len(ds) < 20:
                problems.append(f"{cfg}_{v}: {len(ds)} seeds")
            for d in ds:
                if open(os.path.join(d, "rc")).read().strip() != "0":
                    problems.append(f"{d}: rc")
                if "dirty 0" not in open(os.path.join(d, "run_id.txt")).read():
                    problems.append(f"{d}: dirty")
                S[(cfg, v, int(d.rsplit("s", 1)[1]))] = d
    L += ["| config | arm | variant | CEP95 (m) | CEP50 (m) | tag-0 J | UWB cycles | escalations |", "|---|---|---|---|---|---|---|---|"]
    M = {}
    for cfg in ("A", "B"):
        for arm in ARMS:
            for v in ("OFF", "C2B"):
                rs = [row(S[(cfg, v, s)], arm) for s in range(1, 21)]
                for s, r in zip(range(1, 21), rs):
                    M[(cfg, v, arm, s)] = r
                f = lambda k: st.mean(float(r[k]) for r in rs)
                L.append(f"| {cfg} | {arm} | {v} | {f('cep95M'):.1f} | {f('cep50M'):.1f} | {f('totalJ'):.2f} | {f('cycles_uwb'):.1f} | {f('escalations'):.1f} |")
    d = lambda cfg, arm, k: [float(M[(cfg, "C2B", arm, s)][k]) - float(M[(cfg, "OFF", arm, s)][k]) for s in range(1, 21)]
    c95 = boot(d("B", "ca_tbl_full", "cep95M")); c50 = boot(d("B", "ca_tbl_full", "cep50M")); a50 = boot(d("A", "ca_tbl_full", "cep50M"))
    jr = st.mean(float(M[("B", "C2B", "ca_tbl_full", s)]["totalJ"]) / float(M[("B", "OFF", "ca_tbl_full", s)]["totalJ"]) for s in range(1, 21))
    off50 = st.mean(float(M[("B", "OFF", "ca_tbl_full", s)]["cep50M"]) for s in range(1, 21))
    same = all(filecmp.cmp(os.path.join(S[(c, "OFF", s)], f"trace_static_2p4_seed{s}.csv"), os.path.join(S[(c, "C2B", s)], f"trace_static_2p4_seed{s}.csv"), shallow=False)
               for c in ("A", "B") for s in range(1, 21))
    v = {1: c95[0] > 0, 2: jr >= 1.3, 3: abs(c50[0]) <= 0.2 * off50, 4: a50[0] < 0, 5: same}
    L += ["", f"B ca_tbl_full CEP95 C2B-OFF {c95[0]:+.1f} [{c95[1]:+.1f}, {c95[2]:+.1f}] m; CEP50 {c50[0]:+.2f} [{c50[1]:+.2f}, {c50[2]:+.2f}] m; energy ratio {jr:.3f}",
          f"A ca_tbl_full CEP50 C2B-OFF {a50[0]:+.1f} [{a50[1]:+.1f}, {a50[2]:+.1f}] m", "", "## Predictions", ""]
    L += [f"- **V5-{k}**: {'held' if v[k] else 'not held'}" for k in sorted(v)]
    sup = (not v[1]) and (not v[2])
    L += ["", f"**C2b in CA-TBL: {'SUPPORTED' if sup else 'NOT SUPPORTED'}** (supported only if B CEP95 is not worse and energy rises < 30 %)"]
    if problems:
        L += ["", "Abort conditions: " + "; ".join(problems)]
    txt = "\n".join(L) + "\n"; open(os.path.join(out, "V5.md"), "w").write(txt); print(txt)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
