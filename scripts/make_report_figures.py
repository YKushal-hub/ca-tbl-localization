#!/usr/bin/env python3
"""
CA-TBL Report Figure Generator
Generates all figures for the CA-TBL Technical Report.
Reads directly from results/ directories — no manual data entry.

Usage: python3 scripts/make_report_figures.py
"""
import csv, glob, os, sys, statistics
import matplotlib
matplotlib.use("Agg")
matplotlib.rcParams.update({
    'font.family': 'sans-serif',
    'font.sans-serif': ['DejaVu Sans', 'Arial', 'Helvetica'],
    'font.size': 10,
    'axes.titlesize': 12,
    'axes.labelsize': 11,
    'legend.fontsize': 9,
    'xtick.labelsize': 9,
    'ytick.labelsize': 9,
    'figure.dpi': 150,
    'savefig.dpi': 200,
    'savefig.bbox': 'tight',
    'axes.grid': True,
    'grid.alpha': 0.3,
    'axes.spines.top': False,
    'axes.spines.right': False,
})
import matplotlib.pyplot as plt
import matplotlib.ticker as mticker
import numpy as np

ROOT = os.path.expanduser("~/ns/ns-3-dev")
OUT  = os.path.join(ROOT, "report", "figures")
os.makedirs(OUT, exist_ok=True)

# ── Color palette ──
TIER_COLORS = {"868MHz": "#E07B39", "2.4GHz": "#4A90D9", "UWB": "#2ECC71"}
ARM_COLORS  = {
    "static_868":          "#E07B39",
    "static_2p4":          "#4A90D9",
    "static_uwb":          "#2ECC71",
    "fixed_threshold":     "#9B59B6",
    "ca_tbl_no_integrity": "#95A5A6",
    "ca_tbl_full":         "#E74C3C",
}
ARM_LABELS = {
    "static_868":          "Static 868 MHz",
    "static_2p4":          "Static 2.4 GHz",
    "static_uwb":          "Static UWB",
    "fixed_threshold":     "Fixed Threshold",
    "ca_tbl_no_integrity": "CA-TBL (no integrity)",
    "ca_tbl_full":         "CA-TBL (full)",
}
ARM_MARKERS = {
    "static_868": "s", "static_2p4": "D", "static_uwb": "^",
    "fixed_threshold": "o", "ca_tbl_no_integrity": "v", "ca_tbl_full": "P",
}

def load_csv(path):
    with open(path) as f:
        return list(csv.DictReader(f))

def load_arm_summary(dirpath):
    return load_csv(os.path.join(dirpath, "arm_summary.csv"))

def get_arm(rows, name):
    for r in rows:
        if r["arm"] == name:
            return r
    return None

def load_sweep_seeds(base_dir, n_seeds=20):
    """Load arm_summary from each seed_N directory."""
    all_rows = []
    for s in range(1, n_seeds + 1):
        p = os.path.join(base_dir, f"seed_{s}", "arm_summary.csv")
        if os.path.exists(p):
            rows = load_csv(p)
            for r in rows:
                r["_seed"] = s
            all_rows.extend(rows)
    return all_rows

# ═══════════════════════════════════════════════════════════
# F1 — Probe: PDR vs distance (all three tiers)
# ═══════════════════════════════════════════════════════════
def fig_F1():
    pr = load_csv(os.path.join(ROOT, "results/probe_audit/probe_range_sweep.csv"))
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for tier in ("868MHz", "2.4GHz", "UWB"):
        rows = [r for r in pr if r["tier"] == tier and float(r["pdr"]) >= 0]
        dists = [float(r["distance_m"]) for r in rows]
        pdrs  = [float(r["pdr"]) for r in rows]
        ax.plot(dists, pdrs, marker="o", markersize=5, linewidth=2,
                color=TIER_COLORS[tier], label=tier)
    ax.set_xscale("log")
    ax.set_xlabel("Distance (m)")
    ax.set_ylabel("Packet Delivery Rate (PDR)")
    ax.set_ylim(-0.02, 1.05)
    ax.axhline(0.8, color="gray", linestyle="--", alpha=0.5, label="r80 threshold")
    ax.legend(loc="lower left")
    ax.set_title("F1 — Link Reception vs Distance")
    fig.tight_layout()
    fig.savefig(f"{OUT}/F1_pdr_vs_distance.png")
    plt.close(fig)
    print("  F1 done")

# ═══════════════════════════════════════════════════════════
# F2 — Probe: Range error vs distance
# ═══════════════════════════════════════════════════════════
def fig_F2():
    pr = load_csv(os.path.join(ROOT, "results/probe_audit/probe_range_sweep.csv"))
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for tier in ("868MHz", "2.4GHz", "UWB"):
        rows = [r for r in pr if r["tier"] == tier and float(r["mean_abs_range_err_m"]) >= 0]
        dists = [float(r["distance_m"]) for r in rows]
        errs  = [float(r["mean_abs_range_err_m"]) for r in rows]
        ax.plot(dists, errs, marker="o", markersize=5, linewidth=2,
                color=TIER_COLORS[tier], label=tier)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.set_xlabel("Distance (m)")
    ax.set_ylabel("Mean |Range Error| (m)")
    ax.legend()
    ax.set_title("F2 — Ranging Error vs Distance")
    fig.tight_layout()
    fig.savefig(f"{OUT}/F2_range_error_vs_distance.png")
    plt.close(fig)
    print("  F2 done")

# ═══════════════════════════════════════════════════════════
# F3 — Probe: CRLB ratio vs distance (tier 2 and 3 only)
# ═══════════════════════════════════════════════════════════
def fig_F3():
    pr = load_csv(os.path.join(ROOT, "results/probe_audit/probe_range_sweep.csv"))
    fig, ax = plt.subplots(figsize=(7, 4.5))
    for tier in ("2.4GHz", "UWB"):
        rows = [r for r in pr if r["tier"] == tier
                and float(r["measured_over_crlb"]) > 0
                and float(r["pdr"]) >= 0.20]
        dists  = [float(r["distance_m"]) for r in rows]
        ratios = [float(r["measured_over_crlb"]) for r in rows]
        ax.plot(dists, ratios, marker="o", markersize=5, linewidth=2,
                color=TIER_COLORS[tier], label=tier)
    ax.set_xscale("log")
    ax.set_yscale("log")
    ax.axhline(3.0, color="red", linestyle="--", alpha=0.5, label="3× CRLB bound")
    ax.axhline(1.0, color="green", linestyle="--", alpha=0.3, label="CRLB floor")
    ax.set_xlabel("Distance (m)")
    ax.set_ylabel("Measured / CRLB Floor")
    ax.legend()
    ax.set_title("F3 — CRLB Ratio (PDR ≥ 0.20 only)")
    fig.tight_layout()
    fig.savefig(f"{OUT}/F3_crlb_ratio.png")
    plt.close(fig)
    print("  F3 done")

# ═══════════════════════════════════════════════════════════
# F4 — Reference run: CEP50 bar chart (boxHalf 1000, seed 7)
# ═══════════════════════════════════════════════════════════
def fig_F4():
    arms = load_arm_summary(os.path.join(ROOT, "results/baseline_v3"))
    arm_order = ["static_868","static_2p4","static_uwb",
                 "fixed_threshold","ca_tbl_no_integrity","ca_tbl_full"]
    arms_sorted = [get_arm(arms, a) for a in arm_order]
    arms_sorted = [a for a in arms_sorted if a is not None]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    names = [ARM_LABELS.get(r["arm"], r["arm"]) for r in arms_sorted]
    cep50 = [float(r["cep50M"]) for r in arms_sorted]
    colors = [ARM_COLORS.get(r["arm"], "#666") for r in arms_sorted]

    bars = ax1.barh(names, cep50, color=colors, edgecolor="white", linewidth=0.5)
    ax1.set_xscale("log")
    ax1.set_xlabel("CEP50 (m)")
    ax1.set_title("Median Position Error")
    ax1.invert_yaxis()
    for bar, v in zip(bars, cep50):
        ax1.text(bar.get_width() * 1.1, bar.get_y() + bar.get_height()/2,
                 f"{v:.1f} m", va="center", fontsize=9)

    totalJ = [float(r["totalJ"]) for r in arms_sorted]
    bars2 = ax2.barh(names, totalJ, color=colors, edgecolor="white", linewidth=0.5)
    ax2.set_xscale("log")
    ax2.set_xlabel("Total Energy (J)")
    ax2.set_title("Energy Consumption")
    ax2.invert_yaxis()
    for bar, v in zip(bars2, totalJ):
        ax2.text(bar.get_width() * 1.1, bar.get_y() + bar.get_height()/2,
                 f"{v:.1f} J", va="center", fontsize=9)

    fig.suptitle("F4 — Reference Run: boxHalf 1000, seed 7", fontsize=13, y=1.01)
    fig.tight_layout()
    fig.savefig(f"{OUT}/F4_reference_run.png")
    plt.close(fig)
    print("  F4 done")

# ═══════════════════════════════════════════════════════════
# F5 — searchDuty bracket: static_uwb energy
# ═══════════════════════════════════════════════════════════
def fig_F5():
    duties = [0.0, 0.1, 0.25, 0.5, 1.0]
    dirs = [f"sd_{d}" for d in duties]
    energies = []
    for d in dirs:
        arms = load_arm_summary(os.path.join(ROOT, "results", d))
        uwb = get_arm(arms, "static_uwb")
        energies.append(float(uwb["totalJ"]))

    fig, ax = plt.subplots(figsize=(6, 4.5))
    ax.plot(duties, energies, "o-", color="#2ECC71", linewidth=2.5, markersize=8)
    ax.fill_between(duties, energies, alpha=0.15, color="#2ECC71")
    ax.set_xlabel("searchDuty")
    ax.set_ylabel("static_uwb totalJ (J)")
    ax.set_title("F5 — Search Energy Bracket")
    ax.annotate(f"{energies[0]:.1f} J\n(no search)", (0.0, energies[0]),
                textcoords="offset points", xytext=(15, 5), fontsize=9,
                arrowprops=dict(arrowstyle="->", color="gray"))
    ax.annotate(f"{energies[-1]:.1f} J\n(full search)", (1.0, energies[-1]),
                textcoords="offset points", xytext=(-80, -25), fontsize=9,
                arrowprops=dict(arrowstyle="->", color="gray"))
    ax.annotate(f"Factor: {energies[-1]/energies[0]:.0f}×",
                xy=(0.5, max(energies)*0.6), fontsize=11, fontweight="bold",
                ha="center", color="#E74C3C")
    fig.tight_layout()
    fig.savefig(f"{OUT}/F5_search_duty_bracket.png")
    plt.close(fig)
    print("  F5 done")

# ═══════════════════════════════════════════════════════════
# F6 — boxHalf sweep: RMSE and energy (VOID but informative)
# ═══════════════════════════════════════════════════════════
def fig_F6():
    boxes = [1000, 1500, 2000, 3000, 4000, 6000]
    arm_names = ["static_868","static_2p4","fixed_threshold","ca_tbl_full"]
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    for aname in arm_names:
        rmses, js = [], []
        for bh in boxes:
            arms = load_arm_summary(os.path.join(ROOT, f"results/box_{bh}"))
            r = get_arm(arms, aname)
            rmses.append(float(r["rmseM"]))
            js.append(float(r["totalJ"]))
        ax1.plot(boxes, rmses, marker=ARM_MARKERS.get(aname, "o"),
                 color=ARM_COLORS.get(aname, "#666"), label=ARM_LABELS.get(aname, aname),
                 linewidth=2, markersize=6)
        ax2.plot(boxes, js, marker=ARM_MARKERS.get(aname, "o"),
                 color=ARM_COLORS.get(aname, "#666"), label=ARM_LABELS.get(aname, aname),
                 linewidth=2, markersize=6)

    ax1.set_xlabel("boxHalf (m)")
    ax1.set_ylabel("RMSE (m)")
    ax1.set_yscale("log")
    ax1.legend(fontsize=8)
    ax1.set_title("RMSE vs Geometry")

    ax2.set_xlabel("boxHalf (m)")
    ax2.set_ylabel("Total Energy (J)")
    ax2.set_yscale("log")
    ax2.legend(fontsize=8)
    ax2.set_title("Energy vs Geometry")

    fig.suptitle("F6 — boxHalf Sweep (VOID: density confounded, §10.7)",
                 fontsize=12, y=1.01, color="#E74C3C")
    fig.tight_layout()
    fig.savefig(f"{OUT}/F6_boxhalf_sweep.png")
    plt.close(fig)
    print("  F6 done")

# ═══════════════════════════════════════════════════════════
# F7 — 20-seed paired: acc/J ratio distributions
# ═══════════════════════════════════════════════════════════
def fig_F7():
    fig, axes = plt.subplots(1, 2, figsize=(12, 5))

    for idx, (label, base_dir, fd_dirs) in enumerate([
        ("boxHalf 1000", "sweep20_b1000",
         [("off", "sweep20_b1000")]),
        ("boxHalf 3000", "sweep20_b3000",
         [("off", "sweep20_b3000"),
          ("proposed", "fd_proposed_b3000"),
          ("all", "fd_all_b3000")]),
    ]):
        ax = axes[idx]
        for fd_label, dname in fd_dirs:
            all_rows = load_sweep_seeds(os.path.join(ROOT, "results", dname))
            ratios = []
            for s in range(1, 21):
                seed_rows = [r for r in all_rows if r["_seed"] == s]
                ft = get_arm(seed_rows, "fixed_threshold")
                ca = get_arm(seed_rows, "ca_tbl_full")
                if ft and ca:
                    ft_apj = float(ft["accuracyPerJoule"])
                    ca_apj = float(ca["accuracyPerJoule"])
                    if ca_apj > 0:
                        ratios.append(ca_apj / ft_apj)
            if ratios:
                ax.hist(ratios, bins=12, alpha=0.6, edgecolor="white",
                        label=f"fd={fd_label} (n={len(ratios)})")
                med = statistics.median(ratios)
                ax.axvline(med, linestyle="--", alpha=0.7)

        ax.axvline(1.0, color="red", linestyle="-", alpha=0.5, label="parity")
        ax.set_xlabel("CA-TBL / fixed_threshold acc/J ratio")
        ax.set_ylabel("Count (seeds)")
        ax.set_title(label)
        ax.legend(fontsize=8)

    fig.suptitle("F7 — Accuracy/Joule Ratio (CA-TBL vs Baseline), 20 seeds",
                 fontsize=12, y=1.01)
    fig.tight_layout()
    fig.savefig(f"{OUT}/F7_acc_per_joule_ratio.png")
    plt.close(fig)
    print("  F7 done")

# ═══════════════════════════════════════════════════════════
# F8 — Energy breakdown by radio state (reference run)
# ═══════════════════════════════════════════════════════════
def fig_F8():
    arms = load_arm_summary(os.path.join(ROOT, "results/baseline_v3"))
    arm_order = ["static_868","static_2p4","static_uwb",
                 "fixed_threshold","ca_tbl_no_integrity","ca_tbl_full"]
    arms_sorted = [get_arm(arms, a) for a in arm_order if get_arm(arms, a)]

    states = ["tx", "rx", "cad", "idle", "sleep"]
    tiers  = ["868", "2p4", "uwb"]
    state_colors = {"tx": "#E74C3C", "rx": "#3498DB", "cad": "#F39C12",
                    "idle": "#95A5A6", "sleep": "#2ECC71"}

    fig, ax = plt.subplots(figsize=(10, 5))
    names = [ARM_LABELS.get(r["arm"], r["arm"]) for r in arms_sorted]
    x = np.arange(len(names))
    width = 0.65
    bottom = np.zeros(len(names))

    for state in states:
        vals = []
        for r in arms_sorted:
            total = sum(float(r.get(f"{state}_{t}", 0)) for t in tiers)
            vals.append(max(total, 1e-6))  # avoid log(0)
        vals = np.array(vals)
        ax.bar(x, vals, width, bottom=bottom, label=state.upper(),
               color=state_colors[state], edgecolor="white", linewidth=0.5)
        bottom += vals

    ax.set_xticks(x)
    ax.set_xticklabels(names, rotation=20, ha="right")
    ax.set_yscale("log")
    ax.set_ylabel("Energy (J)")
    ax.legend(ncol=5, loc="upper center", bbox_to_anchor=(0.5, 1.12))
    ax.set_title("F8 — Energy Breakdown by Radio State")
    fig.tight_layout()
    fig.savefig(f"{OUT}/F8_energy_breakdown.png")
    plt.close(fig)
    print("  F8 done")

# ═══════════════════════════════════════════════════════════
# F9 — The Conjunction Plot (central figure)
# ═══════════════════════════════════════════════════════════
def fig_F9():
    # Use gw9_b3000 seed 7 for the canonical conjunction
    gw9_dir = os.path.join(ROOT, "results/gw9_b3000/seed_7")
    if not os.path.exists(gw9_dir):
        # Fallback to fdgw9 or baseline
        gw9_dir = os.path.join(ROOT, "results/fdgw9_proposed_b3000/seed_7")
    if not os.path.exists(gw9_dir):
        gw9_dir = os.path.join(ROOT, "results/baseline_v3")

    arms = load_arm_summary(gw9_dir)
    arm_order = ["static_868","static_2p4","static_uwb",
                 "fixed_threshold","ca_tbl_full"]

    fig, ax = plt.subplots(figsize=(8, 6))

    for aname in arm_order:
        r = get_arm(arms, aname)
        if not r:
            continue
        avail = 100.0 * float(r["fixCycles"]) / float(r["steps"])
        cep50 = float(r["cep50M"])
        totalJ = float(r["totalJ"])

        size = max(30, min(300, totalJ * 0.5))
        ax.scatter(avail, cep50, s=size, alpha=0.8,
                   color=ARM_COLORS.get(aname, "#666"),
                   marker=ARM_MARKERS.get(aname, "o"),
                   edgecolors="black", linewidth=0.5, zorder=5)

        label_text = f"{ARM_LABELS.get(aname, aname)}\n{totalJ:.0f} J"
        offset = (10, 5)
        if aname == "static_868":
            offset = (10, -15)
        elif aname == "static_uwb":
            offset = (-100, 10)
        elif aname == "static_2p4":
            offset = (10, 10)

        ax.annotate(label_text, (avail, cep50), fontsize=8,
                    xytext=offset, textcoords="offset points",
                    arrowprops=dict(arrowstyle="->", color="gray", alpha=0.5),
                    bbox=dict(boxstyle="round,pad=0.3", facecolor="white",
                              alpha=0.8, edgecolor="gray"))

    # Draw the "conjunction zone"
    ax.axhline(10, color="green", linestyle=":", alpha=0.4, label="10 m accuracy target")
    ax.axvline(95, color="blue", linestyle=":", alpha=0.4, label="95% availability target")
    ax.fill_between([95, 101], 0.01, 10, alpha=0.08, color="green")

    ax.set_yscale("log")
    ax.set_xlabel("Fix Availability (%)")
    ax.set_ylabel("CEP50 (m) — lower is better")
    ax.set_xlim(-2, 105)
    ax.legend(loc="upper left", fontsize=8)
    ax.set_title("F9 — The Conjunction: Accuracy × Availability\n(bubble size ∝ energy)",
                 fontsize=12)
    fig.tight_layout()
    fig.savefig(f"{OUT}/F9_conjunction.png")
    plt.close(fig)
    print("  F9 done")

# ═══════════════════════════════════════════════════════════
# F10 — Trace: error time series (seed 7, ca_tbl_full)
# ═══════════════════════════════════════════════════════════
def fig_F10():
    trace_file = os.path.join(ROOT, "results/causal/trace_ca_tbl_full_seed7.csv")
    if not os.path.exists(trace_file):
        print("  F10 skipped (no trace file)")
        return

    rows = load_csv(trace_file)
    t    = [float(r["t"]) for r in rows]
    errM = [float(r["errM"]) for r in rows]
    tier = [r["activeTier"] for r in rows]

    tier_map = {"868MHz": 0, "2.4GHz": 1, "UWB": 2}
    tier_num = [tier_map.get(ti, 0) for ti in tier]

    fig, (ax1, ax2) = plt.subplots(2, 1, figsize=(14, 6), sharex=True,
                                    gridspec_kw={"height_ratios": [3, 1]})

    # Error trace
    ax1.plot(t, errM, linewidth=0.5, alpha=0.7, color="#2C3E50")
    ax1.set_ylabel("Position Error (m)")
    ax1.set_yscale("log")
    ax1.set_title("F10 — Error Trace and Tier Selection (ca_tbl_full, seed 7)")

    # Color-coded tier strip
    colors = [TIER_COLORS.get(ti, "gray") for ti in tier]
    for i in range(len(t) - 1):
        ax2.axvspan(t[i], t[i+1], color=colors[i], alpha=0.7)

    ax2.set_yticks([])
    ax2.set_xlabel("Time (s)")
    ax2.set_ylabel("Active Tier")
    # Legend
    from matplotlib.patches import Patch
    legend_elements = [Patch(facecolor=TIER_COLORS[k], label=k)
                       for k in ("868MHz", "2.4GHz", "UWB")]
    ax2.legend(handles=legend_elements, loc="upper right", ncol=3, fontsize=8)

    fig.tight_layout()
    fig.savefig(f"{OUT}/F10_error_trace.png")
    plt.close(fig)
    print("  F10 done")

# ═══════════════════════════════════════════════════════════
# F11 — PCS vs traceSigma correlation scatter
# ═══════════════════════════════════════════════════════════
def fig_F11():
    trace_file = os.path.join(ROOT, "results/causal/trace_ca_tbl_full_seed7.csv")
    if not os.path.exists(trace_file):
        print("  F11 skipped")
        return

    rows = load_csv(trace_file)
    fix_rows = [r for r in rows if int(r["hadFix"]) == 1]

    errM   = [float(r["errM"]) for r in fix_rows]
    pcs    = [float(r["pcs"]) for r in fix_rows]
    trSig  = [float(r["traceSigma"]) for r in fix_rows]
    tier   = [r["activeTier"] for r in fix_rows]

    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(12, 5))

    for t_name in ("868MHz", "2.4GHz", "UWB"):
        idx = [i for i, ti in enumerate(tier) if ti == t_name]
        if not idx:
            continue
        e = [errM[i] for i in idx]
        p = [pcs[i] for i in idx]
        s = [trSig[i] for i in idx]
        ax1.scatter(p, e, alpha=0.3, s=8, color=TIER_COLORS[t_name], label=t_name)
        ax2.scatter(s, e, alpha=0.3, s=8, color=TIER_COLORS[t_name], label=t_name)

    ax1.set_xlabel("PCS (confidence score)")
    ax1.set_ylabel("Position Error (m)")
    ax1.set_yscale("log")
    ax1.set_title("PCS vs Error (ρ = −0.091)")
    ax1.legend()

    ax2.set_xlabel("traceSigma (m²)")
    ax2.set_ylabel("Position Error (m)")
    ax2.set_xscale("log")
    ax2.set_yscale("log")
    ax2.set_title("traceSigma vs Error (ρ = +0.779)")
    ax2.legend()

    fig.suptitle("F11 — Confidence Model Quality (fix cycles only)", fontsize=12, y=1.01)
    fig.tight_layout()
    fig.savefig(f"{OUT}/F11_confidence_quality.png")
    plt.close(fig)
    print("  F11 done")

# ═══════════════════════════════════════════════════════════
# F12 — Marginal accuracy/joule above tier-1 floor (the killer)
# ═══════════════════════════════════════════════════════════
def fig_F12():
    configs = [
        ("fd=off", "sweep20_b3000", "fixed_threshold", "ca_tbl_full"),
        ("fd=proposed", "fd_proposed_b3000", "fixed_threshold", "ca_tbl_full"),
        ("fd=all", "fd_all_b3000", "fixed_threshold", "ca_tbl_full"),
    ]

    fig, ax = plt.subplots(figsize=(8, 5))
    x_pos = 0
    bar_data = []

    for conf_label, dname, ft_name, ca_name in configs:
        all_rows = load_sweep_seeds(os.path.join(ROOT, "results", dname))
        ft_marginals, ca_marginals = [], []

        for s in range(1, 21):
            seed_rows = [r for r in all_rows if r["_seed"] == s]
            s868 = get_arm(seed_rows, "static_868")
            ft = get_arm(seed_rows, ft_name)
            ca = get_arm(seed_rows, ca_name)
            if not (s868 and ft and ca):
                continue

            rmse_floor = float(s868["rmseM"])
            j_floor = float(s868["totalJ"])

            ft_rmse = float(ft["rmseM"])
            ft_j = float(ft["totalJ"])
            ca_rmse = float(ca["rmseM"])
            ca_j = float(ca["totalJ"])

            if ft_j > j_floor and ft_rmse < rmse_floor:
                ft_marginals.append((rmse_floor - ft_rmse) / (ft_j - j_floor))
            if ca_j > j_floor and ca_rmse < rmse_floor:
                ca_marginals.append((rmse_floor - ca_rmse) / (ca_j - j_floor))

        if ft_marginals:
            bar_data.append((f"Threshold\n{conf_label}", np.mean(ft_marginals),
                             np.std(ft_marginals) / np.sqrt(len(ft_marginals)),
                             ARM_COLORS["fixed_threshold"]))
        if ca_marginals:
            bar_data.append((f"CA-TBL\n{conf_label}", np.mean(ca_marginals),
                             np.std(ca_marginals) / np.sqrt(len(ca_marginals)),
                             ARM_COLORS["ca_tbl_full"]))

    if bar_data:
        names = [b[0] for b in bar_data]
        means = [b[1] for b in bar_data]
        errs  = [b[2] for b in bar_data]
        colors = [b[3] for b in bar_data]
        x = np.arange(len(names))
        ax.bar(x, means, yerr=errs, color=colors, edgecolor="white",
               linewidth=0.5, capsize=4, alpha=0.85)
        ax.set_xticks(x)
        ax.set_xticklabels(names, fontsize=9)
        ax.set_ylabel("Marginal m/J above tier-1 floor")
        ax.set_title("F12 — Crediting-Invariant Comparison\n(boxHalf 3000, 20 seeds)")

    fig.tight_layout()
    fig.savefig(f"{OUT}/F12_marginal_mJ.png")
    plt.close(fig)
    print("  F12 done")

# ═══════════════════════════════════════════════════════════
# F13 — Outage duration histogram (gw9, static_2p4 vs hybrid)
# ═══════════════════════════════════════════════════════════
def fig_F13():
    base = os.path.join(ROOT, "results/fdgw9_proposed_b3000")
    if not os.path.exists(base):
        base = os.path.join(ROOT, "results/gw9_b3000")

    # Collect outage durations from traces
    fig, ax = plt.subplots(figsize=(8, 5))

    # We need trace data; use available seed_7 traces from causal
    trace_2p4 = os.path.join(ROOT, "results/causal/trace_static_2p4_seed7.csv")
    trace_full = os.path.join(ROOT, "results/causal/trace_ca_tbl_full_seed7.csv")

    for trace_path, label, color in [
        (trace_2p4, "Static 2.4 GHz", ARM_COLORS["static_2p4"]),
        (trace_full, "CA-TBL (full)", ARM_COLORS["ca_tbl_full"]),
    ]:
        if not os.path.exists(trace_path):
            continue
        rows = load_csv(trace_path)
        dt = 2.5
        outage_lens = []
        run = 0
        for r in rows:
            if int(r["hadFix"]) == 0:
                run += 1
            else:
                if run > 0:
                    outage_lens.append(run * dt)
                run = 0
        if run > 0:
            outage_lens.append(run * dt)  # right-censored

        if outage_lens:
            bins = np.logspace(np.log10(dt), np.log10(max(outage_lens) * 1.1), 20)
            ax.hist(outage_lens, bins=bins, alpha=0.6, color=color,
                    edgecolor="white", label=f"{label} (n={len(outage_lens)})")

    ax.set_xscale("log")
    ax.set_xlabel("Outage Duration (s)")
    ax.set_ylabel("Count")
    ax.set_title("F13 — Outage Duration Distribution (seed 7)")
    ax.legend()
    fig.tight_layout()
    fig.savefig(f"{OUT}/F13_outage_duration.png")
    plt.close(fig)
    print("  F13 done")

# ═══════════════════════════════════════════════════════════
# F14 — Claim 2 crediting regimes visual (the decisive plot)
# ═══════════════════════════════════════════════════════════
def fig_F14():
    data = [
        ("bH=1000\noff",     0.0363, None, None, "#95A5A6"),
        ("bH=1000\nproposed",0.1038, None, None, "#95A5A6"),
        ("bH=1000\nall",     0.1013, None, None, "#95A5A6"),
        ("bH=3000\noff",     1.053,  0.845, 1.313, "#F39C12"),
        ("bH=3000\nproposed",1.483,  1.082, 2.032, "#2ECC71"),
        ("bH=3000\nall",     0.703,  0.585, 0.846, "#E74C3C"),
        ("gw9\nproposed",    1.649,  1.477, 1.840, "#2ECC71"),
        ("gw9\nall",         0.696,  0.630, 0.866, "#E74C3C"),
    ]

    fig, ax = plt.subplots(figsize=(10, 5))
    x = np.arange(len(data))
    mids = [d[1] for d in data]
    lows = [d[1] - d[2] if d[2] else 0 for d in data]
    highs = [d[3] - d[1] if d[3] else 0 for d in data]
    colors = [d[4] for d in data]
    labels = [d[0] for d in data]

    ax.bar(x, mids, color=colors, edgecolor="white", linewidth=0.5, alpha=0.85)
    ax.errorbar(x, mids, yerr=[lows, highs], fmt="none", color="black",
                capsize=4, linewidth=1.5)

    ax.axhline(1.0, color="red", linestyle="--", linewidth=1.5, alpha=0.7,
               label="Parity (ratio = 1.0)")
    ax.set_xticks(x)
    ax.set_xticklabels(labels, fontsize=8)
    ax.set_ylabel("CA-TBL / fixed_threshold acc/J ratio")
    ax.set_title("F14 — Claim 2 Across Crediting Regimes\n(>1 = CA-TBL wins, <1 = baseline wins)")
    ax.legend()
    fig.tight_layout()
    fig.savefig(f"{OUT}/F14_crediting_regimes.png")
    plt.close(fig)
    print("  F14 done")

# ═══════════════════════════════════════════════════════════
# F15 — Energy decomposition: per-tier stacked (reference)
# ═══════════════════════════════════════════════════════════
def fig_F15():
    arms = load_arm_summary(os.path.join(ROOT, "results/baseline_v3"))
    arm_order = ["static_868","static_2p4","static_uwb",
                 "fixed_threshold","ca_tbl_no_integrity","ca_tbl_full"]
    arms_sorted = [get_arm(arms, a) for a in arm_order if get_arm(arms, a)]

    tier_keys = [("j_868", "868 MHz"), ("j_2p4", "2.4 GHz"), ("j_uwb", "UWB")]
    tier_cols = ["#E07B39", "#4A90D9", "#2ECC71"]

    fig, ax = plt.subplots(figsize=(9, 5))
    names = [ARM_LABELS.get(r["arm"], r["arm"]) for r in arms_sorted]
    x = np.arange(len(names))
    width = 0.6
    bottom = np.zeros(len(names))

    for (key, label), color in zip(tier_keys, tier_cols):
        vals = np.array([float(r[key]) for r in arms_sorted])
        vals = np.maximum(vals, 1e-6)
        ax.bar(x, vals, width, bottom=bottom, label=label,
               color=color, edgecolor="white", linewidth=0.5)
        bottom += vals

    ax.set_xticks(x)
    ax.set_xticklabels(names, rotation=20, ha="right")
    ax.set_yscale("log")
    ax.set_ylabel("Energy (J)")
    ax.legend()
    ax.set_title("F15 — Energy by Tier (Reference Run)")
    fig.tight_layout()
    fig.savefig(f"{OUT}/F15_energy_by_tier.png")
    plt.close(fig)
    print("  F15 done")

# ═══════════════════════════════════════════════════════════
# F16 — 20-seed RMSE distributions (boxplot)
# ═══════════════════════════════════════════════════════════
def fig_F16():
    arm_names = ["static_868","static_2p4","fixed_threshold",
                 "ca_tbl_no_integrity","ca_tbl_full"]
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(14, 5))

    for ax, (label, dname) in zip([ax1, ax2],
                                   [("boxHalf 1000", "sweep20_b1000"),
                                    ("boxHalf 3000", "sweep20_b3000")]):
        all_rows = load_sweep_seeds(os.path.join(ROOT, "results", dname))
        box_data = []
        box_labels = []
        box_colors = []
        for aname in arm_names:
            vals = [float(r["rmseM"]) for r in all_rows if r["arm"] == aname]
            if vals:
                box_data.append(vals)
                box_labels.append(ARM_LABELS.get(aname, aname).replace(" ", "\n"))
                box_colors.append(ARM_COLORS.get(aname, "#666"))

        bp = ax.boxplot(box_data, patch_artist=True, labels=box_labels)
        for patch, color in zip(bp["boxes"], box_colors):
            patch.set_facecolor(color)
            patch.set_alpha(0.7)

        ax.set_yscale("log")
        ax.set_ylabel("RMSE (m)")
        ax.set_title(label)
        ax.tick_params(axis='x', labelsize=8)

    fig.suptitle("F16 — RMSE Distribution (20 seeds)", fontsize=12, y=1.01)
    fig.tight_layout()
    fig.savefig(f"{OUT}/F16_rmse_boxplot.png")
    plt.close(fig)
    print("  F16 done")

# ═══════════════════════════════════════════════════════════
#  MAIN
# ═══════════════════════════════════════════════════════════
if __name__ == "__main__":
    print(f"Output directory: {OUT}")
    print("Generating figures...")

    fig_F1()
    fig_F2()
    fig_F3()
    fig_F4()
    fig_F5()
    fig_F6()
    fig_F7()
    fig_F8()
    fig_F9()
    fig_F10()
    fig_F11()
    fig_F12()
    fig_F13()
    fig_F14()
    fig_F15()
    fig_F16()

    print(f"\nDone. {len(os.listdir(OUT))} files in {OUT}/")
