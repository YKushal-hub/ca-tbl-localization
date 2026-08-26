#!/usr/bin/env python3
"""Closed-form propagation: path loss, link budget, PDR(d), r80.

INDEPENDENT REIMPLEMENTATION. This module must not import from the ns-3 scenario
or the physics engines -- a shared implementation cannot validate itself. It reads
the engines' OUTPUT (probe_range_sweep.csv) only to compare against.
"""
import math

C = 299792458.0
K_BOLTZ_DBM_HZ = -174.0


def friis_ref_db(freq_hz, d0_m=1.0):
    """Free-space loss at the reference distance. 20log10(4*pi*d0*f/c)."""
    return 20.0 * math.log10(4.0 * math.pi * d0_m * freq_hz / C)


def path_loss_db(d_m, freq_hz, n, d0_m=1.0, extra_db=0.0):
    """Log-distance: PL(d) = PL(d0) + 10n log10(d/d0) + extra."""
    if d_m <= 0:
        d_m = 1e-6
    return friis_ref_db(freq_hz, d0_m) + 10.0 * n * math.log10(max(d_m, d0_m) / d0_m) + extra_db


def noise_floor_dbm(bw_hz, nf_db):
    """kTB + NF. -174 dBm/Hz at 290 K."""
    return K_BOLTZ_DBM_HZ + 10.0 * math.log10(bw_hz) + nf_db


def _phi(x):
    """Standard normal CDF."""
    return 0.5 * (1.0 + math.erf(x / math.sqrt(2.0)))


def pdr_lognormal(d_m, freq_hz, n, tx_dbm, sens_dbm, sigma_shadow_db,
                  d0_m=1.0, extra_db=0.0, gain_db=0.0):
    """P(received) with log-normal shadowing.

    margin(d) = tx + gain - PL(d) - sens ; reception iff shadowing draw < margin,
    so PDR = Phi(margin / sigma). Deterministic step as sigma -> 0, which is the
    P8 lower-limit sanity anchor.
    """
    margin = tx_dbm + gain_db - path_loss_db(d_m, freq_hz, n, d0_m, extra_db) - sens_dbm
    if sigma_shadow_db <= 1e-9:
        return 1.0 if margin >= 0.0 else 0.0
    return _phi(margin / sigma_shadow_db)


def r80(freq_hz, n, tx_dbm, sens_dbm, sigma_shadow_db, d0_m=1.0, extra_db=0.0,
        gain_db=0.0, lo=0.5, hi=100000.0, target=0.80):
    """Distance at which PDR falls to `target`. Bisection; PDR is monotone in d."""
    f = lambda d: pdr_lognormal(d, freq_hz, n, tx_dbm, sens_dbm, sigma_shadow_db,
                                d0_m, extra_db, gain_db) - target
    if f(lo) < 0:
        return float("nan")
    if f(hi) > 0:
        return float("inf")
    for _ in range(200):
        mid = math.sqrt(lo * hi)
        if f(mid) > 0:
            lo = mid
        else:
            hi = mid
    return math.sqrt(lo * hi)


def fit_exponent(distances, pl_db, freq_hz, d0_m=1.0):
    """Least-squares n and residual offset from a measured PL curve.

    Used ONLY to report what exponent the engine's own curve implies. Feeding this
    back into the PDR prediction would make the comparison circular, so the caller
    must state which of the two tests it is running.
    """
    xs = [10.0 * math.log10(max(d, d0_m) / d0_m) for d in distances]
    ref = friis_ref_db(freq_hz, d0_m)
    ys = [p - ref for p in pl_db]
    sx = sum(xs); sy = sum(ys); sxx = sum(x * x for x in xs); sxy = sum(x * y for x, y in zip(xs, ys))
    m = len(xs)
    den = m * sxx - sx * sx
    if abs(den) < 1e-12:
        return float("nan"), float("nan")
    slope = (m * sxy - sx * sy) / den
    inter = (sy - slope * sx) / m
    return slope, inter
