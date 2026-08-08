#!/usr/bin/env python3
"""
Compute Spearman rank correlations between confidence signals and
realised position error from CA-TBL per-step trace CSVs.

Usage:
    python3 scripts/compute_rho.py results/causal/
    python3 scripts/compute_rho.py results/baseline_v5/

Reads every trace_*_seed*.csv in the directory. For each arm, filters to
fix cycles (hadFix == 1, nAnchors >= 3) and reports:

    pcs        vs errM   — the claim's target (should be negative)
    traceSigma vs errM   — the baseline control (should be positive, strong)
    1/traceSigma vs errM — monotone control (should score ≈ −0.78 by construction;
                           hitting the target is necessary but not sufficient)

A PCS that merely relabels traceSigma will score ≈ the monotone control.
A PCS that adds information will score *better* than the control.
"""

import csv
import glob
import os
import sys
from collections import defaultdict

# Spearman without scipy: rank, then Pearson on ranks.
def _rank(xs):
    """Average-rank assignment."""
    n = len(xs)
    indexed = sorted(range(n), key=lambda i: xs[i])
    ranks = [0.0] * n
    i = 0
    while i < n:
        j = i
        while j < n - 1 and xs[indexed[j + 1]] == xs[indexed[j]]:
            j += 1
        avg = (i + j) / 2.0 + 1.0  # 1-based
        for k in range(i, j + 1):
            ranks[indexed[k]] = avg
        i = j + 1
    return ranks


def spearman(xs, ys):
    """Spearman rank correlation. Returns (rho, n)."""
    assert len(xs) == len(ys)
    n = len(xs)
    if n < 3:
        return (float('nan'), n)
    rx = _rank(xs)
    ry = _rank(ys)
    mx = sum(rx) / n
    my = sum(ry) / n
    num = sum((rx[i] - mx) * (ry[i] - my) for i in range(n))
    dx = sum((rx[i] - mx) ** 2 for i in range(n))
    dy = sum((ry[i] - my) ** 2 for i in range(n))
    denom = (dx * dy) ** 0.5
    if denom == 0:
        return (float('nan'), n)
    return (num / denom, n)


TIER_NAMES = {0: '868', 1: '2p4', 2: 'UWB'}


def load_trace(path):
    """Load a trace CSV, return list of dicts with floats where possible."""
    with open(path) as f:
        reader = csv.DictReader(f)
        rows = []
        for r in reader:
            row = {}
            for k, v in r.items():
                try:
                    row[k] = float(v)
                except (ValueError, TypeError):
                    row[k] = v
            rows.append(row)
    return rows


def analyse_arm(rows, arm_name):
    """Compute correlations for one arm's trace. Returns a dict of results."""
    # Filter to fix cycles
    fix_rows = [r for r in rows
                if r.get('hadFix', 0) == 1.0
                and r.get('nAnchors', 0) >= 3
                and r.get('errM', 0) > 0
                and r.get('pcs', 0) > 0
                and r.get('traceSigma', 0) > 0]

    if len(fix_rows) < 10:
        return None

    errs = [r['errM'] for r in fix_rows]
    pcs_vals = [r['pcs'] for r in fix_rows]
    ts_vals = [r['traceSigma'] for r in fix_rows]
    inv_ts = [1.0 / r['traceSigma'] for r in fix_rows]

    result = {
        'arm': arm_name,
        'tier': 'pooled',
        'n': len(fix_rows),
        'rho_pcs': spearman(pcs_vals, errs)[0],
        'rho_traceSigma': spearman(ts_vals, errs)[0],
        'rho_inv_traceSigma': spearman(inv_ts, errs)[0],
    }

    # Per-tier breakdown
    per_tier = {}
    for tier_idx, tier_name in TIER_NAMES.items():
        tier_rows = [r for r in fix_rows if r.get('activeTier', -1) == tier_idx]
        if len(tier_rows) < 10:
            continue
        te = [r['errM'] for r in tier_rows]
        tp = [r['pcs'] for r in tier_rows]
        tt = [r['traceSigma'] for r in tier_rows]
        ti = [1.0 / r['traceSigma'] for r in tier_rows]
        per_tier[tier_name] = {
            'arm': arm_name,
            'tier': tier_name,
            'n': len(tier_rows),
            'rho_pcs': spearman(tp, te)[0],
            'rho_traceSigma': spearman(tt, te)[0],
            'rho_inv_traceSigma': spearman(ti, te)[0],
        }

    return result, per_tier


def main():
    if len(sys.argv) < 2:
        print(f"Usage: {sys.argv[0]} <results_dir> [results_dir2 ...]")
        sys.exit(1)

    all_results = []

    for results_dir in sys.argv[1:]:
        traces = sorted(glob.glob(os.path.join(results_dir, 'trace_*_seed*.csv')))
        if not traces:
            print(f"No trace files found in {results_dir}")
            continue

        print(f"\n{'='*80}")
        print(f"  ρ measurement: {results_dir}")
        print(f"  {len(traces)} trace files")
        print(f"{'='*80}\n")

        # Header
        print(f"  {'arm':<25s} {'tier':<8s} {'n':>6s}  {'ρ(PCS,err)':>12s}  "
              f"{'ρ(σ,err)':>12s}  {'ρ(1/σ,err)':>12s}")
        print(f"  {'-'*25} {'-'*8} {'-'*6}  {'-'*12}  {'-'*12}  {'-'*12}")

        for trace_path in traces:
            fname = os.path.basename(trace_path)
            # Extract arm name: trace_<arm>_seed<N>.csv
            parts = fname.replace('trace_', '').replace('.csv', '')
            # Find last _seed<N>
            seed_idx = parts.rfind('_seed')
            if seed_idx < 0:
                continue
            arm_name = parts[:seed_idx]

            rows = load_trace(trace_path)
            result = analyse_arm(rows, arm_name)
            if result is None:
                print(f"  {arm_name:<25s} {'pooled':<8s} {'<10':>6s}  {'n/a':>12s}  "
                      f"{'n/a':>12s}  {'n/a':>12s}")
                continue

            pooled, per_tier = result

            # Print pooled
            print(f"  {pooled['arm']:<25s} {pooled['tier']:<8s} {pooled['n']:>6d}  "
                  f"{pooled['rho_pcs']:>+12.4f}  "
                  f"{pooled['rho_traceSigma']:>+12.4f}  "
                  f"{pooled['rho_inv_traceSigma']:>+12.4f}")
            all_results.append(pooled)

            # Print per-tier
            for tn in ['868', '2p4', 'UWB']:
                if tn in per_tier:
                    pt = per_tier[tn]
                    print(f"  {'':25s} {pt['tier']:<8s} {pt['n']:>6d}  "
                          f"{pt['rho_pcs']:>+12.4f}  "
                          f"{pt['rho_traceSigma']:>+12.4f}  "
                          f"{pt['rho_inv_traceSigma']:>+12.4f}")
                    all_results.append(pt)

        print()

    # Write CSV summary
    if all_results:
        out_path = os.path.join(sys.argv[1], 'rho_summary.csv')
        with open(out_path, 'w', newline='') as f:
            writer = csv.DictWriter(f, fieldnames=[
                'arm', 'tier', 'n', 'rho_pcs', 'rho_traceSigma', 'rho_inv_traceSigma'
            ])
            writer.writeheader()
            writer.writerows(all_results)
        print(f"  Wrote {out_path}")


if __name__ == '__main__':
    main()
