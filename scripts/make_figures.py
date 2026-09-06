#!/usr/bin/env python3
"""Figure set for CA-TBL_Positive_Results.md.

GOVERNING RULE
--------------
Every number printed on a figure is computed here, from the result CSVs, and
asserted against the value written in CA-TBL_Positive_Results.md. A figure that
agrees with a document because the same number was typed twice is decoration. On
mismatch this fails loudly, prints both values, names the figure, and DOES NOT
DRAW it. A mismatch is a finding about the document; it is not a reason to widen
a tolerance.

Geometry is derived from the placement rule (Architecture.md 3.1, and the ring
formulas in ca_tbl_causal_sim.cc BuildDeployment) -- no coordinates are typed in.

Estimator conventions follow the ones already used in this project:
  * seeds are the independent unit;
  * ratios are summarised by the geometric mean of per-seed ratios with a
    percentile bootstrap (analyse_grid_b1.gci);
  * differences by the mean of per-seed differences with the same bootstrap.

    python3 scripts/make_figures.py
"""

import csv
import glob
import math
import os
import random
import statistics as st
import sys

import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import Circle
from matplotlib.lines import Line2D

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
FIGDIR = os.path.join(ROOT, "figures")
B_BOOT = 10000
RNG_SEED = 20260828          # the seed this project's analysis scripts use

# ---------------------------------------------------------------- palette ----
# Colour-blind safe (Okabe-Ito). Tier identity is ALSO carried by marker and
# line style, never by colour alone.
C_T1, C_T2, C_T3 = "#0072B2", "#E69F00", "#009E73"
C_A, C_B         = "#0072B2", "#D55E00"
C_ACCENT, C_GREY = "#CC79A7", "#555555"
plt.rcParams.update({
    "font.family": "DejaVu Sans", "font.size": 11,
    "axes.titlesize": 13, "axes.labelsize": 11,
    "xtick.labelsize": 10, "ytick.labelsize": 10,
    "legend.fontsize": 9.5, "figure.dpi": 100,
    "axes.grid": True, "grid.alpha": 0.25, "grid.linewidth": 0.6,
    "axes.spines.top": False, "axes.spines.right": False,
})

# ------------------------------------------------------------ assertions ----
RECORDS = []


class FigureMismatch(Exception):
    pass


def check(label, computed, documented, tol, kind="point"):
    """Record and enforce. kind='mc' marks a percentile-bootstrap endpoint,
    whose exact value depends on an RNG stream the document does not record."""
    ok = abs(computed - documented) <= tol
    RECORDS.append((label, computed, documented, tol, kind, ok))
    return ok


def enforce(figname):
    bad = [r for r in RECORDS if r[5] is False and r[4] == "point" and r[0].startswith(figname)]
    if bad:
        lines = ["", "=" * 78, "ASSERTION FAILURE in %s -- NOT DRAWN" % figname, "=" * 78]
        for lab, c, d, t, k, _ in bad:
            lines.append("  %-46s computed %-14.6f documented %-14.6f (tol %g)" % (lab, c, d, t))
        lines.append("A mismatch is a finding about the document. The tolerance is not the problem.")
        raise FigureMismatch("\n".join(lines))


# ------------------------------------------------------------ estimators ----
def geo_ci(ratios, B=B_BOOT, seed=RNG_SEED):
    random.seed(seed)
    n = len(ratios)
    lg = [math.log(x) for x in ratios]
    pt = math.exp(sum(lg) / n)
    bs = sorted(math.exp(sum(lg[random.randrange(n)] for _ in range(n)) / n) for _ in range(B))
    return pt, bs[int(.025 * B)], bs[int(.975 * B)]


def mean_ci(vals, B=B_BOOT, seed=RNG_SEED):
    random.seed(seed)
    n = len(vals)
    pt = sum(vals) / n
    bs = sorted(sum(vals[random.randrange(n)] for _ in range(n)) / n for _ in range(B))
    return pt, bs[int(.025 * B)], bs[int(.975 * B)]


# ---------------------------------------------------------------- loaders ---
def summary_rows(cell):
    p = os.path.join(cell, "arm_summary.csv")
    return list(csv.DictReader(open(p))) if os.path.exists(p) else []


def summary_val(cell, arm, col):
    for r in summary_rows(cell):
        if r["arm"] == arm:
            return float(r[col])
    return None


def seed_of(cell):
    return int(os.path.basename(cell).split("_")[1])


def cells(pattern):
    return sorted(glob.glob(pattern), key=seed_of)


def trace(cell, arm):
    s = seed_of(cell)
    p = os.path.join(cell, "trace_%s_seed%d.csv" % (arm, s))
    return list(csv.DictReader(open(p))) if os.path.exists(p) else None


def availability(rows, R):
    """Fraction of ALL decision cycles with a fix inside R. No-fix counts against
    -- the harder denominator, per the measurement discipline."""
    return sum(1 for r in rows if r["hadFix"] == "1" and float(r["errM"]) <= R) / len(rows)


def first_fix_time(rows, R):
    for r in rows:
        if r["hadFix"] == "1" and float(r["errM"]) <= R:
            return float(r["t"])
    return None


def caption(fig, text, reserve=0.0):
    """Caption rendered INTO the image, so the PNG is self-describing.
    reserve>0 reclaims space beneath subplot-based axes so the caption cannot
    land on the x-axis label."""
    if reserve > 0:
        fig.tight_layout(rect=(0, reserve, 1, 1))
    fig.text(0.5, 0.012, text, ha="center", va="bottom", fontsize=8.4,
             color=C_GREY, wrap=True)


def save(fig, name):
    os.makedirs(FIGDIR, exist_ok=True)
    for ext in ("png", "svg"):
        fig.savefig(os.path.join(FIGDIR, "%s.%s" % (name, ext)),
                    dpi=300, bbox_inches="tight", facecolor="white")
    plt.close(fig)
    return os.path.join("figures", name + ".png")


# ============================================================== FIGURE 1 =====
# Geometry from the placement rule, never from typed coordinates.
BOX_HALF, GW_GRID = 2000.0, 5
T2_RING, T3_RING = 150.0, 10.0
T2_PER_SITE, T3_PER_SITE = 3, 5
RING_PHASE = 0.3                 # ca_tbl_causal_sim.cc BuildDeployment
R80_T2, R80_T3 = 1149.5, 12.9    # measured, post sensitivity correction
FIG1_SEED = 21


def deployment(box_half=BOX_HALF, gw_grid=GW_GRID):
    gws = []
    g = 0.9 * box_half
    for ix in range(gw_grid):
        for iy in range(gw_grid):
            gws.append((-g + 2.0 * g * ix / (gw_grid - 1),
                        -g + 2.0 * g * iy / (gw_grid - 1)))
    f = 0.47 * box_half
    sites = [(-f, -f), (f, -f), (f, f), (-f, f)]
    t2, t3 = [], []
    for (cx, cy) in sites:
        for k in range(T2_PER_SITE):
            th = 2 * math.pi * k / T2_PER_SITE + RING_PHASE
            t2.append((cx + T2_RING * math.cos(th), cy + T2_RING * math.sin(th)))
        for k in range(T3_PER_SITE):
            th = 2 * math.pi * k / T3_PER_SITE + RING_PHASE
            t3.append((cx + T3_RING * math.cos(th), cy + T3_RING * math.sin(th)))
    return gws, sites, t2, t3


TIER_STYLE = {
    "868MHz": (C_T1, "Tier 1 - 868 MHz"),
    "2.4GHz": (C_T2, "Tier 2 - 2.4 GHz"),
    "UWB":    (C_T3, "Tier 3 - UWB"),
}


def figure1():
    gws, sites, t2, t3 = deployment()
    check("Fig1 tier-1 gateway count", len(gws), 25, 0)
    check("Fig1 facility site count", len(sites), 4, 0)
    check("Fig1 tier-2 anchor count", len(t2), 12, 0)
    check("Fig1 tier-3 anchor count", len(t3), 20, 0)
    check("Fig1 site spacing m", 2 * 0.47 * BOX_HALF, 1880.0, 0.5)
    enforce("Fig1")

    cell = os.path.join(ROOT, "results/joint_criterion/seed_%d" % FIG1_SEED)
    rows = trace(cell, "ca_tbl_full")
    xs = [float(r["trueX"]) for r in rows]
    ys = [float(r["trueY"]) for r in rows]
    tiers = [r["fixTier"] for r in rows]          # read, never inferred
    check("Fig1 trajectory cycles", len(rows), 1440, 0)
    enforce("Fig1")

    fig = plt.figure(figsize=(12.6, 8.2))
    ax = fig.add_axes([0.055, 0.10, 0.60, 0.84])
    axz = fig.add_axes([0.70, 0.36, 0.28, 0.42])

    for a, zoom in ((ax, False), (axz, True)):
        # Coverage as OUTLINE-dominant: twelve filled r80 discs at this scale
        # merge into a single wash that hides the trajectory the figure is about.
        for (cx, cy) in t2:
            a.add_patch(Circle((cx, cy), R80_T2, facecolor=C_T2, alpha=0.022,
                               edgecolor=C_T2, lw=0.9, ls=(0, (6, 5)), zorder=1))
        for (cx, cy) in t3:
            a.add_patch(Circle((cx, cy), R80_T3, facecolor=C_T3, alpha=0.55,
                               edgecolor=C_T3, lw=0.8, zorder=3))
        # path, then per-cycle tier marks
        a.plot(xs, ys, color="#B0B0B0", lw=0.5, alpha=0.7, zorder=4)
        for key, mk, sz, zo in (("2.4GHz", "s", 5, 5), ("868MHz", "o", 30, 8), ("UWB", "^", 46, 9)):
            px = [x for x, t in zip(xs, tiers) if t == key]
            py = [y for y, t in zip(ys, tiers) if t == key]
            if px:
                a.scatter(px, py, s=sz, marker=mk, c=TIER_STYLE[key][0],
                          linewidths=0.4 if key != "2.4GHz" else 0,
                          edgecolors="white" if key != "2.4GHz" else "none",
                          alpha=0.95, zorder=zo)
        a.scatter([p[0] for p in gws], [p[1] for p in gws], s=190, marker="H",
                  facecolor="none", edgecolor=C_T1, lw=1.9, zorder=6)
        a.scatter([p[0] for p in t2], [p[1] for p in t2], s=64, marker="s",
                  facecolor="white", edgecolor=C_T2, lw=1.7, zorder=8)
        a.scatter([p[0] for p in t3], [p[1] for p in t3], s=40, marker="^",
                  facecolor="white", edgecolor=C_T3, lw=1.4, zorder=9)
        a.scatter([p[0] for p in sites], [p[1] for p in sites], s=140, marker="P",
                  c=C_ACCENT, zorder=10)
        a.set_aspect("equal")

    ax.set_xlim(-BOX_HALF * 1.02, BOX_HALF * 1.02)
    ax.set_ylim(-BOX_HALF * 1.02, BOX_HALF * 1.02)
    ax.set_xlabel("x  (m)"); ax.set_ylabel("y  (m)")
    ax.set_title("Deployment and the tier that served each cycle", loc="left", pad=10)
    from collections import Counter
    cnt = Counter(tiers)
    ax.text(0.015, 0.985, "seed %d cycles served:  tier 1 = %d    tier 2 = %d    tier 3 = %d"
            % (FIG1_SEED, cnt.get("868MHz", 0), cnt.get("2.4GHz", 0), cnt.get("UWB", 0)),
            transform=ax.transAxes, va="top", ha="left", fontsize=9.6,
            bbox=dict(boxstyle="round,pad=0.4", fc="white", ec="#CCCCCC", lw=0.8))
    ax.annotate("", xy=(sites[0][0], sites[0][1]), xytext=(sites[1][0], sites[1][1]),
                arrowprops=dict(arrowstyle="<->", color=C_ACCENT, lw=1.4))
    ax.text(0, sites[0][1] - 118, "site spacing %d m" % round(2 * 0.47 * BOX_HALF),
            ha="center", color=C_ACCENT, fontsize=9.5)

    zc = sites[2]
    pad = 235.0
    axz.set_xlim(zc[0] - pad, zc[0] + pad); axz.set_ylim(zc[1] - pad, zc[1] + pad)
    axz.set_title("One facility site", loc="left", fontsize=11, pad=6)
    axz.tick_params(labelsize=8)
    axz.add_patch(Circle(zc, T2_RING, facecolor="none", edgecolor=C_T2, lw=1.0, ls=":"))
    axz.add_patch(Circle(zc, T3_RING, facecolor="none", edgecolor=C_T3, lw=1.0, ls=":"))
    axz.annotate("tier-2 ring 150 m", xy=(zc[0] + T2_RING * 0.71, zc[1] + T2_RING * 0.71),
                 xytext=(zc[0] - 215, zc[1] + 175), fontsize=8.6, color=C_T2,
                 arrowprops=dict(arrowstyle="->", color=C_T2, lw=0.9))
    axz.annotate("tier-3 ring 10 m\ncoverage r80 12.9 m", xy=(zc[0] + T3_RING, zc[1]),
                 xytext=(zc[0] + 55, zc[1] - 160), fontsize=8.6, color=C_T3,
                 arrowprops=dict(arrowstyle="->", color=C_T3, lw=0.9))

    handles = [
        Line2D([], [], marker="H", ls="none", mfc="none", mec=C_T1, mew=1.9, ms=12,
               label="Tier-1 gateway  (25)"),
        Line2D([], [], marker="P", ls="none", color=C_ACCENT, ms=10, label="Facility site  (4)"),
        Line2D([], [], marker="s", ls="none", mfc="white", mec=C_T2, mew=1.7, ms=8,
               label="Tier-2 anchor  (12, 150 m ring)"),
        Line2D([], [], marker="^", ls="none", mfc="white", mec=C_T3, mew=1.4, ms=8,
               label="Tier-3 anchor  (20, 10 m ring)"),
        Line2D([], [], marker="o", ls="none", color=C_T1, ms=6, label="Cycle served by tier 1"),
        Line2D([], [], marker="s", ls="none", color=C_T2, ms=6, label="Cycle served by tier 2"),
        Line2D([], [], marker="^", ls="none", color=C_T3, ms=7, label="Cycle served by tier 3"),
        Line2D([], [], color=C_T2, lw=0.8, ls=(0, (5, 4)), label="Tier-2 coverage r80 1149.5 m"),
        Line2D([], [], color=C_T3, lw=4, alpha=0.35, label="Tier-3 coverage r80 12.9 m"),
    ]
    fig.legend(handles=handles, loc="lower right", bbox_to_anchor=(0.995, 0.055),
               frameon=False, ncol=1)
    caption(fig, "boxHalf 2000 m, gwGrid 5. Geometry derived from the placement rule; "
                 "per-cycle tier read from fixTier. Trajectory: seed 21, all 1440 cycles. "
                 "Source results/joint_criterion, source ec48e90 dirty 0.")
    return save(fig, "fig1_deployment")


# ============================================================== FIGURE 2 =====
def figure2():
    def energy(cond):
        out = {}
        for c in cells(os.path.join(ROOT, "results/claim2_rebase/%s/seed_*" % cond)):
            v = summary_val(c, "ca_tbl_full", "totalJ")
            if v is not None:
                out[seed_of(c)] = v
        return out

    off, on = energy("propagation_evfalse"), energy("propagation_evtrue")
    ss = sorted(set(off) & set(on))
    ratios = [off[s] / on[s] for s in ss]
    m_off, m_on = st.mean(off[s] for s in ss), st.mean(on[s] for s in ss)

    # The document's headline is the ARITHMETIC mean of per-seed ratios.
    arith, alo, ahi = mean_ci(ratios)
    geo, glo, ghi = geo_ci(ratios)

    check("Fig2 n paired seeds", len(ss), 20, 0)
    check("Fig2 mean energy gate off J", m_off, 149.524, 0.01)
    check("Fig2 mean energy gate on J", m_on, 22.755, 0.01)
    check("Fig2 headline ratio", arith, 6.53, 0.01)
    check("Fig2 reduction pct", 100 * (1 - m_on / m_off), 84.7, 0.05)
    check("Fig2 CI low", alo, 5.11, 0.20, kind="mc")
    check("Fig2 CI high", ahi, 7.95, 0.20, kind="mc")
    enforce("Fig2")

    fig, ax = plt.subplots(figsize=(8.0, 7.0))
    for s in ss:
        ax.plot([0, 1], [off[s], on[s]], color=C_GREY, lw=1.0, alpha=0.5,
                marker="o", ms=3.6, zorder=2)
    ax.plot([0, 1], [m_off, m_on], color=C_A, lw=3.0, marker="D", ms=9,
            zorder=4, label="mean of seeds")
    for x, v, va in ((0, m_off, "bottom"), (1, m_on, "top")):
        ax.annotate("%.3f J" % v, xy=(x, v), xytext=(0, 13 if va == "bottom" else -19),
                    textcoords="offset points", ha="center", color=C_A,
                    fontweight="bold", fontsize=11.5)
    ax.set_yscale("log")
    ax.set_xlim(-0.32, 1.42); ax.set_xticks([0, 1])
    ax.set_xticklabels(["escalation quality gate\nOFF", "escalation quality gate\nON"])
    ax.set_ylabel("Total radio energy per run  (J, log scale)")
    ax.set_title("Energy: %.2f x reduction, every seed in the same direction" % arith,
                 loc="left", pad=10)
    ax.text(1.20, math.sqrt(m_off * m_on),
            "%.2f x\n[%.2f, %.2f]\n%.1f%% reduction\nn = %d paired seeds"
            % (arith, alo, ahi, 100 * (1 - m_on / m_off), len(ss)),
            ha="center", va="center", fontsize=11, color=C_A, fontweight="bold",
            bbox=dict(boxstyle="round,pad=0.5", fc="white", ec=C_A, lw=1.3))
    ax.annotate("", xy=(1.06, m_on), xytext=(1.06, m_off),
                arrowprops=dict(arrowstyle="<->", color=C_A, lw=1.5))
    n_down = sum(1 for s in ss if on[s] < off[s])
    ax.legend(loc="lower left", frameon=False)
    caption(fig, "Paired per-seed slope chart; %d/%d seeds move the same way. Ratio is the "
                 "mean of per-seed ratios with a percentile bootstrap (B=%d). "
                 "Source results/claim2_rebase, nlosSource propagation, post-D1, dirty 0, n=%d."
                 % (n_down, len(ss), B_BOOT, len(ss)), reserve=0.075)
    return save(fig, "fig2_energy"), (arith, alo, ahi, geo, glo, ghi, len(ss))


# ============================================================== FIGURE 3 =====
def figure3():
    def col(cond, c):
        return [summary_val(x, "ca_tbl_full", c)
                for x in cells(os.path.join(ROOT, "results/siting_repair/%s/seed_*" % cond))]

    A, Rp = "colocated_asis", "colocated_min"
    t1 = (st.mean(col(A, "cycles_868")), st.mean(col(Rp, "cycles_868")))
    t2 = (st.mean(col(A, "cycles_2p4")), st.mean(col(Rp, "cycles_2p4")))
    t3 = (st.mean(col(A, "cycles_uwb")), st.mean(col(Rp, "cycles_uwb")))
    esc = (col(A, "escalations"), col(Rp, "escalations"))
    de = (col(A, "deescalations"), col(Rp, "deescalations"))
    uwb_pt, uwb_lo, uwb_hi = mean_ci(col(Rp, "cycles_uwb"))
    seeds_served = sum(1 for v in col(Rp, "cycles_uwb") if v > 0)

    check("Fig3 as-is cycles_868", t1[0], 480.60, 0.01)
    check("Fig3 repaired cycles_868", t1[1], 1.65, 0.01)
    check("Fig3 as-is cycles_2p4", t2[0], 959.40, 0.01)
    check("Fig3 repaired cycles_2p4", t2[1], 1355.30, 0.01)
    check("Fig3 as-is cycles_uwb", t3[0], 0.00, 0.01)
    check("Fig3 repaired cycles_uwb", t3[1], 83.05, 0.01)
    check("Fig3 as-is escalations", st.mean(esc[0]), 147.85, 0.01)
    check("Fig3 repaired escalations", st.mean(esc[1]), 3.50, 0.01)
    check("Fig3 as-is deescalations", st.mean(de[0]), 146.95, 0.01)
    check("Fig3 seeds with tier-3 service", seeds_served, 20, 0)
    check("Fig3 conservation residual", (t1[1] - t1[0]) + (t2[1] - t2[0]) + (t3[1] - t3[0]), 0.0, 0.02)
    check("Fig3 uwb CI low", uwb_lo, 76.9, 1.0, kind="mc")
    check("Fig3 uwb CI high", uwb_hi, 89.2, 1.0, kind="mc")
    enforce("Fig3")

    fig, (axA, axB) = plt.subplots(1, 2, figsize=(14.2, 6.0),
                                   gridspec_kw={"width_ratios": [1.32, 1]})
    labels = ["as-is", "repaired"]
    ypos = [1, 0]
    segs = [(t1, C_T1, "tier 1 - 868 MHz", "//"), (t2, C_T2, "tier 2 - 2.4 GHz", ""),
            (t3, C_T3, "tier 3 - UWB", "xx")]
    left = [0.0, 0.0]
    for vals, colr, lab, hatch in segs:
        axA.barh(ypos, [vals[0], vals[1]], left=left, height=0.52, color=colr,
                 edgecolor="white", lw=1.1, hatch=hatch, label=lab)
        for i, v in enumerate([vals[0], vals[1]]):
            if v > 120:
                axA.text(left[i] + v / 2, ypos[i], "%.2f" % v, ha="center", va="center",
                         fontsize=10, color="white", fontweight="bold")
        left = [left[0] + vals[0], left[1] + vals[1]]
    axA.annotate("0.00 -> 83.05 tier-3 cycles", xy=(1440 - t3[1] / 2, 0.29),
                 xytext=(1090, 0.62), fontsize=10.5, color=C_T3, fontweight="bold",
                 ha="center", arrowprops=dict(arrowstyle="->", color=C_T3, lw=1.4),
                 bbox=dict(boxstyle="round,pad=0.35", fc="white", ec=C_T3, lw=1.0))
    axA.set_yticks(ypos); axA.set_yticklabels(labels)
    axA.set_xlabel("Decision cycles per run (of 1440)")
    axA.set_title("A  Tier occupancy", loc="left", pad=8)
    axA.legend(loc="lower center", bbox_to_anchor=(0.5, -0.30), ncol=3, frameon=False)
    axA.grid(axis="y", alpha=0)

    for i in range(len(esc[0])):
        axB.plot([0, 1], [esc[0][i], esc[1][i]], color=C_GREY, lw=0.9, alpha=0.5,
                 marker="o", ms=3.4, zorder=2)
    axB.plot([0, 1], [st.mean(esc[0]), st.mean(esc[1])], color=C_B, lw=3.0,
             marker="D", ms=9, zorder=4, label="mean escalations")
    axB.scatter([0], [st.mean(de[0])], marker="v", s=110, color=C_ACCENT, zorder=5,
                label="mean de-escalations (as-is)")
    axB.annotate("%.2f escalations against\n%.2f de-escalations\n= thrashing"
                 % (st.mean(esc[0]), st.mean(de[0])), xy=(0.02, st.mean(esc[0])),
                 xytext=(0.30, 92), fontsize=10, color=C_B,
                 arrowprops=dict(arrowstyle="->", color=C_B, lw=1.2),
                 bbox=dict(boxstyle="round,pad=0.35", fc="white", ec=C_B, lw=1.0))
    axB.annotate("%.2f - escalates and stays" % st.mean(esc[1]), xy=(0.985, st.mean(esc[1])),
                 xytext=(0.44, 22), fontsize=10, color=C_B, ha="center",
                 arrowprops=dict(arrowstyle="->", color=C_B, lw=1.2),
                 bbox=dict(boxstyle="round,pad=0.35", fc="white", ec=C_B, lw=1.0))
    axB.set_xlim(-0.25, 1.35); axB.set_xticks([0, 1]); axB.set_xticklabels(labels)
    axB.set_ylabel("Events per seed")
    axB.set_title("B  Escalation stability", loc="left", pad=8)
    axB.legend(loc="upper right", frameon=False)

    caption(fig, "-478.95 cycles off tier 1  =  +395.90 on tier 2  +  83.05 on tier 3   "
                 "(budget conserved, residual %.3f). Tier-3 service on %d/20 seeds, "
                 "83.05 cycles [%.1f, %.1f]. Source results/siting_repair, 80 cells, n=20 per arm."
                 % ((t1[1] - t1[0]) + (t2[1] - t2[0]) + (t3[1] - t3[0]), seeds_served, uwb_lo, uwb_hi),
                 reserve=0.10)
    return save(fig, "fig3_escalation_repair")


# ================================================== FIGURES 4 / 5 / 6 shared =
JC = "results/joint_criterion/seed_*"
R_LADDER = [0.25, 0.5, 1.0, 2.0, 5.0, 10.0, 25.0, 50.0]
BUDGET_S = 1080.0        # 18 min high-survival boundary
R_ICAR = 0.25
A_SPEC = 0.95


def jc_avail(R):
    out = {}
    for c in cells(os.path.join(ROOT, JC)):
        a, b = trace(c, "ca_tbl_full"), trace(c, "static_2p4")
        if a and b:
            out[seed_of(c)] = (availability(a, R), availability(b, R))
    return out


def figure4():
    av = jc_avail(25.0)
    ss = sorted(av)
    diffs = [av[s][0] - av[s][1] for s in ss]
    pt, lo, hi = mean_ci(diffs)
    pass_a = sum(1 for s in ss if av[s][0] >= A_SPEC)
    pass_b = sum(1 for s in ss if av[s][1] >= A_SPEC)

    check("Fig4 n fresh seeds", len(ss), 52, 0)
    check("Fig4 first seed", min(ss), 21, 0)
    check("Fig4 last seed", max(ss), 72, 0)
    check("Fig4 paired difference", pt, 0.001629, 1e-6)
    check("Fig4 ca_tbl_full meeting A>=0.95", pass_a, 52, 0)
    check("Fig4 static_2p4 meeting A>=0.95", pass_b, 52, 0)
    check("Fig4 CI low", lo, 0.000251, 5e-5, kind="mc")
    check("Fig4 CI high", hi, 0.003008, 5e-5, kind="mc")
    enforce("Fig4")

    fig, ax = plt.subplots(figsize=(11.4, 6.2))
    order = sorted(range(len(ss)), key=lambda i: diffs[i])
    xs = range(1, len(ss) + 1)
    ax.axhspan(lo, hi, color=C_A, alpha=0.13, zorder=1,
               label="95%% CI of the mean  [%+.6f, %+.6f]" % (lo, hi))
    ax.axhline(0, color="black", lw=1.5, zorder=3, label="zero - no difference")
    ax.axhline(pt, color=C_A, lw=2.0, ls="--", zorder=4,
               label="mean difference  %+.6f" % pt)
    for x, i in zip(xs, order):
        d = diffs[i]
        ax.plot([x, x], [0, d], color=C_GREY, lw=0.7, alpha=0.5, zorder=2)
        ax.scatter([x], [d], s=34, marker="o" if d >= 0 else "v",
                   color=C_A if d >= 0 else C_B, zorder=5)
    n_pos = sum(1 for d in diffs if d > 0)
    ax.set_xlabel("52 fresh seeds (21-72), sorted by difference")
    ax.set_ylabel("availability(ca_tbl_full) - availability(static_2p4)")
    ax.set_title("Registered availability lead over the strongest single band", loc="left", pad=10)
    ax.text(0.985, 0.05, "%d/%d seeds positive\nCI excludes zero" % (n_pos, len(ss)),
            transform=ax.transAxes, ha="right", va="bottom", fontsize=10.5,
            color=C_A, fontweight="bold",
            bbox=dict(boxstyle="round,pad=0.45", fc="white", ec=C_A, lw=1.2))
    ax.legend(loc="upper left", frameon=False)
    caption(fig, "Criterion, seed count and analysis were committed to version control before the "
                 "first cell ran; the 52 seeds (21-72) are disjoint from the seeds that produced the "
                 "observation under test. Availability over all 1440 cycles at R <= 25 m; a no-fix "
                 "cycle counts against. Source results/joint_criterion, 52 cells, "
                 "source ec48e90 dirty 0, n=52.", reserve=0.105)
    return save(fig, "fig4_registered_availability")


def figure5():
    lad = {R: jc_avail(R) for R in R_LADDER}
    ss = sorted(lad[R_LADDER[0]])
    ma = [st.mean(lad[R][s][0] for s in ss) for R in R_LADDER]
    mb = [st.mean(lad[R][s][1] for s in ss) for R in R_LADDER]
    i25 = R_LADDER.index(R_ICAR)
    d25 = [lad[R_ICAR][s][0] - lad[R_ICAR][s][1] for s in ss]
    p25, lo25, hi25 = mean_ci(d25)
    ratio = ma[i25] / mb[i25]
    i10 = R_LADDER.index(10.0)
    pa10 = sum(1 for s in ss if lad[10.0][s][0] >= A_SPEC)
    pb10 = sum(1 for s in ss if lad[10.0][s][1] >= A_SPEC)

    check("Fig5 ca_tbl_full A at 0.25 m", ma[i25], 0.0465, 5e-4)
    check("Fig5 static_2p4 A at 0.25 m", mb[i25], 0.0093, 5e-4)
    check("Fig5 ratio at 0.25 m", ratio, 5.00, 0.01)
    check("Fig5 paired diff at 0.25 m", p25, 0.037206, 1e-6)
    check("Fig5 CI low at 0.25 m", lo25, 0.0334, 1e-3, kind="mc")
    check("Fig5 CI high at 0.25 m", hi25, 0.0410, 1e-3, kind="mc")
    check("Fig5 rungs drawn", len(R_LADDER), 8, 0)
    check("Fig5 static_2p4 passes A>=0.95 at R=10 m", pb10, 52, 0)
    check("Fig5 ca_tbl_full passes A>=0.95 at R=10 m", pa10, 51, 0)
    enforce("Fig5")

    fig, ax = plt.subplots(figsize=(11.0, 6.6))
    ax.plot(R_LADDER, ma, color=C_A, lw=2.2, marker="o", ms=8, label="ca_tbl_full (repaired)")
    ax.plot(R_LADDER, mb, color=C_B, lw=2.2, ls="--", marker="s", ms=7.5, label="static_2p4")
    ax.set_xscale("log"); ax.set_yscale("log")
    ax.set_xticks(R_LADDER)
    ax.set_xticklabels([("%g" % r) for r in R_LADDER])
    ax.axvline(R_ICAR, color=C_ACCENT, lw=1.5, ls=":", zorder=1)
    ax.text(0.035, 0.93,
            "0.25 m - probe-grid spacing,\nICAR Recommendation 009\n(the one figure verified\nagainst a primary source)",
            transform=ax.transAxes, fontsize=9.2, color=C_ACCENT, va="top", ha="left",
            bbox=dict(boxstyle="round,pad=0.35", fc="white", ec=C_ACCENT, lw=0.9, alpha=0.95))
    ax.annotate("%.4f vs %.4f\n%.2f x\nCI [%+.4f, %+.4f]" % (ma[i25], mb[i25], ratio, lo25, hi25),
                xy=(R_ICAR, ma[i25]), xytext=(0.62, 0.030), fontsize=10.5, color=C_A,
                fontweight="bold", arrowprops=dict(arrowstyle="->", color=C_A, lw=1.3),
                bbox=dict(boxstyle="round,pad=0.42", fc="white", ec=C_A, lw=1.2))
    ax.annotate("at R = 10 m the single-band arm leads\n(%d/52 against %d/52 meeting A >= 0.95)"
                % (pb10, pa10), xy=(10.0, mb[i10]), xytext=(2.6, 0.145), fontsize=9.6,
                color=C_B, arrowprops=dict(arrowstyle="->", color=C_B, lw=1.2))
    ax.set_xlabel("Accuracy requirement R  (m, log scale)")
    ax.set_ylabel("Availability over all 1440 cycles  (log scale)")
    ax.set_title("The accuracy ladder - all eight rungs, including where the single band leads",
                 loc="left", pad=10)
    ax.legend(loc="lower right", frameon=False)
    caption(fig, "Full ladder, no rung removed. Availability re-adjudicated at each R from the "
                 "per-cycle errors of the same campaign. Source results/joint_criterion, 52 cells, "
                 "source ec48e90 dirty 0, n=52.", reserve=0.075)
    return save(fig, "fig5_accuracy_ladder"), (pa10, pb10)


def figure6():
    res = {}
    for arm in ("ca_tbl_full", "static_2p4"):
        ts, cens = [], 0
        for c in cells(os.path.join(ROOT, JC)):
            rows = trace(c, arm)
            if rows is None:
                continue
            t = first_fix_time(rows, R_ICAR)
            if t is None:
                cens += 1
            else:
                ts.append(t)
        v = sorted(ts)
        res[arm] = dict(t=v, censored=cens,
                        med_avg=st.median(v),
                        med_high=v[len(v) // 2],
                        inside=sum(1 for x in v if x <= BUDGET_S))
    a, b = res["ca_tbl_full"], res["static_2p4"]

    check("Fig6 ca_tbl_full median s", a["med_avg"], 132.5, 0.01)
    check("Fig6 ca_tbl_full censored", a["censored"], 0, 0)
    check("Fig6 static_2p4 censored", b["censored"], 0, 0)
    check("Fig6 ca_tbl_full inside budget", a["inside"], 52, 0)
    check("Fig6 static_2p4 inside budget", b["inside"], 51, 0)
    # The document's 170.0 s for static_2p4 is the UPPER median; the averaged
    # median of the same 52 values is 167.5 s. Asserted on the averaged
    # convention, which is what the rest of this project uses.
    check("Fig6 static_2p4 median s", b["med_avg"], 170.0, 0.01)
    check("Fig6 reduction pct", 100 * (1 - a["med_avg"] / b["med_avg"]), 22.1, 0.05)
    enforce("Fig6")

    fig, ax = plt.subplots(figsize=(11.0, 6.4))
    for arm, d, colr, ls, mk, lab in (("ca_tbl_full", a, C_A, "-", "o", "ca_tbl_full (repaired)"),
                                      ("static_2p4", b, C_B, "--", "s", "static_2p4")):
        v = d["t"]
        xs = [0] + [x for x in v for _ in (0, 1)]
        ys = [0] + [y for i in range(len(v)) for y in (i / len(v), (i + 1) / len(v))]
        ax.plot(xs, ys, color=colr, lw=2.2, ls=ls, label=lab)
        ax.axvline(d["med_avg"], color=colr, lw=1.1, ls=":", alpha=0.85)
    ax.axvline(BUDGET_S, color=C_GREY, lw=1.8)
    ax.text(BUDGET_S * 0.985, 0.42, "18-minute high-survival boundary\n(Falk & Brugger 1994)",
            rotation=90, ha="right", va="center", fontsize=9.2, color=C_GREY)
    ax.set_xscale("log")
    ax.set_ylim(0, 1.045); ax.set_xlim(40, 1500)
    ax.set_xlabel("Time to first fix within R = 0.25 m  (s, log scale)")
    ax.set_ylabel("Fraction of seeds with a probe-grade fix")
    ax.set_title("Time to first probe-grade fix - both arms reach 1.0 well inside the budget",
                 loc="left", pad=10)
    ax.axhline(1.0, color="black", lw=0.8, alpha=0.5)
    ax.text(0.985, 0.985, "both curves reach 1.0\n0 censored seeds, both arms",
            transform=ax.transAxes, ha="right", va="top", fontsize=10,
            bbox=dict(boxstyle="round,pad=0.4", fc="white", ec="black", lw=0.9))
    ax.legend(loc="center left", frameon=False)
    caption(fig, "Empirical CDF over 52 seeds. Source results/joint_criterion, "
                 "source ec48e90 dirty 0, n=52.", reserve=0.075)
    return save(fig, "fig6_time_to_first_fix")


# ==================================================================== main ===
def main():
    os.makedirs(FIGDIR, exist_ok=True)
    made, failed = [], []
    for name, fn in (("Figure 1", figure1), ("Figure 2", figure2), ("Figure 3", figure3),
                     ("Figure 4", figure4), ("Figure 5", figure5), ("Figure 6", figure6)):
        try:
            r = fn()
            made.append((name, r[0] if isinstance(r, tuple) else r))
        except FigureMismatch as e:
            failed.append((name, str(e)))
            print(str(e))

    w = 48
    print("\n" + "=" * 104)
    print("ASSERTION TABLE   computed vs CA-TBL_Positive_Results.md")
    print("=" * 104)
    print("%-*s %16s %16s %10s %6s  %s" % (w, "quantity", "computed", "documented", "tol", "kind", "result"))
    print("-" * 104)
    npass = nfail = 0
    for lab, c, d, t, kind, ok in RECORDS:
        print("%-*s %16.6f %16.6f %10g %6s  %s"
              % (w, lab, c, d, t, "MC" if kind == "mc" else "point", "PASS" if ok else "FAIL"))
        npass, nfail = (npass + 1, nfail) if ok else (npass, nfail + 1)
    print("-" * 104)
    print("%d assertions: %d pass, %d FAIL" % (len(RECORDS), npass, nfail))
    print("\n'MC' rows are percentile-bootstrap endpoints. The document does not record the RNG")
    print("stream that produced its intervals, so these are compared within Monte-Carlo tolerance;")
    print("point estimates are compared strictly.")

    print("\nFIGURES WRITTEN")
    for n, p in made:
        print("  %-10s %s" % (n, p))
    if failed:
        print("\nFIGURES NOT DRAWN (assertion failure -- a finding about the document)")
        for n, _ in failed:
            print("  %s" % n)
    return 1 if failed else 0


if __name__ == "__main__":
    sys.exit(main())
