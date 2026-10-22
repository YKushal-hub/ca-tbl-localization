#!/usr/bin/env python3
"""Publication tables for the no-hardware options (TDOA-TIMING, TDOA-TIMING-B, ORACLE-ESC, ML-PATHLOSS,
ML-PATHLOSS-POS). Writes report/paper_tables/nohw_tables.tex (compact, main text) and nohw_tables.md (with 95 %
intervals). Simulator numbers are recomputed from the tracked per-cell summaries with the paper tables' own per-seed
definitions and interval routine (scripts/paper_tables.py: per_seed, paired); the real-data numbers are read from the
two Bonn scripts' generated outputs. Nothing is retyped.

usage: python3 scripts/paper_tables_nohw.py

Colour code: a cell is green when the option improves the method (availability higher, CEP95 and energy lower) and
the paired 95 % interval excludes 0, red when it worsens it and the interval excludes 0, uncoloured otherwise.
"""
import os
import re
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
from lora_cmp_analyse import per_seed  # noqa: E402
from paper_tables import TEXCOL, paired  # noqa: E402

ROOT = os.path.join(HERE, "..")
R = {k: os.path.join(ROOT, "results", v) for k, v in {
    "tt": "tdoa_timing_2026-10-21", "tb": "tdoa_timing_b_2026-10-22", "oe": "oracle_esc_2026-10-21",
    "mp": "ml_pathloss_2026-10-21", "pp": "ml_pathloss_pos_2026-10-21"}.items()}
OUT = os.path.join(ROOT, "report", "paper_tables")
CLAB = {"A": "5 km", "B": "Endgame", "C": "8 GW"}
METRICS = [("av", "$\\Delta$Avail. (pp)", "ΔAvail (pp)", True, 1, 100.0),
           ("c95", "$\\Delta$CEP95 (m)", "ΔCEP95 (m)", False, 1, 1.0),
           ("J", "$\\Delta$E (J)", "ΔE (J)", False, 2, 1.0)]
# (paper row, arm, {configuration: campaign}); CA-TBL at 5 km and in the endgame is TDOA-TIMING's (seeds 341-360),
# everything else TDOA-TIMING-B's (seeds 361-380).
TIMING_ROWS = [("CA-TBL (base)", "ca_tbl_full", {"A": "tt", "B": "tt", "C": "tb"}),
               ("Fixed threshold", "fixed_threshold", {"A": "tb", "B": "tb"}),
               ("Poucet-style fusion", "poucet_fusion", {"A": "tb", "B": "tb"}),
               ("LoRa dual-band fusion", "lora_fusion", {"A": "tb", "B": "tb", "C": "tb"}),
               ("LoRa-868 RSSI+TDoA", "static_868", {"A": "tb", "B": "tb", "C": "tb"}),
               ("LoRa-2.4 ToF", "static_2p4", {"A": "tb", "B": "tb"})]
TIMING_BLOCKS = [("Perfect gateway sync (S0 $-$ L): ceiling for nanosecond sync", "S0", "L"),
                 ("Perfect arrival timing (T0 $-$ L): ceiling for two-pass CFO/STO refinement", "T0", "L"),
                 ("Tier-1 TDoA as configured (L $-$ N): what better timing acts on", "L", "N")]
THRESHOLDS = (3, 12, 50, 200)


def cell(t, hb, p, sc):
    m, lo, hi = t
    cls = "" if not (lo > 0 or hi < 0) else ("fav" if (m > 0) == hb else "unf")
    if cls and round(m * sc, p) == 0:
        p = 3  # an interval that excludes 0 around a mean that rounds to 0: show the digits that do
    return f"{m * sc:+.{p}f}", f"{m * sc:+.{p}f} [{lo * sc:+.{p}f}, {hi * sc:+.{p}f}]", cls


def md_mark(cls):
    return {"fav": "🟩", "unf": "🟥"}.get(cls, "")


def h(t):
    return t.replace("$-$", "−").replace("$\\Delta$", "Δ").replace("\\%", "%")


def timing():
    rows = []
    for title, hi, lo in TIMING_BLOCKS:
        block = []
        for name, arm, src in TIMING_ROWS:
            cells = []
            for k in CLAB:
                if k not in src:
                    cells += [None] * len(METRICS)
                    continue
                a = per_seed(R[src[k]], f"{k}_{hi}", arm)
                b = per_seed(R[src[k]], f"{k}_{lo}", arm)
                assert len(a) == 20 and len(b) == 20, (k, name, hi, lo, len(a), len(b))
                cells += [cell(paired(a, b, q), hb, p, sc) for q, _, _, hb, p, sc in METRICS]
            block.append((name, cells))
        rows.append((title, block))
    return rows


def oracle():
    rows = []
    for t in THRESHOLDS:
        cells = []
        for k in ("A", "B"):
            o = per_seed(R["oe"], f"{k}_O_T{t}", "fixed_threshold")
            r = per_seed(R["oe"], f"{k}_K_T{t}", "fixed_threshold")
            assert len(o) == 20 and len(r) == 20, (k, t)
            cells += [cell(paired(o, r, q), hb, p, sc) for q, _, _, hb, p, sc in METRICS]
        rows.append((f"$T = {t}$", cells))
    return rows


def md_rows(path):
    """{first cell: [other cells]} of the markdown table rows in a generated file; the first row wins."""
    out = {}
    for ln in open(path):
        c = [x.strip() for x in ln.strip().strip("|").split("|")]
        if ln.startswith("|") and len(c) > 1 and not set(c[0]) <= set("-"):
            out.setdefault(c[0], c[1:])
    return out


def bonn():
    rng = md_rows(os.path.join(R["mp"], "ML_PATHLOSS.md"))
    pos = md_rows(os.path.join(R["pp"], "ML_PATHLOSS_POS.md"))
    names = [("C1 global LDPL inverted", "Log-distance inversion, global (as LoRa-868 RSSI)"),
             ("C2 per-gateway LDPL inverted", "Log-distance inversion, per gateway"),
             ("C3 per-gateway inverse regression (RSSI)", "Linear regression on RSSI, per gateway"),
             ("C4 per-gateway inverse regression (RSSI, SNR)", "Linear regression on RSSI and SNR, per gateway"),
             ("ML gradient boosting (RSSI, SNR, gateway)", "Gradient-boosted trees (ML)"),
             ("G0 centroid of receiving gateways", "Centroid of receiving gateways (no ranging)")]
    rows = []
    for key, label in names:
        r = rng.get(key)
        p = pos[key]
        rows.append((label, r[1].replace("x/÷", "") if r else "--", r[2] if r else "--", p[0], p[1]))
    con = {k: v for k, v in pos.items() if k.startswith(("ML -", "C4 -"))}
    head = [ln for ln in open(os.path.join(R["pp"], "ML_PATHLOSS_POS.md")) if ln.strip()][1].strip()
    rhead = [ln for ln in open(os.path.join(R["mp"], "ML_PATHLOSS.md")) if ln.strip()][1].strip()
    rcon = [ln.strip() for ln in open(os.path.join(R["mp"], "ML_PATHLOSS.md")) if ln.startswith("ML - best classical")]
    return rows, con, head, rhead, rcon


def main():
    tm, oc = timing(), oracle()
    br, bcon, bhead, rhead, rcon = bonn()
    grp3 = " & ".join(f"\\multicolumn{{3}}{{c}}{{{CLAB[k]}}}" for k in CLAB)
    sub3 = " & ".join(" & ".join(m[1] for m in METRICS) for _ in CLAB)
    tex = ["% Generated by scripts/paper_tables_nohw.py. Preamble as tables.tex: \\usepackage{booktabs}",
           "% \\usepackage[table]{xcolor} \\definecolor{fav}{HTML}{D4EDDA} \\definecolor{unf}{HTML}{F8D7DA}",
           "% 95 % intervals: nohw_tables.md.", ""]
    md = ["# No-hardware options (generated by scripts/paper_tables_nohw.py; do not edit)", "",
          "Paired differences, option minus without it, mean of 20 seeds with 95 % seed-bootstrap intervals; "
          "🟩 the option improves the method, 🟥 it worsens it (interval excludes 0).", ""]
    # ---- Table 5: tier-1 timing
    tex += ["\\begin{table*}[t]", "\\centering", "\\footnotesize", "\\setlength{\\tabcolsep}{2.5pt}",
            "\\caption{Better tier-1 (868\\,MHz) arrival timing, paired over 20 seeds: each ceiling minus the preset. "
            "Shaded where the 95\\,\\% CI excludes 0: \\colorbox{fav}{the option improves the method}, "
            "\\colorbox{unf}{it worsens it}. --: configuration not run for the method.}", "\\label{tab:nohw-timing}",
            "\\begin{tabular}{l" + "ccc" * 3 + "}", "\\toprule", "Method & " + grp3 + " \\\\",
            "".join(f"\\cmidrule(lr){{{2 + 3 * i}-{4 + 3 * i}}}" for i in range(3)), " & " + sub3 + " \\\\"]
    md += ["## Table 5 — Better tier-1 (868 MHz) arrival timing", "",
           "| Method | " + " | ".join(f"{CLAB[k]} {m[2]}" for k in CLAB for m in METRICS) + " |",
           "|---|" + "---|" * 9]
    for title, block in tm:
        tex += ["\\midrule", f"\\multicolumn{{10}}{{l}}{{\\emph{{{title}}}}} \\\\"]
        md.append(f"| *{h(title)}* |" + " |" * 9)
        for name, cells in block:
            tex.append(name + " & " + " & ".join("--" if c is None else TEXCOL[c[2]] + c[0] for c in cells) + " \\\\")
            md.append(f"| {name} | " + " | ".join("–" if c is None else md_mark(c[2]) + c[1] for c in cells) + " |")
    tex += ["\\bottomrule", "\\end{tabular}",
            "\\\\[2pt]\\footnotesize L: the preset (GPS-PPS 20\\,ns, arrival time at the one-way Cram\\'er--Rao "
            "bound). S0: PPS error 0. T0: PPS error 0 and arrival-time error at 1\\,\\% of the bound, better than any "
            "receiver, so T0 is a ceiling. N: tier-1 TDoA off. CA-TBL at 5\\,km and in the endgame: seeds 341--360; "
            "all other cells: seeds 361--380.", "\\end{table*}", ""]
    md += ["", "L: preset (PPS 20 ns, arrival time at the one-way Cramér–Rao bound). S0: PPS error 0. T0: PPS error 0 "
           "and arrival-time error at 1 % of the bound (a ceiling). N: tier-1 TDoA off. CA-TBL at 5 km and in the "
           "endgame: seeds 341–360 (TDOA-TIMING); all other cells: seeds 361–380 (TDOA-TIMING-B).", ""]
    # ---- Table 6: oracle escalation input
    grp2 = " & ".join(f"\\multicolumn{{3}}{{c}}{{{CLAB[k]}}}" for k in ("A", "B"))
    sub2 = " & ".join(" & ".join(m[1] for m in METRICS) for _ in ("A", "B"))
    tex += ["\\begin{table*}[t]", "\\centering", "\\small", "\\setlength{\\tabcolsep}{4pt}",
            "\\caption{Perfect error knowledge in the escalation trigger: Fixed threshold escalating on the true "
            "squared position error minus escalating on $\\mathrm{trace}(P)$, at the same threshold $T$ (m$^2$), "
            "paired over 20 seeds. Shading as Table~\\ref{tab:nohw-timing}.}", "\\label{tab:nohw-oracle}",
            "\\begin{tabular}{l" + "ccc" * 2 + "}", "\\toprule", "Threshold & " + grp2 + " \\\\",
            "".join(f"\\cmidrule(lr){{{2 + 3 * i}-{4 + 3 * i}}}" for i in range(2)), " & " + sub2 + " \\\\",
            "\\midrule"]
    md += ["## Table 6 — Perfect error knowledge in the escalation trigger (Fixed threshold)", "",
           "Oracle (true squared error) minus trace(P), same threshold T (m²), seeds 321–340 (ORACLE-ESC).", "",
           "| Threshold | " + " | ".join(f"{CLAB[k]} {m[2]}" for k in ("A", "B") for m in METRICS) + " |",
           "|---|" + "---|" * 6]
    for name, cells in oc:
        tex.append(name + " & " + " & ".join(TEXCOL[c[2]] + c[0] for c in cells) + " \\\\")
        md.append(f"| {name.replace('$', '')} | " + " | ".join(md_mark(c[2]) + c[1] for c in cells) + " |")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table*}", ""]
    md.append("")
    # ---- Table 7: learned 868 MHz ranging on real data
    tex += ["\\begin{table*}[t]", "\\centering", "\\small", "\\setlength{\\tabcolsep}{4pt}",
            "\\caption{Learned 868\\,MHz ranging on real measurements (Bonn; Rademacher et al.\\ 2021), evaluated on "
            "1\\,km blocks unseen in training. Range: one packet, one gateway. Position: least-squares "
            "multilateration of each packet heard by $\\geq 3$ gateways.}", "\\label{tab:nohw-ml}",
            "\\begin{tabular}{lcccc}", "\\toprule",
            "Range model & \\multicolumn{2}{c}{Range} & \\multicolumn{2}{c}{Position} \\\\",
            "\\cmidrule(lr){2-3}\\cmidrule(lr){4-5}",
            " & 68\\,\\% factor & median (m) & median (m) & 68\\,\\% (m) \\\\", "\\midrule"]
    md += ["## Table 7 — Learned 868 MHz ranging on real measurements (Bonn)", "", f"Range: {rhead}", "",
           f"Position: {bhead}", "",
           "| Range model | Range 68 % factor | Range median error (m) | Position median error (m) | "
           "Position 68 % error (m) |", "|---|---|---|---|---|"]
    for label, f68, rmed, pmed, p68 in br:
        tex.append(f"{label} & {f68} & {rmed} & {pmed} & {p68} \\\\")
        md.append(f"| {label} | {f68} | {rmed} | {pmed} | {p68} |")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table*}", ""]
    md += ["", "Contrasts (block-bootstrap 95 %):", ""] + [f"- Range: {ln}" for ln in rcon] + \
          [f"- Position, {k}: {v[0]} (median error ratio {v[1]}), {v[2]}" for k, v in bcon.items()] + [""]
    os.makedirs(OUT, exist_ok=True)
    open(os.path.join(OUT, "nohw_tables.tex"), "w").write("\n".join(tex))
    open(os.path.join(OUT, "nohw_tables.md"), "w").write("\n".join(md))
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    sys.exit(main())
