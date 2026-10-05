#!/usr/bin/env python3
"""Publication tables from the registered campaigns (CAMP-FINAL, LORA-CMP, LORA-CMP-2,
FIXES-N26). Writes report/paper_tables/tables.tex (LaTeX: booktabs + xcolor[table]),
tables.md and preview.html. Every number is recomputed from the tracked per-cell summaries
with the campaigns' own per-seed definitions (scripts/lora_cmp_analyse.py per_seed); nothing is
retyped.

usage: python3 scripts/paper_tables.py

Colour code: a cell is green when it favours CA-TBL (+ fixes, or the fix in the ablation)
and its paired 95 % interval excludes 0, red when it disfavours it and the interval excludes 0,
uncoloured when the interval includes 0. "Favours" is per column: availability higher, CEP and
energy lower. Table 1 bolds the best value of each column within a configuration. Nothing is
dropped for being unfavourable.

Intervals are 95 % seed-bootstrap intervals of the mean (10 000 resamples). Each interval
draws from its own generator seeded 20261005, so a value does not depend on which others were
computed first.
"""
import glob
import html
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, HERE)
import camp_spec_analyse as cs  # noqa: E402
from camp_final_analyse import uwb_fixes  # noqa: E402
from lora_cmp_analyse import per_seed  # noqa: E402
import numpy as np  # noqa: E402

ROOT = os.path.join(HERE, "..")
R = {k: os.path.join(ROOT, "results", v) for k, v in {
    "cf": "camp_final_2026-10-04", "lc": "lora_cmp_2026-10-05",
    "l2": "lora_cmp2_2026-10-05", "ev": "fixes_n26_2026-10-05/eval"}.items()}
OUT = os.path.join(ROOT, "report", "paper_tables")
CFG = [("A", "5 km, 3 gateways"), ("B", "Endgame, finders converged"), ("C", "8 gateways (ring)")]
CELL = {"A": ("A_main", "R_main", "F_main", "_main", "G_single", "A_main"),
        "B": ("B_end", "R_end", "F_end", "_end", "G_single_end", "B_end"),
        "C": ("C_n8", "R_n8", "F_n8", "_n8", None, "S2_n8")}
PROPOSED = "CA-TBL + N.26 fixes"
FAULTS = [("rb_common", "Common-mode range bias, 30 m"), ("rb_single", "Single-anchor range bias, 30 m"),
          ("txdrop", "Transmit-power drop, 6 dB"), ("antenna", "Antenna loss, 10 dB")]
GREEN, RED, HL = "#d4edda", "#f8d7da", "#eef4fb"


def ci(v):
    v = np.asarray([x for x in v if not np.isnan(x)], dtype=float)
    rng = np.random.default_rng(20261005)
    m = v[rng.integers(0, len(v), size=(10000, len(v)))].mean(axis=1)
    return float(v.mean()), float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975))


def methods(k):
    cf, lc, l2, suf, gs, s2 = CELL[k]
    rows = [(PROPOSED, "ev", "ALL" + suf, "ca_tbl_full"),
            ("CA-TBL (preset)", "cf", cf, "ca_tbl_full")]
    if k != "C":
        rows += [("Poucet-style fusion (LoRa + UWB)", "cf", cf, "poucet_fusion"),
                 ("Fixed-threshold switching", "cf", cf, "fixed_threshold")]
    rows += [("LoRa dual-band fusion (868 + 2.4 GHz)", "l2", l2, "lora_fusion"),
             ("LoRa 868 MHz RSSI + TDoA", "cf", cf, "static_868"),
             ("LoRa 868 MHz RSSI", "lc", lc, "static_868"),
             ("LoRa 2.4 GHz ToF", "l2" if k == "C" else "cf", s2, "static_2p4")]
    if gs:
        rows += [("Single-hop confidence (LoRaWAPS-style)", "cf", gs, "ca_tbl_full")]
    return rows


def paired(a, b, key):
    s = sorted(set(a) & set(b))
    return ci([a[i][key] - b[i][key] for i in s])


# A rendered cell: text, and a colour class in {"", "fav", "unf", "best", "hl"} combinations.
def dcell(t, p, higher_better):
    """A paired difference, coloured by whether it favours CA-TBL (+ fixes)."""
    m, lo, hi = t
    txt = f"{m:+.{p}f} [{lo:+.{p}f}, {hi:+.{p}f}]"
    if lo > 0 or hi < 0:
        return txt, ("fav" if (m > 0) == higher_better else "unf")
    return txt, ""


def table1():
    out = []
    for k, title in CFG:
        block = []
        for name, r, c, arm in methods(k):
            v = per_seed(R[r], c, arm)
            assert len(v) == 20, (k, name, len(v))
            col = lambda q: [x[q] for x in v.values()]  # noqa: E731
            u = "--"
            if arm == "ca_tbl_full" and r in ("cf", "ev") and not c.startswith("G_"):
                n = [uwb_fixes(d, int(d.rsplit("s", 1)[1])) for d in cs.seeds(R[r], c)]
                u = f"{sum(n)} ({sum(x > 0 for x in n)})"
            block.append(dict(name=name, av=ci(col("av")), c50=float(np.mean(col("c50"))),
                              c95=float(np.mean(col("c95"))), J=float(np.mean(col("J"))),
                              jpf=float(np.nanmean(col("jpf"))), uwb=u))
        best = dict(av=max(b["av"][0] for b in block), c50=min(b["c50"] for b in block),
                    c95=min(b["c95"] for b in block), J=min(b["J"] for b in block),
                    jpf=min(b["jpf"] for b in block))
        out.append((k, title, block, best))
    return out


def table2():
    out = []
    for k, title in CFG:
        ms = methods(k)
        allv = per_seed(R["ev"], ms[0][2], "ca_tbl_full")
        block = []
        for name, r, c, arm in ms[1:]:
            v = per_seed(R[r], c, arm)
            block.append((name, dcell(paired(allv, v, "av"), 3, True), dcell(paired(allv, v, "c95"), 1, False),
                          dcell(paired(allv, v, "J"), 2, False), dcell(paired(allv, v, "jpf"), 3, False)))
        out.append((k, title, block))
    return out


def table3():
    out = []
    fixes = [("Dual LoRa base (both LoRa bands fused)", "D"), ("UWB cross-tier polling", "U"),
             ("TDoA innovation gate", "T"), ("Integrity: correct covariance + fitted threshold", "M"),
             ("All four", "ALL")]
    for k, title in CFG[:2]:
        base = per_seed(R["cf"], CELL[k][0], "ca_tbl_full")
        block = []
        for name, pre in fixes:
            v = per_seed(R["ev"], pre + CELL[k][3], "ca_tbl_full")
            block.append((name, dcell(paired(v, base, "av"), 3, True), dcell(paired(v, base, "c95"), 1, False),
                          dcell(paired(v, base, "J"), 2, False)))
        out.append((k, title, block))
    return out


def suspect_share(root, cell):
    n = s = 0
    for d in cs.seeds(root, cell):
        for r in cs.trace(d, "ca_tbl_full"):
            if r["hadFix"] == "1":
                n += 1
                s += r["suspect"] == "1"
    return s / n


def faults(root):
    res = {}
    for cell in sorted(glob.glob(os.path.join(root, "E_*_downweight"))):
        fault = os.path.basename(cell)[2:].rsplit("_", 1)[0]
        tp = fp = fn = 0
        for d in cs.seeds(root, os.path.basename(cell)):
            seed = int(d.rsplit("s", 1)[1])
            w0 = cs.DUR * (0.25 + 0.5 * (seed % 7) / 7.0)
            w1 = w0 + cs.FRAC * cs.DUR
            for r in cs.trace(d, "ca_tbl_full"):
                if r["hadFix"] != "1":
                    continue
                inw = w0 <= float(r["t"]) < w1
                sus = r["suspect"] == "1"
                tp += inw and sus
                fp += (not inw) and sus
                fn += inw and not sus
        res[fault] = (tp / (tp + fp) if tp + fp else float("nan"), tp / (tp + fn) if tp + fn else float("nan"))
    return res


def table4():
    pre = {"A": suspect_share(R["cf"], "A_main"), "B": suspect_share(R["cf"], "B_end")}
    new = {"A": suspect_share(R["ev"], "ALL_main"), "B": suspect_share(R["ev"], "ALL_end")}
    return pre, new, faults(R["cf"]), faults(R["ev"])


# ---------------------------------------------------------------- rendering
TEXCOL = {"fav": "\\cellcolor{fav}", "unf": "\\cellcolor{unf}", "": ""}


def tex_cell(txt, cls, bold=False):
    t = txt.replace("--", "--")
    if bold:
        t = "\\textbf{" + t + "}"
    return TEXCOL.get(cls, "") + t


def md_cell(txt, cls, bold=False):
    t = f"**{txt}**" if bold else txt
    return {"fav": "🟩 ", "unf": "🟥 "}.get(cls, "") + t


def html_cell(txt, cls, bold=False):
    st = {"fav": f"background:{GREEN}", "unf": f"background:{RED}"}.get(cls, "")
    return f"<td style='{st}'>{'<b>' if bold else ''}{html.escape(txt)}{'</b>' if bold else ''}</td>"


def write(t1, t2, t3, t4):
    os.makedirs(OUT, exist_ok=True)
    tex = ["% Generated by scripts/paper_tables.py from the tracked campaign summaries.",
           "% Preamble: \\usepackage{booktabs} \\usepackage[table]{xcolor}",
           "%           \\definecolor{fav}{HTML}{D4EDDA} \\definecolor{unf}{HTML}{F8D7DA} \\definecolor{hl}{HTML}{EEF4FB}",
           "% Intervals: 95 % seed-bootstrap of the mean, 20 seeds (fault matrix: 10).",
           "% Green / red: favours / disfavours CA-TBL (+ fixes) with the paired interval excluding 0.", ""]
    md, hb = [], []
    dag = "$^\\dagger$"

    # ---- Table 1
    tex += ["\\begin{table*}[t]", "\\centering", "\\small",
            "\\caption{Localisation performance on the CA-TBL simulator, 20 seeds per cell, 1\\,800\\,s. "
            "Availability: share of cycles with a position fix, mean over five tracked nodes. CEP: tag~0's "
            "circular error probable. Energy: mean per tracked node over the run; per fix: summed energy over "
            "summed fix cycles. UWB fixes: total over 20 seeds (seeds with any). Bold: best in the column "
            "within a configuration.}", "\\label{tab:performance}",
            "\\begin{tabular}{lcccccc}", "\\toprule",
            "Method & Availability [95\\,\\% CI] & CEP50 (m) & CEP95 (m) & Energy (J) & J per fix & UWB fixes \\\\",
            "\\midrule"]
    md += ["## Table 1 — Localisation performance (20 seeds, 1 800 s)", "",
           "| Method | Availability [95 % CI] | CEP50 (m) | CEP95 (m) | Energy (J) | J per fix | UWB fixes |",
           "|---|---|---|---|---|---|---|"]
    hb += ["<h2>Table 1 — Localisation performance (20 seeds, 1 800 s)</h2><table><tr><th>Method</th>"
           "<th>Availability [95 % CI]</th><th>CEP50 (m)</th><th>CEP95 (m)</th><th>Energy (J)</th><th>J per fix</th>"
           "<th>UWB fixes</th></tr>"]
    for i, (k, title, block, best) in enumerate(t1):
        if i:
            tex.append("\\midrule")
        tex.append(f"\\multicolumn{{7}}{{l}}{{\\textit{{({k}) {title}}}}} \\\\")
        md.append(f"| *({k}) {title}* | | | | | | |")
        hb.append(f"<tr class='g'><td colspan=7>({k}) {html.escape(title)}</td></tr>")
        for b in block:
            prop = b["name"] == PROPOSED
            vals = [(f"{b['av'][0]:.3f} [{b['av'][1]:.3f}, {b['av'][2]:.3f}]", b["av"][0] == best["av"]),
                    (f"{b['c50']:.1f}", b["c50"] == best["c50"]), (f"{b['c95']:.1f}", b["c95"] == best["c95"]),
                    (f"{b['J']:.2f}", b["J"] == best["J"]), (f"{b['jpf']:.3f}", b["jpf"] == best["jpf"]),
                    (b["uwb"], False)]
            rowc = "\\rowcolor{hl}" if prop else ""
            tex.append(rowc + ("\\textbf{" + b["name"] + "}" + dag if prop else b["name"]) + " & " +
                       " & ".join(tex_cell(t, "", bo) for t, bo in vals) + " \\\\")
            md.append(f"| {('**' + b['name'] + '†**') if prop else b['name']} | " +
                      " | ".join(md_cell(t.replace('--', '—'), '', bo) for t, bo in vals) + " |")
            hb.append(f"<tr class='{'p' if prop else ''}'><td>{html.escape(b['name'] + ('†' if prop else ''))}</td>" +
                      "".join(html_cell(t.replace('--', '—'), '', bo) for t, bo in vals) + "</tr>")
    note1 = ("Not adopted into the preset by the pre-registered rule: its energy at 5 km exceeds the preset's "
             "(Table 2). All methods run in the same simulator, deployment, channel and energy model.")
    tex += ["\\bottomrule", "\\end{tabular}", "",
            "\\vspace{2pt}\\footnotesize " + dag + note1.replace("5 km", "5\\,km"), "\\end{table*}", ""]
    md += ["", "† " + note1, ""]
    hb += [f"</table><p class='n'>† {html.escape(note1)}</p>"]

    # ---- Table 2
    hdr = ["Comparator", "Δ availability", "Δ CEP95 (m)", "Δ energy (J)", "Δ J per fix"]
    tex += ["\\begin{table*}[t]", "\\centering", "\\small",
            "\\caption{CA-TBL with the N.26 fixes minus each comparator, paired by seed (mean [95\\,\\% CI]). "
            "Green: favours CA-TBL + fixes (higher availability, lower CEP95 or energy), interval excluding 0. "
            "Red: favours the comparator. Uncoloured: no detectable difference.}", "\\label{tab:paired}",
            "\\begin{tabular}{lcccc}", "\\toprule",
            "Comparator & $\\Delta$ availability & $\\Delta$ CEP95 (m) & $\\Delta$ energy (J) & $\\Delta$ J per fix \\\\",
            "\\midrule"]
    md += ["## Table 2 — CA-TBL + N.26 fixes minus each comparator, paired by seed", "",
           "| " + " | ".join(hdr) + " |", "|---|---|---|---|---|"]
    hb += ["<h2>Table 2 — CA-TBL + N.26 fixes minus each comparator, paired by seed</h2><table><tr>" +
           "".join(f"<th>{h}</th>" for h in hdr) + "</tr>"]
    for i, (k, title, block) in enumerate(t2):
        if i:
            tex.append("\\midrule")
        tex.append(f"\\multicolumn{{5}}{{l}}{{\\textit{{({k}) {title}}}}} \\\\")
        md.append(f"| *({k}) {title}* | | | | |")
        hb.append(f"<tr class='g'><td colspan=5>({k}) {html.escape(title)}</td></tr>")
        for name, *cells in block:
            tex.append(name + " & " + " & ".join(tex_cell(t, c) for t, c in cells) + " \\\\")
            md.append(f"| {name} | " + " | ".join(md_cell(t, c) for t, c in cells) + " |")
            hb.append(f"<tr><td>{html.escape(name)}</td>" + "".join(html_cell(t, c) for t, c in cells) + "</tr>")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table*}", ""]
    md += ["", "🟩 favours CA-TBL + fixes, 🟥 favours the comparator (paired interval excludes 0); unmarked: no "
           "detectable difference.", ""]
    hb += ["</table><p class='n'><span class='k' style='background:" + GREEN + "'></span>favours CA-TBL + fixes "
           "<span class='k' style='background:" + RED + "'></span>favours the comparator (paired interval excludes 0); "
           "uncoloured: no detectable difference.</p>"]

    # ---- Table 3
    tex += ["\\begin{table}[t]", "\\centering", "\\small",
            "\\caption{Ablation: each N.26 fix alone, and all four, minus the CA-TBL preset, paired by seed "
            "(mean [95\\,\\% CI]). Green: an improvement; red: a cost; interval excluding 0.}", "\\label{tab:ablation}",
            "\\begin{tabular}{lccc}", "\\toprule",
            "Fix & $\\Delta$ availability & $\\Delta$ CEP95 (m) & $\\Delta$ energy (J) \\\\", "\\midrule"]
    md += ["## Table 3 — Ablation: each fix alone, and all four, minus the preset (paired)", "",
           "| Fix | Δ availability | Δ CEP95 (m) | Δ energy (J) |", "|---|---|---|---|"]
    hb += ["<h2>Table 3 — Ablation: each fix alone, and all four, minus the preset (paired)</h2><table><tr><th>Fix</th>"
           "<th>Δ availability</th><th>Δ CEP95 (m)</th><th>Δ energy (J)</th></tr>"]
    for i, (k, title, block) in enumerate(t3):
        if i:
            tex.append("\\midrule")
        tex.append(f"\\multicolumn{{4}}{{l}}{{\\textit{{({k}) {title}}}}} \\\\")
        md.append(f"| *({k}) {title}* | | | |")
        hb.append(f"<tr class='g'><td colspan=4>({k}) {html.escape(title)}</td></tr>")
        for name, *cells in block:
            tex.append(name + " & " + " & ".join(tex_cell(t, c) for t, c in cells) + " \\\\")
            md.append(f"| {name} | " + " | ".join(md_cell(t, c) for t, c in cells) + " |")
            hb.append(f"<tr><td>{html.escape(name)}</td>" + "".join(html_cell(t, c) for t, c in cells) + "</tr>")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table}", ""]
    md += ["", "🟩 improvement, 🟥 cost (paired interval excludes 0); unmarked: no detectable difference.", ""]
    hb += ["</table><p class='n'>Green: improvement; red: cost (paired interval excludes 0); uncoloured: no detectable "
           "difference.</p>"]

    # ---- Table 4: the fixes column coloured against the preset
    pre, new, fpre, fnew = t4
    rows = [("Fault-free flagged share, 5 km", f"{pre['A']:.3f}", f"{new['A']:.3f}", "fav" if new["A"] < pre["A"] else "unf"),
            ("Fault-free flagged share, endgame", f"{pre['B']:.3f}", f"{new['B']:.3f}", "fav" if new["B"] < pre["B"] else "unf")]
    frows = []
    for f, name in FAULTS:
        (pp, pr), (np_, nr) = fpre[f], fnew[f]
        frows.append((name, f"{pp:.3f}", f"{np_:.3f}", "fav" if np_ > pp else "unf",
                      f"{pr:.3f}", f"{nr:.3f}", "fav" if nr > pr else "unf"))
    tex += ["\\begin{table}[t]", "\\centering", "\\small",
            "\\caption{Integrity check. Fault-free: share of tag~0 fix cycles flagged. Injected faults (10 seeds, a "
            "25\\,\\% window, flagged fixes downweighted): precision and recall of the flag against the window. "
            "Green / red: better / worse with the fixes than the preset.}", "\\label{tab:integrity}",
            "\\begin{tabular}{lcccc}", "\\toprule",
            " & \\multicolumn{2}{c}{Preset} & \\multicolumn{2}{c}{With N.26 fixes} \\\\",
            "\\cmidrule(lr){2-3}\\cmidrule(lr){4-5}"]
    for name, a, b, c in rows:
        nm = name.replace("5 km", "5\\,km")
        tex.append(f"{nm} & \\multicolumn{{2}}{{c}}{{{a}}} & \\multicolumn{{2}}{{c}}{{{TEXCOL[c]}{b}}} \\\\")
    tex += ["\\midrule", " & Precision & Recall & Precision & Recall \\\\", "\\midrule"]
    for name, pp, np_, cp, pr, nr, cr in frows:
        tex.append(f"{name} & {pp} & {pr} & {TEXCOL[cp]}{np_} & {TEXCOL[cr]}{nr} \\\\")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table}", ""]
    md += ["## Table 4 — Integrity check", "", "| | Preset | With N.26 fixes |", "|---|---|---|"]
    md += [f"| {n} | {a} | {md_cell(b, c)} |" for n, a, b, c in rows]
    md += ["", "| Injected fault | Precision, preset | Recall, preset | Precision, fixes | Recall, fixes |",
           "|---|---|---|---|---|"]
    md += [f"| {n} | {pp} | {pr} | {md_cell(np_, cp)} | {md_cell(nr, cr)} |" for n, pp, np_, cp, pr, nr, cr in frows]
    md += ["", "The fitted threshold (1 992.9) removes the false alarms and, with them, detection: the open defect is "
           "the covariance's error tail (TECH_DEBT N.15).", ""]
    hb += ["<h2>Table 4 — Integrity check</h2><table><tr><th></th><th colspan=2>Preset</th><th colspan=2>With N.26 fixes"
           "</th></tr>"]
    hb += [f"<tr><td>{n}</td><td colspan=2>{a}</td>{html_cell(b, c).replace('<td', '<td colspan=2', 1)}</tr>"
           for n, a, b, c in rows]
    hb += ["<tr class='g'><td>Injected fault</td><td>Precision</td><td>Recall</td><td>Precision</td><td>Recall</td></tr>"]
    hb += [f"<tr><td>{n}</td><td>{pp}</td><td>{pr}</td>{html_cell(np_, cp)}{html_cell(nr, cr)}</tr>"
           for n, pp, np_, cp, pr, nr, cr in frows]
    hb += ["</table><p class='n'>Green / red: better / worse with the fixes than the preset. The fitted threshold "
           "(1 992.9) removes the false alarms and, with them, detection: the open defect is the covariance's error "
           "tail (TECH_DEBT N.15).</p>"]

    open(os.path.join(OUT, "tables_full.tex"), "w").write("\n".join(tex) + "\n")
    open(os.path.join(OUT, "tables_full.md"), "w").write(
        "# Publication tables, full (supplementary; generated by scripts/paper_tables.py; do not edit)\n\n"
        "Intervals: 95 % seed-bootstrap of the mean, 20 seeds (fault matrix 10). Sources: "
        "results/camp_final_2026-10-04, lora_cmp_2026-10-05, lora_cmp2_2026-10-05, fixes_n26_2026-10-05.\n\n"
        + "\n".join(md) + "\n")
    css = ("body{margin:0;background:#fff;color:#111;font:13px/1.35 'DejaVu Serif',Georgia,serif}"
           ".w{padding:24px 28px;display:inline-block}h2{font-size:15px;margin:18px 0 6px}"
           "table{border-collapse:collapse;border-top:2px solid #111;border-bottom:2px solid #111}"
           "th{border-bottom:1px solid #111;padding:5px 10px;font-weight:600;text-align:center}"
           "td{padding:3px 10px;text-align:center;white-space:nowrap}td:first-child{text-align:left}"
           f"tr.g td{{font-style:italic;border-top:1px solid #999;padding-top:6px}}tr.p td{{background:{HL}}}tr.p td:first-child{{font-weight:700}}"
           "p.n{font-size:11.5px;color:#333;max-width:980px;margin:5px 0 0}"
           ".k{display:inline-block;width:11px;height:11px;margin:0 4px -1px 8px;border:1px solid #999}")
    open(os.path.join(OUT, "preview_full.html"), "w").write(
        f"<!doctype html><html><head><meta charset='utf-8'><style>{css}</style></head><body><div class='w'>"
        + "".join(hb) + "</div></body></html>\n")


# ---------------------------------------------------------------- compact (main-text) tables
SHORT = {PROPOSED: "CA-TBL$^{+}$", "CA-TBL (preset)": "CA-TBL (base)",
         "Poucet-style fusion (LoRa + UWB)": "Poucet-style fusion", "Fixed-threshold switching": "Fixed threshold",
         "LoRa dual-band fusion (868 + 2.4 GHz)": "LoRa dual-band fusion", "LoRa 868 MHz RSSI + TDoA": "LoRa-868 RSSI+TDoA",
         "LoRa 868 MHz RSSI": "LoRa-868 RSSI", "LoRa 2.4 GHz ToF": "LoRa-2.4 ToF",
         "Single-hop confidence (LoRaWAPS-style)": "Single-hop (LoRaWAPS)"}
ORDER = [PROPOSED, "CA-TBL (preset)", "Poucet-style fusion (LoRa + UWB)", "Fixed-threshold switching",
         "LoRa dual-band fusion (868 + 2.4 GHz)", "LoRa 868 MHz RSSI + TDoA", "LoRa 868 MHz RSSI", "LoRa 2.4 GHz ToF",
         "Single-hop confidence (LoRaWAPS-style)"]
UP, DN = "$\\uparrow$", "$\\downarrow$"
CLAB = {"A": "5 km", "B": "Endgame", "C": "8 GW"}
METRICS = [("av", "Avail. (\\%)", True, 1, 100.0), ("c95", "CEP95 (m)", False, 1, 1.0), ("J", "E (J)", False, 1, 1.0)]


def h(t):
    return t.replace("$^{+}$", "⁺").replace("\\%", "%")


def compact():
    data = {}
    for k, _ in CFG:
        for name, r, c, arm in methods(k):
            data[(k, name)] = per_seed(R[r], c, arm)
    mean = lambda v, q: float(np.mean([x[q] for x in v.values()]))  # noqa: E731
    keys = [k for k, _ in CFG]
    # rank per (config, metric): best and second best
    rank = {}
    for k in keys:
        for q, _, hb_, _, _ in METRICS:
            vals = sorted({mean(data[(k, n)], q) for n in ORDER if (k, n) in data}, reverse=hb_)
            rank[(k, q)] = vals[:2]
    tex = ["% Generated by scripts/paper_tables.py (compact, main text). Preamble: \\usepackage{booktabs}",
           "% \\usepackage[table]{xcolor} \\definecolor{fav}{HTML}{D4EDDA} \\definecolor{unf}{HTML}{F8D7DA}",
           "% Full tables with 95 % intervals: tables_full.tex.", ""]
    md, hb = [], []
    grp = " & ".join(f"\\multicolumn{{3}}{{c}}{{{CLAB[k]}}}" for k, _ in CFG)
    sub = " & ".join(" & ".join(m[1] + (UP if m[2] else DN) for m in METRICS) for _ in CFG)
    tex += ["\\begin{table*}[t]", "\\centering", "\\small", "\\setlength{\\tabcolsep}{4.5pt}",
            "\\caption{Localisation performance, mean of 20 seeds. 5\\,km: three gateways; Endgame: finders converged; 8\\,GW: eight gateways on a ring. Best in \\textbf{bold}, second \\underline{underlined}.}",
            "\\label{tab:performance}", "\\begin{tabular}{l" + "ccc" * 3 + "}", "\\toprule",
            "Method & " + grp + " \\\\", "".join(f"\\cmidrule(lr){{{2 + 3 * i}-{4 + 3 * i}}}" for i in range(3)),
            " & " + sub + " \\\\", "\\midrule"]
    arrow = lambda m: "↑" if m[2] else "↓"  # noqa: E731
    md += ["**Table 1.** Localisation performance, mean of 20 seeds. 5 km: three gateways; Endgame: finders converged; 8 GW: eight gateways on a ring. Best in **bold**, second _underlined_.", "",
           "| Method | " + " | ".join(f"{CLAB[k]} {h(m[1])}{arrow(m)}" for k, _ in CFG for m in METRICS) + " |",
           "|---|" + "---|" * 9]
    hb += ["<table><caption><b>Table 1.</b> Localisation performance, mean of 20 seeds. 5 km: three gateways; Endgame: finders converged; 8 GW: eight gateways on a ring. Best in <b>bold</b>, second "
           "<u>underlined</u>.</caption><tr><th rowspan=2>Method</th>" +
           "".join(f"<th colspan=3 class='grp'>{CLAB[k]}</th>" for k, _ in CFG) + "</tr><tr>" +
           "".join(f"<th>{h(m[1])}{arrow(m)}</th>" for _ in CFG for m in METRICS) + "</tr>"]
    for i, n in enumerate(ORDER):
        if i == 2:
            tex.append("\\midrule")
        tc, mc, hc = [], [], []
        for k in keys:
            v = data.get((k, n))
            for q, _, _, p, sc in METRICS:
                if not v:
                    tc.append("--"); mc.append("–"); hc.append("<td>–</td>")
                    continue
                x = mean(v, q)
                s_ = f"{x * sc:.{p}f}"
                b1, b2 = x == rank[(k, q)][0], len(rank[(k, q)]) > 1 and x == rank[(k, q)][1]
                tc.append("\\textbf{" + s_ + "}" if b1 else ("\\underline{" + s_ + "}" if b2 else s_))
                mc.append(f"**{s_}**" if b1 else (f"_{s_}_" if b2 else s_))
                hc.append(f"<td>{'<b>' if b1 else ''}{'<u>' if b2 else ''}{s_}{'</u>' if b2 else ''}{'</b>' if b1 else ''}</td>")
        nm = SHORT[n] + ("$^{\\dagger}$" if n == PROPOSED else "")
        tex.append(nm + " & " + " & ".join(tc) + " \\\\")
        md.append(f"| {h(SHORT[n]) + ('†' if n == PROPOSED else '')} | " + " | ".join(mc) + " |")
        hb.append(f"<tr class='{'p' if n == PROPOSED else ''}{' sep' if i == 2 else ''}'><td>{h(SHORT[n])}"
                  f"{'†' if n == PROPOSED else ''}</td>" + "".join(hc) + "</tr>")
    fn = ("CA-TBL with the four N.26 fixes; not adopted into the preset by the pre-registered rule (energy at 5 km). "
          "All methods share one simulator, deployment and energy model.")
    tex += ["\\bottomrule", "\\end{tabular}", "\\\\[2pt]\\footnotesize $^{\\dagger}$" + fn.replace("5 km", "5\\,km"),
            "\\end{table*}", ""]
    md += ["", "† " + fn, ""]
    hb += [f"</table><p class='n'>† {fn}</p>"]

    # Table 2: paired differences, values only, coloured when the interval excludes 0
    tex += ["\\begin{table*}[t]", "\\centering", "\\small", "\\setlength{\\tabcolsep}{4.5pt}",
            "\\caption{CA-TBL$^{+}$ minus each method, paired over 20 seeds. Shaded where the 95\\,\\% CI excludes 0: "
            "\\colorbox{fav}{favours CA-TBL$^{+}$}, \\colorbox{unf}{favours the method}.}", "\\label{tab:paired}",
            "\\begin{tabular}{l" + "ccc" * 3 + "}", "\\toprule", "vs. & " + grp + " \\\\",
            "".join(f"\\cmidrule(lr){{{2 + 3 * i}-{4 + 3 * i}}}" for i in range(3)),
            " & " + " & ".join("$\\Delta$Avail. (pp) & $\\Delta$CEP95 (m) & $\\Delta$E (J)" for _ in CFG) + " \\\\", "\\midrule"]
    md += ["**Table 2.** CA-TBL⁺ minus each method, paired over 20 seeds; 🟩 favours CA-TBL⁺, 🟥 favours the method "
           "(95 % CI excludes 0).", "",
           "| vs. | " + " | ".join(f"{CLAB[k]} {x}" for k, _ in CFG for x in ("ΔAvail (pp)", "ΔCEP95 (m)", "ΔE (J)")) + " |",
           "|---|" + "---|" * 9]
    hb += ["<table><caption><b>Table 2.</b> CA-TBL⁺ minus each method, paired over 20 seeds. Shaded where the 95 % CI "
           f"excludes 0: <span class='k' style='background:{GREEN}'></span>favours CA-TBL⁺, "
           f"<span class='k' style='background:{RED}'></span>favours the method.</caption><tr><th rowspan=2>vs.</th>" +
           "".join(f"<th colspan=3 class='grp'>{CLAB[k]}</th>" for k, _ in CFG) + "</tr><tr>" +
           "".join("<th>ΔAvail. (pp)</th><th>ΔCEP95 (m)</th><th>ΔE (J)</th>" for _ in CFG) + "</tr>"]
    for n in ORDER[1:]:
        tc, mc, hc = [], [], []
        for k in keys:
            a, v = data.get((k, PROPOSED)), data.get((k, n))
            for q, _, hb_, p, sc in METRICS:
                if not v:
                    tc.append("--"); mc.append("–"); hc.append("<td>–</td>")
                    continue
                m, lo, hi = paired(a, v, q)
                cls = "" if not (lo > 0 or hi < 0) else ("fav" if (m > 0) == hb_ else "unf")
                s_ = f"{m * sc:+.{p}f}" if abs(m * sc) < 100 else f"{m * sc:+.0f}"
                tc.append(TEXCOL[cls] + s_)
                mc.append({"fav": "🟩", "unf": "🟥"}.get(cls, "") + s_)
                hc.append(html_cell(s_, cls))
        tex.append(SHORT[n] + " & " + " & ".join(tc) + " \\\\")
        md.append(f"| {h(SHORT[n])} | " + " | ".join(mc) + " |")
        hb.append(f"<tr><td>{h(SHORT[n])}</td>" + "".join(hc) + "</tr>")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table*}", ""]
    md.append("")
    hb.append("</table>")

    # Table 3: ablation, absolute means, coloured against the base when the paired CI excludes 0
    abl = [("CA-TBL (base)", None), ("+ dual-band LoRa fusion", "D"), ("+ cross-tier UWB polling", "U"),
           ("+ TDoA innovation gate", "T"), ("+ integrity calibration", "M"), ("All four (CA-TBL$^{+}$)", "ALL")]
    two = CFG[:2]
    tex += ["\\begin{table}[t]", "\\centering", "\\small", "\\setlength{\\tabcolsep}{4pt}",
            "\\caption{Ablation, mean of 20 seeds. Shaded where the paired 95\\,\\% CI against the base excludes 0: "
            "\\colorbox{fav}{better}, \\colorbox{unf}{worse}.}", "\\label{tab:ablation}",
            "\\begin{tabular}{l" + "ccc" * 2 + "}", "\\toprule",
            "Variant & " + " & ".join(f"\\multicolumn{{3}}{{c}}{{{CLAB[k]}}}" for k, _ in two) + " \\\\",
            "\\cmidrule(lr){2-4}\\cmidrule(lr){5-7}",
            " & " + " & ".join(" & ".join(m[1] + (UP if m[2] else DN) for m in METRICS) for _ in two)
            + " \\\\", "\\midrule"]
    md += ["**Table 3.** Ablation, mean of 20 seeds; 🟩 better / 🟥 worse than the base (paired 95 % CI excludes 0).", "",
           "| Variant | " + " | ".join(f"{CLAB[k]} {h(m[1])}{arrow(m)}" for k, _ in two for m in METRICS) + " |",
           "|---|" + "---|" * 6]
    hb += ["<table><caption><b>Table 3.</b> Ablation, mean of 20 seeds. Shaded where the paired 95 % CI against the base "
           f"excludes 0: <span class='k' style='background:{GREEN}'></span>better, "
           f"<span class='k' style='background:{RED}'></span>worse.</caption><tr><th rowspan=2>Variant</th>" +
           "".join(f"<th colspan=3 class='grp'>{CLAB[k]}</th>" for k, _ in two) + "</tr><tr>" +
           "".join(f"<th>{h(m[1])}{arrow(m)}</th>" for _ in two for m in METRICS) + "</tr>"]
    for i, (name, pre) in enumerate(abl):
        if i == len(abl) - 1:
            tex.append("\\midrule")
        tc, mc, hc = [], [], []
        for k, _ in two:
            base = data[(k, "CA-TBL (preset)")]
            v = base if pre is None else per_seed(R["ev"], pre + CELL[k][3], "ca_tbl_full")
            for q, _, hb_, p, sc in METRICS:
                s_ = f"{mean(v, q) * sc:.{p}f}"
                cls = ""
                if pre is not None:
                    m, lo, hi = paired(v, base, q)
                    cls = "" if not (lo > 0 or hi < 0) else ("fav" if (m > 0) == hb_ else "unf")
                tc.append(TEXCOL[cls] + s_)
                mc.append({"fav": "🟩", "unf": "🟥"}.get(cls, "") + s_)
                hc.append(html_cell(s_, cls))
        tex.append(name + " & " + " & ".join(tc) + " \\\\")
        md.append(f"| {h(name)} | " + " | ".join(mc) + " |")
        hb.append(f"<tr class='{'sep' if i == len(abl) - 1 else ''}'><td>{h(name)}</td>" + "".join(hc) + "</tr>")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table}", ""]
    md.append("")
    hb.append("</table>")

    # Table 4: integrity, compact; precision and recall coloured separately against the base
    pre, new, fpre, fnew = table4()
    short = {"rb_common": "Range bias, all anchors", "rb_single": "Range bias, one anchor",
             "txdrop": "Tx-power drop", "antenna": "Antenna loss"}
    tex += ["\\begin{table}[t]", "\\centering", "\\small",
            "\\caption{Integrity check (10 seeds for faults). False alarms: share of fault-free fixes flagged. "
            "P / R: precision / recall of the flag under each injected fault. Shaded against the base: "
            "\\colorbox{fav}{better}, \\colorbox{unf}{worse}.}", "\\label{tab:integrity}",
            "\\begin{tabular}{lcccc}", "\\toprule",
            " & \\multicolumn{2}{c}{CA-TBL (base)} & \\multicolumn{2}{c}{CA-TBL$^{+}$} \\\\",
            "\\cmidrule(lr){2-3}\\cmidrule(lr){4-5}", " & 5\\,km & Endgame & 5\\,km & Endgame \\\\", "\\midrule",
            f"False alarms (\\%)$\\downarrow$ & {pre['A'] * 100:.1f} & {pre['B'] * 100:.1f} & "
            f"\\cellcolor{{fav}}{new['A'] * 100:.1f} & \\cellcolor{{fav}}{new['B'] * 100:.1f} \\\\",
            "\\midrule", "Injected fault & P$\\uparrow$ & R$\\uparrow$ & P$\\uparrow$ & R$\\uparrow$ \\\\", "\\midrule"]
    md += ["**Table 4.** Integrity check (10 seeds for faults). False alarms: share of fault-free fixes flagged. "
           "P / R: precision / recall under each injected fault; 🟩 better / 🟥 worse than the base.", "",
           "| | Base 5 km | Base Endgame | CA-TBL⁺ 5 km | CA-TBL⁺ Endgame |", "|---|---|---|---|---|",
           f"| False alarms (%)↓ | {pre['A'] * 100:.1f} | {pre['B'] * 100:.1f} | 🟩{new['A'] * 100:.1f} | 🟩{new['B'] * 100:.1f} |",
           "| **Injected fault** | **P↑** | **R↑** | **P↑** | **R↑** |"]
    hb += ["<table><caption><b>Table 4.</b> Integrity check (10 seeds for faults). False alarms: share of fault-free "
           "fixes flagged. P / R: precision / recall of the flag under each injected fault. Shaded against the base: "
           f"<span class='k' style='background:{GREEN}'></span>better, <span class='k' style='background:{RED}'></span>"
           "worse.</caption><tr><th rowspan=2></th><th colspan=2 class='grp'>CA-TBL (base)</th><th colspan=2 class='grp'>"
           "CA-TBL⁺</th></tr><tr><th>5 km</th><th>Endgame</th><th>5 km</th><th>Endgame</th></tr>",
           f"<tr><td>False alarms (%)↓</td><td>{pre['A'] * 100:.1f}</td><td>{pre['B'] * 100:.1f}</td>"
           + html_cell(f"{new['A'] * 100:.1f}", "fav") + html_cell(f"{new['B'] * 100:.1f}", "fav") + "</tr>",
           "<tr class='sep'><td><i>Injected fault</i></td><td><i>P↑</i></td><td><i>R↑</i></td><td><i>P↑</i></td>"
           "<td><i>R↑</i></td></tr>"]
    for f, _ in FAULTS:
        (pp, pr), (np_, nr) = fpre[f], fnew[f]
        cp, cr = ("fav" if np_ > pp else "unf"), ("fav" if nr > pr else "unf")
        tex.append(f"{short[f]} & {pp:.2f} & {pr:.2f} & {TEXCOL[cp]}{np_:.2f} & {TEXCOL[cr]}{nr:.2f} \\\\")
        md.append(f"| {short[f]} | {pp:.2f} | {pr:.2f} | {md_cell(f'{np_:.2f}', cp)} | {md_cell(f'{nr:.2f}', cr)} |")
        hb.append(f"<tr><td>{short[f]}</td><td>{pp:.2f}</td><td>{pr:.2f}</td>" + html_cell(f"{np_:.2f}", cp)
                  + html_cell(f"{nr:.2f}", cr) + "</tr>")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table}", ""]
    md.append("")
    hb.append("</table>")
    open(os.path.join(OUT, "tables.tex"), "w").write("\n".join(tex) + "\n")
    open(os.path.join(OUT, "tables.md"), "w").write(
        "# Publication tables (main text; generated by scripts/paper_tables.py; do not edit)\n\n"
        "Full tables with 95 % intervals: tables_full.md.\n\n" + "\n".join(md) + "\n")
    css = ("body{margin:0;background:#fff;color:#111;font:13px/1.4 'DejaVu Serif',Georgia,serif}"
           ".w{padding:22px 26px;display:inline-block}table{border-collapse:collapse;border-top:1.6px solid #111;"
           "border-bottom:1.6px solid #111;margin:0 0 26px}caption{caption-side:top;text-align:left;padding:0 0 6px;"
           "font-size:12.5px;max-width:900px}th{padding:3px 9px;font-weight:600;border-bottom:0.8px solid #111;"
           "text-align:center}th.grp{border-bottom:0.6px solid #555}td{padding:2.5px 9px;text-align:center;"
           "white-space:nowrap}td:first-child{text-align:left}tr.sep td{border-top:0.8px solid #111}"
           "tr.p td:first-child{font-weight:700}p.n{font-size:11.5px;margin:-20px 0 26px;max-width:900px}"
           ".k{display:inline-block;width:10px;height:10px;margin:0 4px -1px 6px;border:1px solid #999}")
    open(os.path.join(OUT, "preview.html"), "w").write(
        f"<!doctype html><html><head><meta charset='utf-8'><style>{css}</style></head><body><div class='w'>"
        + "".join(hb) + "</div></body></html>\n")


def main():
    write(table1(), table2(), table3(), table4())
    compact()
    print(open(os.path.join(OUT, "tables.md")).read())
    return 0


if __name__ == "__main__":
    sys.exit(main())
