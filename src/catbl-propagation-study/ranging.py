#!/usr/bin/env python3
"""Estimation theory: CRLB ranging sigma, GDOP, position sigma."""
import math

C = 299792458.0


def crlb_sigma_m(bw_hz, snr_lin):
    """ToA Cramer-Rao bound: sigma >= c / (2*pi*BW*sqrt(2*SNR)).

    This is the term that makes the architecture tiered: sigma falls as 1/BW, so
    precision is bought with bandwidth. Path loss rises as f^2, so range is bought
    with low frequency. Bandwidth and frequency are not independent in any real
    allocation, which is the whole trade.
    """
    if bw_hz <= 0 or snr_lin <= 0:
        return float("inf")
    return C / (2.0 * math.pi * bw_hz * math.sqrt(2.0 * snr_lin))


def db_to_lin(db):
    return 10.0 ** (db / 10.0)


def gdop(anchor_azimuths_rad):
    """sqrt(trace((H^T H)^-1)) for a 2-D fix from unit bearing vectors."""
    n = len(anchor_azimuths_rad)
    if n < 3:
        return float("inf")
    a = b = c = 0.0
    for th in anchor_azimuths_rad:
        cx, cy = math.cos(th), math.sin(th)
        a += cx * cx; b += cx * cy; c += cy * cy
    det = a * c - b * b
    if abs(det) < 1e-12:
        return float("inf")          # collinear -- the P15 degenerate case
    return math.sqrt((c + a) / det)  # trace of the inverse


def position_sigma_m(range_sigma_m, gdop_val):
    return range_sigma_m * gdop_val
