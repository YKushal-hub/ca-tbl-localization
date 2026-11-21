"""System-side priors for the collapse estimators (DD 38).

Only generic, published values live here: device PHY settings, published ranging accuracy, and
ranges spanning the published rubble physics. Nothing here is a World parameter of a particular
run: the World's bracket (low/central/high) is never known to the system. Each constant names its
row in docs/SOURCES_COLLAPSE.md (or, for earlier work, the design note it came from).
"""
import math

# Device transmit power, dBm (main simulator PHY; DESIGN_DECISIONS / ca_tbl_causal_sim.cc phy[]).
TX_DBM = {"868": 14.0, "2400": 12.5, "uwb": -16.47}
# Carrier, Hz (engine configs: 868e6; 2.44e9 "Fixed 2.4GHz channel"; 7.99e9 "DW3000 Channel 9").
FREQ_HZ = {"868": 868e6, "2400": 2.44e9, "uwb": 7.99e9}

# Published time-of-flight ranging accuracy (1 sigma), m, line of sight.
# 2.4 GHz: Zhou et al., arXiv:2509.23125 (2025), SX1280 outdoors (tofLosFloorM, lora-physics-parameters.h).
# UWB: Benouakta et al., Electronics 2023, 10-33 cm for DW3000-class hardware (uwbLosFloorM).
TOF_SIGMA_M = {"2400": 3.0, "uwb": 0.10}

# Loss-to-delay ratio of rubble, dB of excess loss per metre of excess apparent distance:
# rho_b = A_b / (n - 1), from rows OES-LAYER-PLAIN / OES-LAYER-FERRO / OES-MEAS-80 / UWB8-* (A_b at
# the engine carrier) and SLOW-BR (n - 1), as printed by build/scratch/ns3.47-ca_tbl_rubble_selftest-optimized.
# The prior spans all three brackets; the system does not know which one the World uses.
#   2.44 GHz: low 26.0131/0.454039 = 57.29; central 38.9406/0.703760 = 55.33; high 100.4919/1.541129 = 65.21
#   7.99 GHz: low 59.0931/0.454039 = 130.15; central 91.5942/0.703760 = 130.15; high 219.4549/1.541129 = 142.40
#   868 MHz: low 6.63464/0.454039 = 14.61; central 11.9020/0.703760 = 16.91; high 30.7148/1.541129 = 19.93
RHO_RANGE = {"868": (14.61, 19.93), "2400": (55.33, 65.21), "uwb": (130.15, 142.40)}

# Specific loss per metre of pile at the carrier, dB/m, prior span (low .. high bracket), DD 36.
A_RANGE = {"868": (6.634642, 30.714836), "2400": (26.013062, 100.491890), "uwb": (59.093052, 219.454924)}


def rho_mid(band):
    lo, hi = RHO_RANGE[band]
    return 0.5 * (lo + hi)


def rho_rel_sd(band):
    """Relative 1-sigma of rho under a uniform prior on its range."""
    lo, hi = RHO_RANGE[band]
    return (hi - lo) / math.sqrt(12.0) / rho_mid(band)


def free_space_l0(band):
    """Friis loss at 1 m, dB: 20 log10(4 pi f / c)."""
    return 20.0 * math.log10(4.0 * math.pi * FREQ_HZ[band] / 299792458.0)
