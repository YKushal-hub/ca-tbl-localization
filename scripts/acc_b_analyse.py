#!/usr/bin/env python3
"""ACC-B analysis (report/PREREG_ACC_B_HYBRID.md): --dualBaseRelayTier off and on.

usage: acc_b_analyse.py <outdir written by scripts/acc_b_run.sh>

Writes <outdir>/ACC_B.md, prints it, and exits 1 if any registered abort condition holds.
"""
import csv
import filecmp
import glob
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import numpy as np  # noqa: E402

NCELLS = 80


def rows(p):
    try:
        return list(csv.DictReader(open(p)))
    except OSError:
        return None


def metrics(d, arm):
    s = int(d.rsplit("s", 1)[1])
    a = cs.arm_row(d, arm)
    ss = cs.subj_rows(d, arm)
    t0 = rows(os.path.join(d, f"trace_{arm}_seed{s}.csv"))
    if a is None or not ss or t0 is None:
        return None
    e0 = np.array([float(r["errM"]) for r in t0])
    w, subj_med = [float(np.mean(e0 <= 25))], []
    for k in range(2, 6):
        st = rows(os.path.join(d, f"subjtrace_{arm}_seed{s}_subj{k}.csv"))
        if st:
            e = np.array([float(r["errM"]) for r in st])
            w.append(float(np.mean(e <= 25)))
            subj_med.append(float(np.median(e)))
    out = dict(av=float(np.mean([float(r["fixCycles"]) / float(r["steps"]) for r in ss])),
               w25=float(np.mean(w)), c50=float(a["cep50M"]), c95=float(a["cep95M"]),
               J=float(np.mean([float(r["totalJ"]) for r in ss])),
               nr=float(np.mean([int(r["nRangesEkf"]) for r in t0])), subj_med=subj_med)
    L = open(os.path.join(d, "run.log")).read().splitlines()
    tag = [ln for ln in L if "leg tag->relay" in ln]
    i = 0 if arm == "ca_tbl_full" else 1
    if len(tag) == 2:
        m = re.search(r"offered by depth 1/2/3\+ (\d+)/(\d+)/(\d+).*ADMIT (\d+)", tag[i])
        out["legs"], out["adm"] = sum(int(x) for x in m.groups()[:3]), int(m.group(4))
    return out


def collect(out, cell, arm):
    res = {}
    for d in cs.seeds(out, cell):
        if cs.ok(d):
            m = metrics(d, arm)
            if m is not None:
                res[int(d.rsplit("s", 1)[1])] = m
    return res


def paired(a, b, k):
    s = sorted(set(a) & set(b))
    return cs.ci([a[i][k] - b[i][k] for i in s])


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems = []
    rcs = glob.glob(os.path.join(out, "*", "s*", "rc"))
    if len(rcs) != NCELLS:
        problems.append(f"{len(rcs)} cells with an rc, not {NCELLS}")
    for p in rcs:
        if open(p).read().strip() != "0":
            problems.append(f"{os.path.dirname(p)} exit")
    srcs = set()
    for p in glob.glob(os.path.join(out, "*", "s*", "run_id.txt")):
        for ln in open(p):
            if ln.startswith("source "):
                srcs.add(" ".join(ln.split()[:4]))
    if len(srcs) != 1 or not next(iter(srcs)).endswith("dirty 0"):
        problems.append(f"run_id source lines {sorted(srcs)}")
    D = {(f, c, arm): collect(out, f"{f}_{c}", arm) for f in ("off", "on") for c in "AB"
         for arm in ("ca_tbl_full", "static_2p4")}
    for k, v in D.items():
        if len(v) != 20:
            problems.append(f"{k}: {len(v)} seeds, not 20")
    # P-B5: static_2p4 byte-identical on and off, every seed
    iso_bad = 0
    for c in "AB":
        for d in cs.seeds(out, f"off_{c}"):
            s = d.rsplit("/", 1)[1]
            for p in glob.glob(os.path.join(d, "*_static_2p4_seed*")):
                q = os.path.join(out, f"on_{c}", s, os.path.basename(p))
                if not os.path.exists(q) or not filecmp.cmp(p, q, shallow=False):
                    iso_bad += 1
            ro = [ln for ln in open(os.path.join(d, "arm_summary.csv")) if ln.startswith("static_2p4,")]
            on_p = os.path.join(out, f"on_{c}", s, "arm_summary.csv")
            rn = [ln for ln in open(on_p) if ln.startswith("static_2p4,")] if os.path.exists(on_p) else []
            iso_bad += (ro != rn)
    # rule 4: the flag fired in every on cell
    fired = [bool(re.search(r"B-F1 dual-base relay tier \(ca_tbl_full\): tag 0 legs on 2\.4 GHz with 868 MHz active [1-9]",
                            open(os.path.join(d, "run.log")).read())) for c in "AB" for d in cs.seeds(out, f"on_{c}")]
    if not fired or not all(fired):
        problems.append(f"the B-F1 line shows no tag-0 legs in {fired.count(False)} on cells")

    md = ["# ACC-B — --dualBaseRelayTier (scripts/acc_b_analyse.py)", "",
          "Mean [95 % seed bootstrap], seeds 1-20, paired by seed.", ""]
    q = {}
    for c, name in (("A", "5 km"), ("B", "endgame")):
        f0, f1 = D[("off", c, "ca_tbl_full")], D[("on", c, "ca_tbl_full")]
        s2 = D[("off", c, "static_2p4")]
        col = lambda v, k: np.mean([x[k] for x in v.values()]) if v else float("nan")  # noqa: E731
        md += [f"## {name} ({c})", "", "| | availability | within 25 m | CEP50 | CEP95 | J/node | ranges into EKF / cycle | tag-0 relay legs | admitted |",
               "|---|---|---|---|---|---|---|---|---|"]
        for lab, v in (("ca_tbl_full off", f0), ("ca_tbl_full on", f1), ("static_2p4", s2)):
            md.append(f"| {lab} | {col(v, 'av'):.3f} | {col(v, 'w25'):.3f} | {col(v, 'c50'):.1f} | {col(v, 'c95'):.1f} | {col(v, 'J'):.2f} | "
                      f"{col(v, 'nr'):.2f} | {col(v, 'legs'):.0f} | {col(v, 'adm'):.0f} |")
        r = {k: paired(f1, f0, k) for k in ("av", "w25", "c50", "c95", "J", "nr", "legs", "adm")}
        rs = {k: paired(f1, s2, k) for k in ("av", "w25", "c50", "c95")}
        r0 = {k: paired(f0, s2, k) for k in ("w25",)}
        q[c] = (r, rs, r0, f0, f1)
        md += ["", "on − off: " + "; ".join(f"{k} {cs.fmt(*r[k], p=3 if k in ('av', 'w25') else 2)}" for k in r),
               "", "on − static_2p4: " + "; ".join(f"{k} {cs.fmt(*rs[k], p=3 if k in ('av', 'w25') else 2)}" for k in rs),
               f"; off − static_2p4 within 25 m {cs.fmt(*r0['w25'])}", "",
               "subject median error (m), mean over seeds, subjects 2-5: off "
               + str([round(float(np.mean([x['subj_med'][i] for x in f0.values() if len(x['subj_med']) > i])), 1) for i in range(4)])
               + ", on " + str([round(float(np.mean([x['subj_med'][i] for x in f1.values() if len(x['subj_med']) > i])), 1) for i in range(4)]), ""]
    md.append(f"P-B5 isolation: static_2p4 files or rows differing on vs off: {iso_bad}")

    def above0(iv):
        return not math.isnan(iv[1]) and iv[1] > 0

    def below0(iv):
        return not math.isnan(iv[2]) and iv[2] < 0

    V = []
    try:
        rB, _, _, f0B, f1B = q["B"]
        rA, rsA, _, f0A, f1A = q["A"]
        V.append(("Q1", np.mean([x["legs"] for x in f1B.values()]) >= 2 * np.mean([x["legs"] for x in f0B.values()])
                  and above0(rB["adm"])))
        V.append(("Q2", above0(rB["nr"])))
        ratio = lambda f0, f1, k: float(np.mean([f1[i][k] / f0[i][k] - 1 for i in sorted(set(f0) & set(f1))]))  # noqa: E731
        V.append(("Q3", below0(rB["c50"]) and below0(rB["c95"]) and ratio(f0B, f1B, "c50") <= -0.10 and ratio(f0B, f1B, "c95") <= -0.10))
        V.append(("Q4", not below0(rsA["w25"])))
        V.append(("Q5", all(q[c][0]["av"][0] >= -0.01 for c in "AB")
                  and all(ratio(q[c][3], q[c][4], "J") <= 0.15 for c in "AB")))
        V.append(("Q6", iso_bad == 0))
    except (KeyError, TypeError, ValueError, ZeroDivisionError) as e:
        problems.append(f"a prediction could not be evaluated: {e!r}")
    md += ["", "## Registered predictions", ""] + [f"- {k}: {'HELD' if v else 'FAILED'}" for k, v in V] + [""]
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems]
    open(os.path.join(out, "ACC_B.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
