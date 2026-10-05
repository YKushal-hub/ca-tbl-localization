#!/usr/bin/env python3
"""Publication tables from the registered campaigns (CAMP-FINAL, LORA-CMP, LORA-CMP-2,
FIXES-N26). Writes report/paper_tables/tables.tex (LaTeX, booktabs), tables.md and
preview.html. Every number is recomputed from the tracked per-cell summaries with the
campaigns' own per-seed definitions (scripts/lora_cmp_analyse.py per_seed); nothing is retyped.

usage: python3 scripts/paper_tables.py

Intervals are 95 % seed-bootstrap intervals of the mean (10 000 resamples). Each interval
draws from its own generator seeded 20261005, so a value does not depend on which others were
computed first. A bootstrap's ends can therefore differ from a campaign report's by a unit in
the last digit; the means are identical.
"""
import csv
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


def ci(v):
    v = np.asarray([x for x in v if not np.isnan(x)], dtype=float)
    rng = np.random.default_rng(20261005)
    m = v[rng.integers(0, len(v), size=(10000, len(v)))].mean(axis=1)
    return float(v.mean()), float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975))


def methods(k):
    cf, lc, l2, suf, gs, s2 = CELL[k]
    rows = [("CA-TBL (preset)", "cf", cf, "ca_tbl_full"),
            ("CA-TBL + N.26 fixes$^\\dagger$", "ev", "ALL" + suf, "ca_tbl_full")]
    if k != "C":
        rows += [("Fixed-threshold switching", "cf", cf, "fixed_threshold"),
                 ("Poucet-style fusion (LoRa + UWB)", "cf", cf, "poucet_fusion")]
    rows += [("LoRa dual-band fusion (868 + 2.4 GHz)", "l2", l2, "lora_fusion"),
             ("LoRa 868 MHz RSSI + TDoA", "cf", cf, "static_868"),
             ("LoRa 868 MHz RSSI", "lc", lc, "static_868"),
             ("LoRa 2.4 GHz ToF", "l2" if k == "C" else "cf", s2, "static_2p4")]
    if gs:
        rows += [("Single-hop confidence (LoRaWAPS-style)", "cf", gs, "ca_tbl_full")]
    return rows


def plain(s):
    return s.replace("$^\\dagger$", "†")


def fmt_ci(m, lo, hi, p):
    return f"{m:.{p}f} [{lo:.{p}f}, {hi:.{p}f}]"


def paired(a, b, key):
    s = sorted(set(a) & set(b))
    return ci([a[i][key] - b[i][key] for i in s]), len(s)


def table1():
    """Absolute performance: rows per configuration."""
    out = []
    for k, title in CFG:
        block = []
        for name, r, c, arm in methods(k):
            v = per_seed(R[r], c, arm)
            assert len(v) == 20, (k, name, len(v))
            col = lambda q: [x[q] for x in v.values()]  # noqa: E731
            u = ""
            if arm == "ca_tbl_full" and r in ("cf", "ev") and not c.startswith("G_"):
                n = [uwb_fixes(d, int(d.rsplit("s", 1)[1])) for d in cs.seeds(R[r], c)]
                u = f"{sum(n)} ({sum(x > 0 for x in n)})"
            block.append((name, ci(col("av")), float(np.mean(col("c50"))), float(np.mean(col("c95"))),
                          float(np.mean(col("J"))), u))
        out.append((k, title, block))
    return out


def table2():
    """CA-TBL + fixes minus each comparator, paired by seed."""
    out = []
    for k, title in CFG:
        ms = methods(k)
        allv = per_seed(R["ev"], ms[1][2], "ca_tbl_full")
        block = []
        for name, r, c, arm in [ms[0]] + ms[2:]:
            v = per_seed(R[r], c, arm)
            (a, n), (e, _), (j, _) = paired(allv, v, "av"), paired(allv, v, "c95"), paired(allv, v, "J")
            block.append((name, a, e, j, n))
        out.append((k, title, block))
    return out


def table3():
    """Ablation: each fix alone, and all four, minus the preset."""
    out = []
    fixes = [("Dual LoRa base (both LoRa bands fused)", "D"), ("UWB cross-tier polling", "U"),
             ("TDoA innovation gate", "T"), ("Integrity: correct covariance + fitted threshold", "M"),
             ("All four", "ALL")]
    for k, title in CFG[:2]:
        suf = CELL[k][3]
        base = per_seed(R["cf"], CELL[k][0], "ca_tbl_full")
        block = []
        for name, pre in fixes:
            v = per_seed(R["ev"], pre + suf, "ca_tbl_full")
            block.append((name, paired(v, base, "av")[0], paired(v, base, "c95")[0], paired(v, base, "J")[0]))
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


FAULTS = [("rb_common", "Common-mode range bias, 30 m"), ("rb_single", "Single-anchor range bias, 30 m"),
          ("txdrop", "Transmit-power drop, 6 dB"), ("antenna", "Antenna loss, 10 dB")]


def sig(t):
    return t[1] > 0 or t[2] < 0


def write(t1, t2, t3, t4):
    os.makedirs(OUT, exist_ok=True)
    tex, md, hb = [], [], []
    tex += ["% Generated by scripts/paper_tables.py from the tracked campaign summaries. Needs \\usepackage{booktabs}.",
            "% Intervals: 95 % seed-bootstrap of the mean, 20 seeds (fault matrix: 10). Bold: paired interval excludes 0.", ""]

    # Table 1
    tex += ["\\begin{table*}[t]", "\\centering", "\\small",
            "\\caption{Localisation performance on the CA-TBL simulator, 20 seeds per cell, 1\\,800\\,s. "
            "Availability: share of cycles with a position fix, mean over five tracked nodes. CEP: tag~0's "
            "circular error probable. Energy: mean per tracked node over the run. UWB fixes: total over 20 seeds "
            "(seeds with any).}", "\\label{tab:performance}",
            "\\begin{tabular}{lccccc}", "\\toprule",
            "Method & Availability [95\\,\\% CI] & CEP50 (m) & CEP95 (m) & Energy (J) & UWB fixes \\\\", "\\midrule"]
    md += ["## Table 1 — Localisation performance (20 seeds, 1 800 s)", "",
           "| Method | Availability [95 % CI] | CEP50 (m) | CEP95 (m) | Energy (J) | UWB fixes |", "|---|---|---|---|---|---|"]
    hb += ["<h2>Table 1 — Localisation performance (20 seeds, 1 800 s)</h2><table><tr><th>Method</th><th>Availability [95 % CI]</th>"
           "<th>CEP50 (m)</th><th>CEP95 (m)</th><th>Energy (J)</th><th>UWB fixes</th></tr>"]
    for i, (k, title, block) in enumerate(t1):
        if i:
            tex.append("\\midrule")
        tex.append(f"\\multicolumn{{6}}{{l}}{{\\textit{{({k}) {title}}}}} \\\\")
        md.append(f"| *({k}) {title}* | | | | | |")
        hb.append(f"<tr class='g'><td colspan=6>({k}) {html.escape(title)}</td></tr>")
        for name, av, c50, c95, j, u in block:
            tex.append(f"{name} & {fmt_ci(*av, 3)} & {c50:.1f} & {c95:.1f} & {j:.2f} & {u or '--'} \\\\")
            md.append(f"| {plain(name)} | {fmt_ci(*av, 3)} | {c50:.1f} | {c95:.1f} | {j:.2f} | {u or '—'} |")
            hb.append(f"<tr><td>{html.escape(plain(name))}</td><td>{fmt_ci(*av, 3)}</td><td>{c50:.1f}</td><td>{c95:.1f}</td>"
                      f"<td>{j:.2f}</td><td>{u or '—'}</td></tr>")
    note1 = ("$^\\dagger$Not adopted into the preset by the pre-registered rule: its energy at 5\\,km exceeds the preset's. "
             "All comparators run in the same simulator, deployment, channel and energy model.")
    tex += ["\\bottomrule", "\\end{tabular}", "", f"\\vspace{{2pt}}\\footnotesize {note1}", "\\end{table*}", ""]
    md += ["", "† Not adopted into the preset by the pre-registered rule: its energy at 5 km exceeds the preset's. "
           "All comparators run in the same simulator, deployment, channel and energy model.", ""]
    hb += ["</table><p class='n'>† Not adopted into the preset by the pre-registered rule: its energy at 5 km exceeds the "
           "preset's. All comparators run in the same simulator, deployment, channel and energy model.</p>"]

    # Table 2
    tex += ["\\begin{table}[t]", "\\centering", "\\small",
            "\\caption{CA-TBL with the N.26 fixes minus each comparator, paired by seed (mean [95\\,\\% CI]). "
            "Positive availability and negative CEP95 or energy favour CA-TBL + fixes.}", "\\label{tab:paired}",
            "\\begin{tabular}{lccc}", "\\toprule",
            "Comparator & $\\Delta$ availability & $\\Delta$ CEP95 (m) & $\\Delta$ energy (J) \\\\", "\\midrule"]
    md += ["## Table 2 — CA-TBL + N.26 fixes minus each comparator, paired by seed", "",
           "| Comparator | Δ availability | Δ CEP95 (m) | Δ energy (J) |", "|---|---|---|---|"]
    hb += ["<h2>Table 2 — CA-TBL + N.26 fixes minus each comparator, paired by seed</h2><table><tr><th>Comparator</th>"
           "<th>Δ availability</th><th>Δ CEP95 (m)</th><th>Δ energy (J)</th></tr>"]
    for i, (k, title, block) in enumerate(t2):
        if i:
            tex.append("\\midrule")
        tex.append(f"\\multicolumn{{4}}{{l}}{{\\textit{{({k}) {title}}}}} \\\\")
        md.append(f"| *({k}) {title}* | | | |")
        hb.append(f"<tr class='g'><td colspan=4>({k}) {html.escape(title)}</td></tr>")
        for name, a, e, j, n in block:
            cells = [(a, 3), (e, 1), (j, 2)]
            tex.append(plain(name).replace("†", "") + " & " + " & ".join(
                ("\\textbf{" + fmt_ci(*t, p) + "}") if sig(t) else fmt_ci(*t, p) for t, p in cells) + " \\\\")
            md.append(f"| {plain(name)} | " + " | ".join(
                (f"**{fmt_ci(*t, p)}**" if sig(t) else fmt_ci(*t, p)) for t, p in cells) + " |")
            hb.append(f"<tr><td>{html.escape(plain(name))}</td>" + "".join(
                f"<td class='{'s' if sig(t) else ''}'>{fmt_ci(*t, p)}</td>" for t, p in cells) + "</tr>")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table}", ""]
    md += ["", "Bold: the paired interval excludes 0.", ""]
    hb += ["</table><p class='n'>Bold: the paired interval excludes 0.</p>"]

    # Table 3
    tex += ["\\begin{table}[t]", "\\centering", "\\small",
            "\\caption{Ablation: each N.26 fix alone, and all four, minus the CA-TBL preset, paired by seed "
            "(mean [95\\,\\% CI]).}", "\\label{tab:ablation}",
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
        for name, a, e, j in block:
            cells = [(a, 3), (e, 1), (j, 2)]
            tex.append(name + " & " + " & ".join(
                ("\\textbf{" + fmt_ci(*t, p) + "}") if sig(t) else fmt_ci(*t, p) for t, p in cells) + " \\\\")
            md.append(f"| {name} | " + " | ".join(
                (f"**{fmt_ci(*t, p)}**" if sig(t) else fmt_ci(*t, p)) for t, p in cells) + " |")
            hb.append(f"<tr><td>{html.escape(name)}</td>" + "".join(
                f"<td class='{'s' if sig(t) else ''}'>{fmt_ci(*t, p)}</td>" for t, p in cells) + "</tr>")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table}", ""]
    md += ["", "Bold: the paired interval excludes 0.", ""]
    hb += ["</table><p class='n'>Bold: the paired interval excludes 0.</p>"]

    # Table 4
    pre, new, fpre, fnew = t4
    tex += ["\\begin{table}[t]", "\\centering", "\\small",
            "\\caption{Integrity check. Fault-free: share of tag~0 fix cycles flagged. Injected faults (10 seeds, "
            "a 25\\,\\% window, flagged fixes downweighted): precision and recall of the flag against the window.}",
            "\\label{tab:integrity}", "\\begin{tabular}{lcc}", "\\toprule",
            " & Preset & With N.26 fixes \\\\", "\\midrule",
            f"Fault-free flagged share, 5\\,km & {pre['A']:.3f} & {new['A']:.3f} \\\\",
            f"Fault-free flagged share, endgame & {pre['B']:.3f} & {new['B']:.3f} \\\\", "\\midrule",
            "\\multicolumn{3}{l}{\\textit{Precision / recall under injected faults}} \\\\"]
    md += ["## Table 4 — Integrity check", "", "| | Preset | With N.26 fixes |", "|---|---|---|",
           f"| Fault-free flagged share, 5 km | {pre['A']:.3f} | {new['A']:.3f} |",
           f"| Fault-free flagged share, endgame | {pre['B']:.3f} | {new['B']:.3f} |",
           "| *Precision / recall under injected faults* | | |"]
    hb += ["<h2>Table 4 — Integrity check</h2><table><tr><th></th><th>Preset</th><th>With N.26 fixes</th></tr>",
           f"<tr><td>Fault-free flagged share, 5 km</td><td>{pre['A']:.3f}</td><td>{new['A']:.3f}</td></tr>",
           f"<tr><td>Fault-free flagged share, endgame</td><td>{pre['B']:.3f}</td><td>{new['B']:.3f}</td></tr>",
           "<tr class='g'><td colspan=3>Precision / recall under injected faults</td></tr>"]
    for f, name in FAULTS:
        a, b = fpre[f], fnew[f]
        tex.append(f"{name} & {a[0]:.3f} / {a[1]:.3f} & {b[0]:.3f} / {b[1]:.3f} \\\\")
        md.append(f"| {name} | {a[0]:.3f} / {a[1]:.3f} | {b[0]:.3f} / {b[1]:.3f} |")
        hb.append(f"<tr><td>{name}</td><td>{a[0]:.3f} / {a[1]:.3f}</td><td>{b[0]:.3f} / {b[1]:.3f}</td></tr>")
    tex += ["\\bottomrule", "\\end{tabular}", "\\end{table}", ""]
    md += ["", "The fitted threshold (1 992.9) removes the false alarms and, with them, detection: the open defect is "
           "the covariance's error tail (TECH_DEBT N.15).", ""]
    hb += ["</table><p class='n'>The fitted threshold (1 992.9) removes the false alarms and, with them, detection: "
           "the open defect is the covariance's error tail (TECH_DEBT N.15).</p>"]

    open(os.path.join(OUT, "tables.tex"), "w").write("\n".join(tex) + "\n")
    open(os.path.join(OUT, "tables.md"), "w").write(
        "# Publication tables (generated by scripts/paper_tables.py; do not edit)\n\n"
        "Intervals: 95 % seed-bootstrap of the mean, 20 seeds (fault matrix 10). Sources: "
        "results/camp_final_2026-10-04, lora_cmp_2026-10-05, lora_cmp2_2026-10-05, fixes_n26_2026-10-05.\n\n"
        + "\n".join(md) + "\n")
    css = ("body{margin:0;background:#fff;color:#111;font:13px/1.35 'DejaVu Serif',Georgia,serif}"
           ".w{padding:24px 28px;display:inline-block}h2{font-size:15px;margin:18px 0 6px}"
           "table{border-collapse:collapse;border-top:2px solid #111;border-bottom:2px solid #111}"
           "th{border-bottom:1px solid #111;padding:5px 10px;font-weight:600;text-align:center}"
           "td{padding:3px 10px;text-align:center;white-space:nowrap}td:first-child{text-align:left}"
           "tr.g td{font-style:italic;border-top:1px solid #999;padding-top:6px}td.s{font-weight:700}"
           "p.n{font-size:11.5px;color:#333;max-width:900px;margin:5px 0 0}")
    open(os.path.join(OUT, "preview.html"), "w").write(
        f"<!doctype html><html><head><meta charset='utf-8'><style>{css}</style></head><body><div class='w'>"
        + "".join(hb) + "</div></body></html>\n")


def main():
    write(table1(), table2(), table3(), table4())
    print(open(os.path.join(OUT, "tables.md")).read())
    return 0


if __name__ == "__main__":
    sys.exit(main())
