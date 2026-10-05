#!/usr/bin/env python3
"""LORA-CMP analysis (report/PREREG_LORA_CMP.md): CA-TBL against outdoor LoRa-based
localisation on our simulator, paired by seed with CAMP-FINAL's cells. Writes
<outdir>/LORA_CMP.md and <outdir>/lora_cmp_table.csv.

The per-seed quantities are CAMP-SPEC's matrix definitions (scripts/camp_spec_analyse.py
matrix()), restated here per seed so they can be paired: availability is the mean over the
subjects of fixCycles / steps; CEP50 and CEP95 are tag 0's; subject CEP95 and subject J are
means over the subjects; TTFF is tag 0's first fix (never = the duration). Added: subject J per
fix (summed totalJ over summed fixCycles, over the subjects), and the paired difference of each
comparator from CA-TBL, per seed, with a 95 % seed bootstrap.

usage: lora_cmp_analyse.py <lora_cmp outdir> <camp_final outdir>
"""
import csv
import glob
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import numpy as np  # noqa: E402

CONFIGS = [
    ("5 km (A_main)", [
        ("CA-TBL", "cf", "A_main", "ca_tbl_full"),
        ("LoRa RSSI multilateration", "lc", "R_main", "static_868"),
        ("LoRa RSSI + gateway TDoA", "cf", "A_main", "static_868"),
        ("fixed-threshold band switching", "cf", "A_main", "fixed_threshold"),
        ("Poucet-style LoRa+UWB fusion", "cf", "A_main", "poucet_fusion"),
        ("CA-TBL single-hop (LoRaWAPS-style)", "cf", "G_single", "ca_tbl_full"),
    ]),
    ("endgame (B_end)", [
        ("CA-TBL", "cf", "B_end", "ca_tbl_full"),
        ("LoRa RSSI multilateration", "lc", "R_end", "static_868"),
        ("LoRa RSSI + gateway TDoA", "cf", "B_end", "static_868"),
        ("fixed-threshold band switching", "cf", "B_end", "fixed_threshold"),
        ("Poucet-style LoRa+UWB fusion", "cf", "B_end", "poucet_fusion"),
        ("CA-TBL single-hop (LoRaWAPS-style)", "cf", "G_single_end", "ca_tbl_full"),
    ]),
    ("N8_ring (C_n8)", [
        ("CA-TBL", "cf", "C_n8", "ca_tbl_full"),
        ("LoRa RSSI multilateration", "lc", "R_n8", "static_868"),
        ("LoRa RSSI + gateway TDoA", "cf", "C_n8", "static_868"),
    ]),
]


def per_seed(out, cell, arm):
    """{seed: dict of the per-seed quantities} for <arm> in <out>/<cell>."""
    res = {}
    for d in cs.seeds(out, cell):
        if not cs.ok(d):
            continue
        a = cs.arm_row(d, arm)
        if a is None:
            continue
        ss = cs.subj_rows(d, arm)
        st = float(a["steps"])
        t = float(a["tFirstFixS"])
        nf = sum(float(r["fixCycles"]) for r in ss)
        res[int(d.rsplit("s", 1)[1])] = dict(
            av=np.mean([float(r["fixCycles"]) / float(r["steps"]) for r in ss]) if ss else float(a["fixCycles"]) / st,
            c50=float(a["cep50M"]), c95=float(a["cep95M"]),
            s95=np.mean([float(r["cep95M"]) for r in ss]) if ss else float("nan"),
            tff=t if t >= 0 else cs.DUR,
            J=np.mean([float(r["totalJ"]) for r in ss]) if ss else float(a["totalJ"]),
            jpf=sum(float(r["totalJ"]) for r in ss) / nf if nf > 0 else float("nan"))
    return res


def checks(lc, cf, problems):
    """The registered abort conditions (PREREG_LORA_CMP sect 4)."""
    rcs = glob.glob(os.path.join(lc, "*", "s*", "rc"))
    if len(rcs) != 61:
        problems.append(f"{len(rcs)} cells with an rc, not 61")
    for p in rcs:
        if open(p).read().strip() != "0":
            problems.append(f"{os.path.dirname(p)} exit")
    srcs = set()
    for p in glob.glob(os.path.join(lc, "*", "s*", "run_id.txt")):
        for ln in open(p):
            if ln.startswith("source "):
                srcs.add(" ".join(ln.split()[:4]))
    if len(srcs) != 1 or not next(iter(srcs)).endswith("dirty 0"):
        problems.append(f"run_id source lines {sorted(srcs)}")
    a = [ln for ln in open(os.path.join(cf, "A_main", "s1", "arm_summary.csv")) if ln.startswith("static_868,")]
    x = os.path.join(lc, "X_repro", "s1", "arm_summary.csv")
    b = [ln for ln in open(x) if ln.startswith("static_868,")] if os.path.exists(x) else []
    if not a or a != b:
        problems.append("X_repro: static_868 row differs from CAMP-FINAL A_main/s1 (binary or stream drift)")
    # Reachability, both ways: TDoA ran in CAMP-FINAL's static_868 cells and not in R_*.
    for cell in ("R_main", "R_end", "R_n8"):
        n = sum(len(glob.glob(os.path.join(d, "tdoa_*"))) for d in cs.seeds(lc, cell))
        if n:
            problems.append(f"{cell}: {n} tdoa_ files with --tier1Tdoa=false")
    for cell in ("A_main", "B_end", "C_n8"):
        n = sum(len(glob.glob(os.path.join(d, "tdoa_static_868_*"))) for d in cs.seeds(cf, cell))
        if n != 20:
            problems.append(f"{cell}: tdoa_static_868 files {n}, not 20")


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    lc, cf = sys.argv[1], sys.argv[2]
    roots = {"lc": lc, "cf": cf}
    problems = []
    checks(lc, cf, problems)
    md = ["# LORA-CMP — CA-TBL against outdoor LoRa localisation (scripts/lora_cmp_analyse.py)", ""]
    rows = []
    for title, comps in CONFIGS:
        data = [(name, per_seed(roots[r], cell, arm), f"{cell}/{arm}") for name, r, cell, arm in comps]
        ref = data[0][1]
        md += [f"## {title}", "",
               "| comparator | source | seeds | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | TTFF (s) | subject J | J per subject fix |",
               "|---|---|---|---|---|---|---|---|---|"]
        for name, v, src in data:
            if len(v) != 20:
                problems.append(f"{title} {name}: {len(v)} seeds, not 20")
            if not v:
                continue
            col = lambda k: [x[k] for x in v.values()]  # noqa: E731
            av = cs.ci(col("av"))
            md.append(f"| {name} | {src} | {len(v)} | {cs.fmt(*av)} | {np.mean(col('c50')):.1f} / {np.mean(col('c95')):.1f} | "
                      f"{np.nanmean(col('s95')):.1f} | {np.median(col('tff')):.0f} | {np.mean(col('J')):.2f} | {np.nanmean(col('jpf')):.4f} |")
            rows.append(dict(config=title, comparator=name, source=src, seeds=len(v), availability=f"{av[0]:.4f}",
                             availability_lo=f"{av[1]:.4f}", availability_hi=f"{av[2]:.4f}",
                             cep50_tag0_m=f"{np.mean(col('c50')):.2f}", cep95_tag0_m=f"{np.mean(col('c95')):.2f}",
                             cep95_subjects_m=f"{np.nanmean(col('s95')):.2f}", ttff_median_s=f"{np.median(col('tff')):.0f}",
                             subject_j=f"{np.mean(col('J')):.3f}", j_per_subject_fix=f"{np.nanmean(col('jpf')):.5f}"))
        md += ["", "Paired by seed, comparator − CA-TBL, mean [95 %] (availability > 0: the comparator fixes more often; "
               "CEP95 < 0: the comparator's tag 0 error is lower):", "",
               "| comparator | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J | seeds |", "|---|---|---|---|---|"]
        for name, v, _ in data[1:]:
            s = sorted(set(v) & set(ref))
            if not s:
                continue
            d = lambda k: cs.fmt(*cs.ci([v[i][k] - ref[i][k] for i in s]), p=3 if k == "av" else 1)  # noqa: E731
            md.append(f"| {name} | {d('av')} | {d('c95')} | {d('J')} | {len(s)} |")
        md.append("")
    with open(os.path.join(lc, "lora_cmp_table.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems]
    open(os.path.join(lc, "LORA_CMP.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
