#!/usr/bin/env python3
"""LORA-CMP-2 analysis (report/PREREG_LORA_CMP2.md): CA-TBL against LoRa localisation on both
LoRa bands, paired by seed with CAMP-FINAL's and LORA-CMP's cells. Writes
<l2 outdir>/LORA_CMP2.md and <l2 outdir>/lora_cmp2_table.csv.

The per-seed quantities are LORA-CMP's (scripts/lora_cmp_analyse.py per_seed(), CAMP-SPEC's
matrix definitions), imported unchanged. Added: the paired difference poucet_fusion −
lora_fusion (both with the preset's TDoA) in A and B, which is what UWB adds to Poucet.

usage: lora_cmp2_analyse.py <lora_cmp2 outdir> <lora_cmp outdir> <camp_final outdir>
"""
import csv
import glob
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402
import numpy as np  # noqa: E402

DUAL = "LoRa dual-band fusion, 868 RSSI+TDoA + 2.4 ToF"
DUALR = "LoRa dual-band fusion, 868 RSSI + 2.4 ToF"
POUCET = "Poucet-style LoRa+UWB fusion"
CONFIGS = [
    ("5 km (A_main)", [
        ("CA-TBL", "cf", "A_main", "ca_tbl_full"),
        ("LoRa 868 RSSI", "lc", "R_main", "static_868"),
        ("LoRa 868 RSSI + gateway TDoA", "cf", "A_main", "static_868"),
        ("LoRa 2.4 GHz ToF", "cf", "A_main", "static_2p4"),
        (DUAL, "l2", "F_main", "lora_fusion"),
        (DUALR, "l2", "FR_main", "lora_fusion"),
        ("fixed-threshold band switching", "cf", "A_main", "fixed_threshold"),
        (POUCET, "cf", "A_main", "poucet_fusion"),
        ("CA-TBL single-hop (LoRaWAPS-style)", "cf", "G_single", "ca_tbl_full"),
    ]),
    ("endgame (B_end)", [
        ("CA-TBL", "cf", "B_end", "ca_tbl_full"),
        ("LoRa 868 RSSI", "lc", "R_end", "static_868"),
        ("LoRa 868 RSSI + gateway TDoA", "cf", "B_end", "static_868"),
        ("LoRa 2.4 GHz ToF", "cf", "B_end", "static_2p4"),
        (DUAL, "l2", "F_end", "lora_fusion"),
        (DUALR, "l2", "FR_end", "lora_fusion"),
        ("fixed-threshold band switching", "cf", "B_end", "fixed_threshold"),
        (POUCET, "cf", "B_end", "poucet_fusion"),
        ("CA-TBL single-hop (LoRaWAPS-style)", "cf", "G_single_end", "ca_tbl_full"),
    ]),
    ("N8_ring (C_n8)", [
        ("CA-TBL", "cf", "C_n8", "ca_tbl_full"),
        ("LoRa 868 RSSI", "lc", "R_n8", "static_868"),
        ("LoRa 868 RSSI + gateway TDoA", "cf", "C_n8", "static_868"),
        ("LoRa 2.4 GHz ToF", "l2", "S2_n8", "static_2p4"),
        (DUAL, "l2", "F_n8", "lora_fusion"),
        (DUALR, "l2", "FR_n8", "lora_fusion"),
    ]),
]
REPRO = [  # (this campaign's cell, the earlier campaign's root and cell, arms)
    ("X_main", "cf", "A_main", ["ca_tbl_full", "static_868", "static_2p4", "poucet_fusion"]),
    ("X_end", "cf", "B_end", ["ca_tbl_full", "static_868", "static_2p4", "poucet_fusion"]),
    ("X_rssi", "lc", "R_main", ["static_868"]),
]


# Amendment (PREREG_LORA_CMP2 sect 7, after the registered check fired): the snapshot
# multilateration diagnostic's two error columns are left out of the reproduction comparison.
# They amplify 1e-13 input differences through near-singular solves, and nothing here reads
# them. Every other column, mlat_solved included, must still reproduce byte-for-byte.
UNSTABLE = {"rmse_mlat", "cep95_mlat"}


def lines(p, arm):
    try:
        rows = list(csv.reader(open(p)))
    except OSError:
        return []
    keep = [i for i, h in enumerate(rows[0]) if h not in UNSTABLE]
    return [[r[i] for i in keep] for r in rows[1:] if r and r[0] == arm]


def checks(roots, problems):
    """The registered abort conditions (PREREG_LORA_CMP2 sect 5)."""
    l2 = roots["l2"]
    rcs = glob.glob(os.path.join(l2, "*", "s*", "rc"))
    if len(rcs) != 143:
        problems.append(f"{len(rcs)} cells with an rc, not 143")
    for p in rcs:
        if open(p).read().strip() != "0":
            problems.append(f"{os.path.dirname(p)} exit")
    srcs = set()
    for p in glob.glob(os.path.join(l2, "*", "s*", "run_id.txt")):
        for ln in open(p):
            if ln.startswith("source "):
                srcs.add(" ".join(ln.split()[:4]))
    if len(srcs) != 1 or not next(iter(srcs)).endswith("dirty 0"):
        problems.append(f"run_id source lines {sorted(srcs)}")
    for mine, r, theirs, arms in REPRO:
        for f in ("arm_summary.csv", "subject_summary.csv"):
            for arm in arms:
                a = lines(os.path.join(roots[r], theirs, "s1", f), arm)
                b = lines(os.path.join(l2, mine, "s1", f), arm)
                if not a or a != b:
                    problems.append(f"{mine}: {arm}'s {f} rows differ from {theirs}/s1 (the pairing across binaries fails)")
    for cell in ("F_main", "F_end", "F_n8"):
        n = sum(len(glob.glob(os.path.join(d, "tdoa_lora_fusion_*"))) for d in cs.seeds(l2, cell))
        if n != 20:
            problems.append(f"{cell}: tdoa_lora_fusion files {n}, not 20 (TDoA not on)")
    for cell in ("FR_main", "FR_end", "FR_n8"):
        n = sum(len(glob.glob(os.path.join(d, "tdoa_*"))) for d in cs.seeds(l2, cell))
        if n:
            problems.append(f"{cell}: {n} tdoa_ files with --tier1Tdoa=false")
    for cell in ("F_main", "F_end", "F_n8", "FR_main", "FR_end", "FR_n8"):
        for d in cs.seeds(l2, cell):
            a = cs.arm_row(d, "lora_fusion")
            if a is None or float(a["tx_uwb"]) != 0.0 or float(a["j_2p4"]) <= 0.0:
                problems.append(f"{cell}/{os.path.basename(d)}: lora_fusion row missing, UWB transmitted, or 2.4 GHz unbilled")


def paired(v, ref, k):
    s = sorted(set(v) & set(ref))
    return cs.ci([v[i][k] - ref[i][k] for i in s]), len(s)


def main():
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    roots = {"l2": sys.argv[1], "lc": sys.argv[2], "cf": sys.argv[3]}
    problems = []
    checks(roots, problems)
    md = ["# LORA-CMP-2 — CA-TBL against LoRa localisation on both LoRa bands (scripts/lora_cmp2_analyse.py)", ""]
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
            if not v or not ref:
                continue
            (a, n), (c, _), (j, _) = paired(v, ref, "av"), paired(v, ref, "c95"), paired(v, ref, "J")
            md.append(f"| {name} | {cs.fmt(*a)} | {cs.fmt(*c, p=1)} | {cs.fmt(*j, p=1)} | {n} |")
        by = {name: v for name, v, _ in data}
        if POUCET in by and DUAL in by and by[POUCET] and by[DUAL]:
            (a, n), (c, _), (j, _) = paired(by[POUCET], by[DUAL], "av"), paired(by[POUCET], by[DUAL], "c95"), \
                paired(by[POUCET], by[DUAL], "J")
            md += ["", f"What UWB adds to Poucet (poucet_fusion − lora_fusion with TDoA, paired, {n} seeds): "
                   f"availability {cs.fmt(*a)}, CEP95 tag 0 {cs.fmt(*c, p=1)} m, subject J {cs.fmt(*j, p=1)}."]
        md.append("")
    with open(os.path.join(roots["l2"], "lora_cmp2_table.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems]
    open(os.path.join(roots["l2"], "LORA_CMP2.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
