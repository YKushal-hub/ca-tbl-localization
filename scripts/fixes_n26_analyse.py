#!/usr/bin/env python3
"""FIXES-N26 analysis (report/PREREG_FIXES_N26.md).

usage: fixes_n26_analyse.py fit  <outdir>
       fixes_n26_analyse.py eval <outdir> <camp_final outdir> <lora_cmp outdir> <lora_cmp2 outdir>

fit:  the integrity threshold from the fault-free calibration cells (seeds 21-40, every fix
      on, --mahalUseR=true): integrityChi2 = max(9.21, the 99th percentile of tag 0's
      mahalanobis over fix cycles the check judged, i.e. mahalanobis > 0, at t >= WARMUP_S),
      pooled over 5 km and the endgame. Writes <outdir>/fit.json and FIT.md.
eval: the tables, the abort checks and the registered adoption rule. Writes
      <outdir>/FIXES_N26.md and fixes_n26_table.csv. Per-seed quantities are LORA-CMP's
      (scripts/lora_cmp_analyse.py per_seed), paired by seed.
"""
import csv
import glob
import json
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from camp_final_analyse import uwb_fixes  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402
import numpy as np  # noqa: E402

UNSTABLE = {"rmse_mlat", "cep95_mlat"}  # as LORA-CMP-2 sect 7
WARMUP_S = 120.0  # OURS: the first two minutes are the EKF's acquisition from its 100 m prior
CONFIGS = {
    "A": ("5 km (A_main)", "A_main", "R_main", "F_main", "_main"),
    "B": ("endgame (B_end)", "B_end", "R_end", "F_end", "_end"),
    "C": ("N8_ring (C_n8)", "C_n8", "R_n8", "F_n8", "_n8"),
}


def fit(out):
    vals = {}
    for cell in ("CAL_main", "CAL_end"):
        v = []
        for d in cs.seeds(os.path.join(out, "cal"), cell):
            if not cs.ok(d):
                print(f"cell {d} did not exit 0: abort")
                return 1
            v += [float(r["mahalanobis"]) for r in cs.trace(d, "ca_tbl_full")
                  if r["hadFix"] == "1" and float(r["mahalanobis"]) > 0.0 and float(r["t"]) >= WARMUP_S]
        vals[cell] = np.asarray(v)
    pooled = np.concatenate(list(vals.values()))
    if len(pooled) == 0:
        print(f"no judged fix cycle at t >= {WARMUP_S:.0f} s in the calibration cells: abort")
        return 1
    p99 = float(np.quantile(pooled, 0.99))
    chi = max(9.21, round(p99, 2))
    res = {"integrityChi2": chi, "p99_pooled": p99, "n_pooled": int(len(pooled))}
    md = ["# FIXES-N26 — integrity threshold fit (scripts/fixes_n26_analyse.py fit)", "",
          "| cell | eligible fix cycles | p50 | p90 | p99 |", "|---|---|---|---|---|"]
    for k, v in vals.items():
        q = np.quantile(v, [0.5, 0.9, 0.99]) if len(v) else [float("nan")] * 3
        res[k] = {"n": int(len(v)), "p50": float(q[0]), "p90": float(q[1]), "p99": float(q[2])}
        md.append(f"| {k} | {len(v)} | {q[0]:.2f} | {q[1]:.2f} | {q[2]:.2f} |")
    md += ["", f"Pooled p99 {p99:.3f} over {len(pooled)} cycles at t >= {WARMUP_S:.0f} s -> "
           f"integrityChi2 = max(9.21, p99) = **{chi}**."]
    json.dump(res, open(os.path.join(out, "fit.json"), "w"), indent=1, sort_keys=True)
    open(os.path.join(out, "FIT.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


def rows_of(p, arm):
    try:
        rr = list(csv.reader(open(p)))
    except OSError:
        return []
    keep = [i for i, h in enumerate(rr[0]) if h not in UNSTABLE]
    return [[r[i] for i in keep] for r in rr[1:] if r and r[0] == arm]


def checks(ev, roots, chi, problems):
    rcs = glob.glob(os.path.join(ev, "*", "s*", "rc"))
    if len(rcs) != 263:
        problems.append(f"{len(rcs)} eval cells with an rc, not 263")
    for p in rcs:
        if open(p).read().strip() != "0":
            problems.append(f"{os.path.dirname(p)} exit")
    srcs = set()
    for p in glob.glob(os.path.join(ev, "*", "s*", "run_id.txt")) + \
            glob.glob(os.path.join(ev, "..", "cal", "*", "s*", "run_id.txt")):
        for ln in open(p):
            if ln.startswith("source "):
                srcs.add(" ".join(ln.split()[:4]))
    if len(srcs) != 1 or not next(iter(srcs)).endswith("dirty 0"):
        problems.append(f"run_id source lines {sorted(srcs)}")
    for mine, root, theirs, arms in (("X_main", "cf", "A_main", ["ca_tbl_full", "static_868", "poucet_fusion"]),
                                     ("X_end", "cf", "B_end", ["ca_tbl_full", "static_868", "poucet_fusion"]),
                                     ("X_lf", "l2", "F_end", ["lora_fusion"])):
        for f in ("arm_summary.csv", "subject_summary.csv"):
            for arm in arms:
                a = rows_of(os.path.join(roots[root], theirs, "s1", f), arm)
                b = rows_of(os.path.join(ev, mine, "s1", f), arm)
                if not a or a != b:
                    problems.append(f"{mine}: {arm}'s {f} rows differ from {theirs}/s1 (the pairing across binaries fails)")
    want = {"ALL": ("dual", "cross", "chi"), "D": ("dual",), "U": (), "T": (), "M": ("chi",)}
    for pre, need in want.items():
        for suf in ("_main", "_end") + (("_n8",) if pre == "ALL" else ()):
            for d in cs.seeds(ev, pre + suf):
                t = open(os.path.join(d, "run.log")).read() if os.path.exists(os.path.join(d, "run.log")) else ""
                ln = next((x for x in t.splitlines() if "N.26 fixes (ca_tbl_full)" in x), None)
                if ln is None:
                    problems.append(f"{pre}{suf}/{os.path.basename(d)}: no N.26 line")
                    continue
                if "dual" in need and "node-cycles 0," in ln:
                    problems.append(f"{pre}{suf}/{os.path.basename(d)}: dual base did not range")
                if "chi" in need and f"integrity threshold {chi:.6f}" not in ln:
                    problems.append(f"{pre}{suf}/{os.path.basename(d)}: integrity threshold is not {chi}")


def suspect_share(root, cell):
    n = s = 0
    for d in cs.seeds(root, cell):
        for r in cs.trace(d, "ca_tbl_full"):
            if r["hadFix"] == "1":
                n += 1
                s += r["suspect"] == "1"
    return s / n if n else float("nan")


def evaluate(out, cf, lc, l2):
    ev = os.path.join(out, "eval")
    chi = json.load(open(os.path.join(out, "fit.json")))["integrityChi2"]
    roots = {"ev": ev, "cf": cf, "lc": lc, "l2": l2}
    problems = []
    checks(ev, roots, chi, problems)
    md = ["# FIXES-N26 — the four N.26 fixes on the preset (scripts/fixes_n26_analyse.py eval)", "",
          f"Integrity threshold from the calibration: {chi}.", ""]
    out_rows = []
    verdict = {}
    for key, (title, cfc, lcc, l2c, suf) in CONFIGS.items():
        comps = [("CA-TBL (preset)", "cf", cfc, "ca_tbl_full"),
                 ("CA-TBL, all four fixes", "ev", "ALL" + suf, "ca_tbl_full")]
        if key != "C":
            comps += [("+ dual LoRa base only", "ev", "D" + suf, "ca_tbl_full"),
                      ("+ UWB cross-tier poll only", "ev", "U" + suf, "ca_tbl_full"),
                      ("+ TDoA innovation gate only", "ev", "T" + suf, "ca_tbl_full"),
                      ("+ integrity (mahalUseR + fitted chi2) only", "ev", "M" + suf, "ca_tbl_full")]
        comps += [("LoRa dual-band fusion (TDoA)", "l2", l2c, "lora_fusion"),
                  ("LoRa 868 RSSI", "lc", lcc, "static_868"),
                  ("LoRa 868 RSSI + TDoA", "cf", cfc, "static_868")]
        if key != "C":
            comps += [("fixed-threshold switching", "cf", cfc, "fixed_threshold"),
                      ("Poucet (LoRa + UWB fused)", "cf", cfc, "poucet_fusion")]
        data = [(n, per_seed(roots[r], c, a), f"{c}/{a}", r, c, a) for n, r, c, a in comps]
        ref = data[0][1]
        md += [f"## {title}", "",
               "| comparator | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | UWB-delivered fixes (seeds) |",
               "|---|---|---|---|---|---|---|"]
        for n, v, src, r, c, a in data:
            if len(v) != 20:
                problems.append(f"{title} {n}: {len(v)} seeds, not 20")
            if not v:
                continue
            col = lambda k: [x[k] for x in v.values()]  # noqa: E731
            av = cs.ci(col("av"))
            uw = ""
            if a == "ca_tbl_full":
                u = [uwb_fixes(d, int(d.rsplit("s", 1)[1])) for d in cs.seeds(roots[r], c)]
                uw = f"{sum(u)} ({sum(x > 0 for x in u)})"
            md.append(f"| {n} | {cs.fmt(*av)} | {np.mean(col('c50')):.1f} / {np.mean(col('c95')):.1f} | "
                      f"{np.nanmean(col('s95')):.1f} | {np.median(col('tff')):.0f} | {np.mean(col('J')):.2f} | {uw} |")
            out_rows.append(dict(config=title, comparator=n, source=src, availability=f"{av[0]:.4f}",
                                 availability_lo=f"{av[1]:.4f}", availability_hi=f"{av[2]:.4f}",
                                 cep50_tag0_m=f"{np.mean(col('c50')):.2f}", cep95_tag0_m=f"{np.mean(col('c95')):.2f}",
                                 cep95_subjects_m=f"{np.nanmean(col('s95')):.2f}", subject_j=f"{np.mean(col('J')):.3f}"))
        md += ["", "Paired by seed, comparator − CA-TBL (preset), mean [95 %]:", "",
               "| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|"]
        for n, v, *_ in data[1:]:
            s = sorted(set(v) & set(ref))
            if not s:
                continue
            f = lambda k, p: cs.fmt(*cs.ci([v[i][k] - ref[i][k] for i in s]), p=p)  # noqa: E731
            md.append(f"| {n} | {f('av', 3)} | {f('c95', 1)} | {f('J', 2)} |")
        allv = data[1][1]
        for other in ("LoRa dual-band fusion (TDoA)", "Poucet (LoRa + UWB fused)"):
            ov = next((v for n, v, *_ in data if n == other), None)
            if ov:
                s = sorted(set(allv) & set(ov))
                g = lambda k, p: cs.fmt(*cs.ci([allv[i][k] - ov[i][k] for i in s]), p=p)  # noqa: E731
                md.append(f"\nAll four fixes − {other}, paired: availability {g('av', 3)}, CEP95 tag 0 {g('c95', 1)} m, "
                          f"subject J {g('J', 2)}.")
        if key in ("A", "B"):
            s = sorted(set(allv) & set(ref))
            dav = cs.ci([allv[i]["av"] - ref[i]["av"] for i in s])
            ratio = float(np.mean([allv[i]["c95"] / ref[i]["c95"] - 1.0 for i in s]))
            jall, jref = float(np.mean([allv[i]["J"] for i in s])), float(np.mean([ref[i]["J"] for i in s]))
            verdict[key] = (dav[1] > 0.0, ratio <= 0.10, jall <= jref, dav, ratio, jall, jref)
            md.append(f"\nAdoption terms ({key}): availability gain interval {cs.fmt(*dav)} (need lower > 0); tag 0 CEP95 "
                      f"mean per-seed change {ratio:+.3f} (need <= +0.10); subject J {jall:.2f} against {jref:.2f} (need <=).")
        md.append("")
    md += ["## Integrity check, fault-free: share of tag 0 fix cycles SUSPECT", "",
           f"- preset: A_main {suspect_share(cf, 'A_main'):.3f}, B_end {suspect_share(cf, 'B_end'):.3f}",
           f"- all four fixes: ALL_main {suspect_share(ev, 'ALL_main'):.3f}, ALL_end {suspect_share(ev, 'ALL_end'):.3f}",
           f"- integrity fix alone: M_main {suspect_share(ev, 'M_main'):.3f}, M_end {suspect_share(ev, 'M_end'):.3f}", ""]
    cs.faults(ev, md)
    adopt = all(all(verdict[k][:3]) for k in ("A", "B")) if len(verdict) == 2 else False
    md += [f"**Registered adoption rule: {'ADOPT' if adopt else 'DO NOT ADOPT'}** "
           f"(A: {verdict.get('A', ('?',) * 3)[:3]}, B: {verdict.get('B', ('?',) * 3)[:3]}; "
           "terms = availability gain, CEP95 within +10 %, subject J not higher).", ""]
    with open(os.path.join(ev, "fixes_n26_table.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(out_rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(out_rows)
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems]
    open(os.path.join(ev, "FIXES_N26.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


def main():
    if len(sys.argv) >= 3 and sys.argv[1] == "fit":
        return fit(sys.argv[2])
    if len(sys.argv) == 6 and sys.argv[1] == "eval":
        return evaluate(*sys.argv[2:])
    print(__doc__)
    return 2


if __name__ == "__main__":
    sys.exit(main())
