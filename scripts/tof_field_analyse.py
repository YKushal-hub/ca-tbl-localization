#!/usr/bin/env python3
"""TOF-FIELD analysis (report/PREREG_TOF_FIELD.md): what the 2.4 GHz ToF ranging error at
measured outdoor levels does to every arm that consumes tier-2 ranges, on the 102-setting preset.

usage: tof_field_analyse.py <outdir written by scripts/tof_field_run.sh>

Writes <outdir>/TOF_FIELD.md and <outdir>/tof_field_table.csv, prints the same, and exits 1 if
any registered abort condition (PREREG sect 6) holds.
"""
import csv
import glob
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402
import numpy as np  # noqa: E402

SETTINGS = {"D0": "default (3.0 m floor, no bias)", "R1": "rural: sigma 83.6 m, bias +46.4 m",
            "R0": "rural spread, bias removed: sigma 83.6 m, bias 0", "S": "suburban: sigma 45 m, bias +39 m"}
CONFIGS = {"A": ("5 km (A_main)", "main"), "B": ("endgame (B_end)", "end")}
ARMS = {"ca_tbl_full": "CA-TBL (preset)", "fixed_threshold": "fixed threshold (same preset)",
        "static_2p4": "static 2.4 GHz", "lora_fusion": "dual-band LoRa fusion",
        "poucet_fusion": "Poucet (LoRa + UWB fused)"}
PAIRS = (("ca_tbl_full", "fixed_threshold"), ("ca_tbl_full", "lora_fusion"),
         ("ca_tbl_full", "poucet_fusion"), ("static_2p4", "ca_tbl_full"))
NCELLS = 160


def cell(cfg, st):
    return f"X_{CONFIGS[cfg][1]}_{st}"


def checks(out, problems):
    rcs = glob.glob(os.path.join(out, "*", "s*", "rc"))
    if len(rcs) != NCELLS:
        problems.append(f"{len(rcs)} cells with an rc, not {NCELLS}")
    for p in rcs:
        if open(p).read().strip() != "0":
            problems.append(f"{os.path.dirname(p)} exit {open(p).read().strip()}")
    srcs = set()
    for p in glob.glob(os.path.join(out, "*", "s*", "run_id.txt")):
        for ln in open(p):
            if ln.startswith("source "):
                srcs.add(" ".join(ln.split()[:4]))
    if len(srcs) != 1 or not next(iter(srcs)).endswith("dirty 0"):
        problems.append(f"run_id source lines {sorted(srcs)}")


def fixes_reached(out, problems):
    """Rule 4: in every configuration and setting, both switching arms ranged a dual LoRa base
    and had a TDoA position judged by the innovation gate (fused or refused) on some seed, so the
    preset's fixes are live in both."""
    for cfg in CONFIGS:
        for st in SETTINGS:
            for arm in ("ca_tbl_full", "fixed_threshold"):
                dual = judged = 0
                for d in cs.seeds(out, cell(cfg, st)):
                    try:
                        t = open(os.path.join(d, "run.log")).read()
                    except OSError:
                        continue
                    ln = next((x for x in t.splitlines() if f"N.26 fixes ({arm})" in x), None)
                    if ln is None:
                        problems.append(f"{d}: no N.26 line for {arm}")
                        continue
                    dual += int(ln.split("dual LoRa base node-cycles ")[1].split(",")[0])
                    judged += int(ln.split("TDoA fused ")[1].split(";")[0])
                    judged += int(ln.split("TDoA innovation gate refused ")[1].split(",")[0])
                if dual == 0 or judged == 0:
                    problems.append(f"{cell(cfg, st)}/{arm}: dual-base node-cycles {dual}, TDoA judged {judged} (need both > 0)")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems = []
    checks(out, problems)
    fixes_reached(out, problems)
    md = ["# TOF-FIELD — 2.4 GHz ToF ranging at measured outdoor error (scripts/tof_field_analyse.py)", "",
          "Settings: " + "; ".join(f"{k} = {v}" for k, v in SETTINGS.items()) + ".", "",
          "Mean [95 % bootstrap] over seeds 1-20; differences paired by seed.", ""]
    rows, data, q = [], {}, {}
    for cfg in CONFIGS:
        for st in SETTINGS:
            for arm in ARMS:
                v = per_seed(out, cell(cfg, st), arm)
                if len(v) != 20:
                    problems.append(f"{cell(cfg, st)}/{arm}: {len(v)} seeds, not 20")
                data[(cfg, st, arm)] = v
    for cfg, (title, _) in CONFIGS.items():
        md += [f"## {title}", ""]
        for st, sname in SETTINGS.items():
            md += [f"### {st}: {sname}", "",
                   "| arm | availability | CEP50 / CEP95 tag 0 (m) | CEP95 subjects (m) | subject J |", "|---|---|---|---|---|"]
            for arm, name in ARMS.items():
                v = data[(cfg, st, arm)]
                if not v:
                    continue
                col = lambda k: [x[k] for x in v.values()]  # noqa: E731
                av = cs.ci(col("av"))
                md.append(f"| {name} | {cs.fmt(*av)} | {np.mean(col('c50')):.1f} / {np.mean(col('c95')):.1f} | "
                          f"{np.nanmean(col('s95')):.1f} | {np.mean(col('J')):.2f} |")
                rows.append(dict(config=cfg, setting=st, arm=arm, availability=f"{av[0]:.4f}",
                                 availability_lo=f"{av[1]:.4f}", availability_hi=f"{av[2]:.4f}",
                                 cep50_tag0_m=f"{np.mean(col('c50')):.2f}", cep95_tag0_m=f"{np.mean(col('c95')):.2f}",
                                 cep95_subjects_m=f"{np.nanmean(col('s95')):.2f}", subject_j=f"{np.mean(col('J')):.3f}"))
            md.append("")
        md += [f"Paired, setting − D0, per arm ({title}):", "",
               "| arm | setting | Δ availability | Δ CEP95 tag 0 (m) | CEP95 ratio − 1 | Δ subject J |", "|---|---|---|---|---|---|"]
        for arm, name in ARMS.items():
            ref = data[(cfg, "D0", arm)]
            for st in ("R1", "R0", "S"):
                v = data[(cfg, st, arm)]
                s = sorted(set(v) & set(ref))
                if not s:
                    continue
                dav = cs.ci([v[i]["av"] - ref[i]["av"] for i in s])
                dc = cs.ci([v[i]["c95"] - ref[i]["c95"] for i in s])
                rat = float(np.mean([v[i]["c95"] / ref[i]["c95"] - 1.0 for i in s]))
                dj = cs.ci([v[i]["J"] - ref[i]["J"] for i in s])
                q[(cfg, st, arm)] = (dav, dc, rat)
                md.append(f"| {name} | {st} | {cs.fmt(*dav)} | {cs.fmt(*dc, p=1)} | {rat:+.3f} | {cs.fmt(*dj, p=2)} |")
        md += ["", f"Paired comparisons within each setting ({title}):", "",
               "| comparison | setting | Δ availability | Δ CEP95 tag 0 (m) | Δ subject J |", "|---|---|---|---|---|"]
        for a, b in PAIRS:
            for st in SETTINGS:
                va, vb = data[(cfg, st, a)], data[(cfg, st, b)]
                s = sorted(set(va) & set(vb))
                if not s:
                    continue
                dav = cs.ci([va[i]["av"] - vb[i]["av"] for i in s])
                dc = cs.ci([va[i]["c95"] - vb[i]["c95"] for i in s])
                dj = cs.ci([va[i]["J"] - vb[i]["J"] for i in s])
                q[(cfg, st, a, b)] = (dav, dc)
                md.append(f"| {ARMS[a]} − {ARMS[b]} | {st} | {cs.fmt(*dav)} | {cs.fmt(*dc, p=1)} | {cs.fmt(*dj, p=2)} |")
        md.append("")

    # The registered predictions (PREREG sect 5), evaluated as written.
    def above0(iv):
        return not math.isnan(iv[1]) and iv[1] > 0.0

    def below0(iv):
        return not math.isnan(iv[2]) and iv[2] < 0.0

    def inc0(iv):
        return not math.isnan(iv[1]) and iv[1] <= 0.0 <= iv[2]

    def mean_c95(cfg, st, arm):
        return float(np.mean([x["c95"] for x in data[(cfg, st, arm)].values()]))

    V = []
    try:
        V.append(("Q1", all(above0(q[(c, "R1", "static_2p4")][1]) for c in CONFIGS)))
        V.append(("Q2", all(above0(q[(c, "R0", "static_2p4")][1]) for c in CONFIGS)))
        V.append(("Q3", all(abs(q[(c, "R1", a)][0][0]) <= 0.02 for c in CONFIGS
                            for a in ("static_2p4", "lora_fusion", "poucet_fusion"))))
        V.append(("Q4", above0(q[("B", "R1", "ca_tbl_full")][1])))
        V.append(("Q5", above0(q[("A", "R1", "ca_tbl_full")][1])))
        V.append(("Q6", all(above0(q[(c, st, "ca_tbl_full", "lora_fusion")][0]) for c in CONFIGS for st in SETTINGS)))
        V.append(("Q7", all(above0(q[(c, st, "ca_tbl_full", "poucet_fusion")][0]) for c in CONFIGS for st in SETTINGS)))
        V.append(("Q8", below0(q[("B", "R1", "ca_tbl_full", "lora_fusion")][1])))
        V.append(("Q9", all(inc0(q[(c, "D0", "ca_tbl_full", "fixed_threshold")][0]) for c in CONFIGS)))
        V.append(("Q10", all(inc0(q[(c, "D0", "ca_tbl_full", "fixed_threshold")][1]) for c in CONFIGS)))
        V.append(("Q11", all(inc0(q[(c, "R1", "ca_tbl_full", "fixed_threshold")][0])
                             and inc0(q[(c, "R1", "ca_tbl_full", "fixed_threshold")][1]) for c in CONFIGS)))
        V.append(("Q12", below0(q[("B", "D0", "static_2p4", "ca_tbl_full")][1])))
        V.append(("Q13", all(not below0(q[(c, "R1", "static_2p4", "ca_tbl_full")][1]) for c in CONFIGS)))
        V.append(("Q14", mean_c95("B", "R1", "ca_tbl_full") > 2.0 * mean_c95("B", "D0", "ca_tbl_full")))
    except (TypeError, KeyError, ValueError, IndexError) as e:
        problems.append(f"a prediction could not be evaluated: {e!r}")
    md += ["## Registered predictions", ""] + [f"- {k}: {'HELD' if v else 'FAILED'}" for k, v in V] + [""]

    if rows:
        with open(os.path.join(out, "tof_field_table.csv"), "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=list(rows[0].keys()), lineterminator="\n")
            w.writeheader()
            w.writerows(rows)
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems]
    open(os.path.join(out, "TOF_FIELD.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
