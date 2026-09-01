#!/usr/bin/env python3
"""Independent link-budget model. Computes the margin; never reads it.

WHY THIS FILE EXISTS SEPARATELY FROM validate.py
------------------------------------------------
The previous validate.py read `mean_margin_db` from the probe CSV and applied a
shadowing distribution to it. The link budget -- the quantity §7 exists to validate --
was imported from the subject. Detuning K_BOLTZ_DBM_HZ by 14 dB and a tier's TX power
by 16 dB both produced BYTE-IDENTICAL output, because neither constant was on the
path.

The separation is STRUCTURAL, not a convention: nothing in this file opens, imports or
receives the probe CSV. The probe's margin column cannot be in scope here because the
probe is not in scope here. A later edit cannot silently reintroduce the defect
without first adding an import that does not belong.

EVERY CONSTANT CARRIES ITS file:line FROM THE ENGINE SOURCES.
"""
import math

C_LIGHT = 299792458.0

# --- noise -----------------------------------------------------------------
K_BOLTZ_DBM_HZ = -174.0   # lora-physics-engine.cc:541
NOISE_FIGURE_DB = 6.0     # lora-physics-engine.cc:542

# --- receiver decision ------------------------------------------------------
SOFT_THRESHOLD_WIDTH_DB = 0.35   # lora-physics-parameters.cc:389
# Reception is a LOGISTIC draw, not a hard threshold: 1/(1+exp(-margin/width)).
# lora-physics-engine.cc:135-146. At 0.35 dB it is nearly a step, but the form is
# reproduced because a hard threshold disagrees near the transition for a reason that
# is documented rather than physical (DEFECTS.md D4).

# --- per-tier PHY ----------------------------------------------------------
# ca_tbl_causal_sim.cc:3571-3573  (sf, bandwidthHz, txPowerDbm)
TIER_PHY = {
    "868MHz": {"sf": 7, "bw": 125e3,   "tx": 14.0,    "f": 868e6,
               "f_src": "lora-physics-parameters.cc:12"},
    "2.4GHz": {"sf": 7, "bw": 800e3,   "tx": 12.5,    "f": 2.44e9,
               "f_src": "lora-2p4-physics-parameters.cc:52"},
    "UWB":    {"sf": 7, "bw": 499.2e6, "tx": -16.47,  "f": 7.99e9,
               "f_src": "lora-uwb-physics-parameters.cc:52"},
}

# --- sensitivity at the configured SF7 / CR 4/5 -----------------------------
# The accessor is clamp(sf-5,0,7) for the eight-entry SF5-based tables and
# clamp(sf-7,0,5) for the six-entry SF7-based one, after a4e23ad corrected the index.
SENSITIVITY_DBM = {
    "868MHz": -123.0,   # lora-physics-parameters.cc:391, six entries, index 0 = SF7
    "2.4GHz": -106.0,   # lora-2p4-physics-parameters.cc:14, eight entries, index 2 = SF7
    "UWB":    -104.0,   # lora-uwb-physics-parameters.cc:14, flat across all eight
}
CODING_RATE_OFFSET_DB = 0.0   # crIndex 0 at codingRateDenom=5, offset 0.0
                              # lora-physics-parameters.cc:397, :425

# --- environment -----------------------------------------------------------
# The probe puts tier 1 in the wide-area environment and tiers 2/3 at facilities.
# ca_tbl_causal_sim.cc:3143 env=ENV_RURAL_FORESTED, :3144 envFacility=ENV_RURAL_OPEN
ENV = {
    "RURAL_OPEN": {
        "n": 2.2,                 # lora-physics-parameters.cc:24
        "shadow_sd": 5.0,         # lora-physics-parameters.cc:79
        "nlos_sigma_ratio": 1.25, # lora-physics-parameters.cc:79
        "nlos_scale_m": 54000.0,  # lora-physics-parameters.cc:187
        "nlos_atten_mu": 6.0, "nlos_atten_sd": 5.0,   # lora-physics-parameters.cc:187
        "diffraction_mu": 4.0, "diffraction_sd": 3.0, # lora-physics-parameters.cc:187, :209
    },
    "RURAL_FORESTED": {
        "n": 2.3,                 # lora-physics-parameters.cc:27, Normal(2.3,0.1,2.1,2.6)
        "shadow_sd": 7.0,         # lora-physics-parameters.cc:81
        "nlos_sigma_ratio": 1.3,  # lora-physics-parameters.cc:81
        "nlos_scale_m": 8000.0,   # lora-physics-parameters.cc:189
        "nlos_atten_mu": 10.0, "nlos_atten_sd": 7.0,  # lora-physics-parameters.cc:189
        "diffraction_mu": 5.0, "diffraction_sd": 3.0, # lora-physics-parameters.cc:189, :209
    },
}
TIER_ENV = {"868MHz": "RURAL_FORESTED", "2.4GHz": "RURAL_OPEN", "UWB": "RURAL_OPEN"}

# --- foliage ---------------------------------------------------------------
# ITU-R P.833-10, lora-foliage-submodule.cc:36:
#     A_veg = A_max * (1 - exp(-depth * gamma / A_max))
# depth ~ LogNormal(ln(100), 0.6, 0, 500)   lora-physics-parameters.cc:293
# gamma, A_max per vegetation type          lora-physics-parameters.cc:304-309
# vegWide=VEG_DENSE_FOREST, vegFacility=VEG_SPARSE_SHRUBS
#                                           ca_tbl_causal_sim.cc:3174-3175
#
# DISTANCE-INDEPENDENT: depth is a per-link draw, not a function of link length. This
# term can explain a constant offset and never a slope. Stated here because the
# measured gap has both, and only one of them is this.
VEGETATION = {
    "VEG_DENSE_FOREST":  {"gamma": 0.20, "amax": 25.0},  # lora-physics-parameters.cc:309
    "VEG_SPARSE_SHRUBS": {"gamma": 0.04, "amax": 8.0},   # lora-physics-parameters.cc:305
}
TIER_VEG = {"868MHz": "VEG_DENSE_FOREST",
            "2.4GHz": "VEG_SPARSE_SHRUBS",
            "UWB": "VEG_SPARSE_SHRUBS"}
FOLIAGE_DEPTH_LOG_MU = math.log(100.0)   # lora-physics-parameters.cc:293
FOLIAGE_DEPTH_LOG_SD = 0.6               # lora-physics-parameters.cc:293
FOLIAGE_DEPTH_MAX_M = 500.0              # lora-physics-parameters.cc:293
FOLIAGE_CLAMP_DB = 35.0                  # lora-physics-engine.cc:527


def foliage_loss_db(tier, u_depth, overrides=None):
    """A_veg for one realization. u_depth is a unit-normal draw from the corpus."""
    o = overrides or {}
    veg = dict(VEGETATION[TIER_VEG[tier]])
    veg.update(o.get("veg", {}))
    depth = math.exp(FOLIAGE_DEPTH_LOG_MU + FOLIAGE_DEPTH_LOG_SD * u_depth)
    depth = min(max(depth, 0.0), FOLIAGE_DEPTH_MAX_M)
    g, amax = veg["gamma"], veg["amax"]
    if depth <= 0.0 or g <= 0.0 or amax <= 0.0:
        return 0.0
    a = amax * (1.0 - math.exp(-depth * g / amax))
    return min(max(a, 0.0), FOLIAGE_CLAMP_DB)
REFERENCE_DISTANCE_M = 1.0   # lora-physics-parameters.cc:11

# --- two-ray ground reflection ---------------------------------------------
# lora-two-ray-ground-submodule.cc:10-39. THIS REPLACES THE PATH LOSS, IT DOES NOT ADD
# TO IT: the engine calls ComputePathLossDb only in the `else` branch
# (lora-physics-engine.cc:389-394), and the submodule's own comment at :37 reads
# "We overwrite the propagation loss entirely for true open field two-ray".
# Adding a term here would double-count; the existing term is conditioned instead.
#
# Gated twice, and the gating is what explains the measured two_ray_frac:
#   env must be ENV_RURAL_OPEN   lora-two-ray-ground-submodule.cc:11
#   the link must be LOS         lora-two-ray-ground-submodule.cc:17
# 868 MHz is RURAL_FORESTED so it never fires (two_ray_frac 0.0000 at every distance);
# 2.4 GHz and UWB are RURAL_OPEN so it fires on every LOS link, and measured
# two_ray_frac + nlos_frac = 1.0000 exactly at every 2.4 GHz distance.
#
# Breakpoint is 4*PI*h_t*h_r/lambda -- note the PI, which an earlier hand calculation
# of mine omitted, making the breakpoints PI times too small.
TWO_RAY_ENV = "RURAL_OPEN"     # lora-two-ray-ground-submodule.cc:11
# The engine takes the heights from the POSITIONS, not from the parameter
# distributions: lora-physics-engine.cc:385-386 uses txPosition.z / rxPosition.z and
# falls back to the distribution mean only when z <= 0. The probe registers real
# anchors, so the fallback never fires and Normal(30,10,10,50) is never consulted.
# Sourced from the probe scenario, ca_tbl_causal_sim.cc:1116-1117.
NODE_HEIGHT_M = 1.5                                          # :1116 nodeH
ANCHOR_HEIGHT_M = {"868MHz": 15.0, "2.4GHz": 6.0, "UWB": 2.5}  # :1117 anchorH
# These are exact positions, not distribution summaries, so there is no
# evaluate-at-the-mean bias to register here. The first implementation DID use the
# distribution mean (30 m) and was wrong by ~10 dB on 2.4 GHz at long range; averaging
# two-ray over the height distribution was then tested and recovered under 1 dB of that,
# which is what ruled the distribution out and pointed at the registered positions.


def slant_distance_m(d_m, h_t, h_r):
    """The engine computes distance from the 3-D positions, so a link at horizontal
    range d spans sqrt(d^2 + dh^2). At short range against a tall anchor this is not a
    rounding detail: at 10 m under the 6 m 2.4 GHz anchor it is 0.80 dB."""
    return math.sqrt(d_m * d_m + (h_t - h_r) ** 2)


def two_ray_breakpoint_m(f_hz, h_t, h_r=NODE_HEIGHT_M):
    """dc = 4*pi*h_t*h_r/lambda. lora-two-ray-ground-submodule.cc:26"""
    lam = C_LIGHT / f_hz
    return (4.0 * math.pi * h_t * h_r) / lam


def two_ray_path_loss_db(f_hz, d_m, h_t, h_r=NODE_HEIGHT_M):
    """Hard switch at dc, not a smooth interpolation.
    lora-two-ray-ground-submodule.cc:29-35"""
    d = max(d_m, 1.0)
    lam = C_LIGHT / f_hz
    dc = two_ray_breakpoint_m(f_hz, h_t, h_r)
    if d <= dc:
        return 20.0 * math.log10(4.0 * math.pi * d / lam)
    return 40.0 * math.log10(d) - 20.0 * math.log10(h_t) - 20.0 * math.log10(h_r)


def fspl_at_reference(f_hz, d0=REFERENCE_DISTANCE_M):
    """20*log10(4*pi*d0/lambda). lora-propagation-submodule.cc:31"""
    lam = C_LIGHT / f_hz
    return 20.0 * math.log10(4.0 * math.pi * max(d0, 1.0) / lam)


def path_loss_db(f_hz, d_m, n, d0=REFERENCE_DISTANCE_M):
    """FSPL(d0) + 10*n*log10(d/d0). lora-propagation-submodule.cc:50

    The dual-slope and urban branches are not reached for RURAL_OPEN or
    RURAL_FORESTED: enableDualSlope is false for both
    (lora-physics-parameters.cc:28, and :17 by default).
    """
    d = max(d_m, 1.0)
    d0 = max(d0, 1.0)
    return fspl_at_reference(f_hz, d0) + 10.0 * n * math.log10(d / d0)


def noise_floor_dbm(bw_hz, nf_db=NOISE_FIGURE_DB):
    """kTB + 10log10(BW) + NF. lora-physics-engine.cc:543"""
    return K_BOLTZ_DBM_HZ + 10.0 * math.log10(bw_hz) + nf_db


def nlos_probability(d_m, scale_m):
    """1 - exp(-d/scale). lora-nlos-submodule.cc:47, non-urban branch."""
    return 1.0 - math.exp(-max(d_m, 1.0) / max(scale_m, 1.0))


def nlos_excess_db(u_nlos, u_atten, u_diff, d_m, env):
    """Excess loss on a link drawn NLOS. lora-nlos-submodule.cc:84-93.

    THIS IS THE TERM THAT SETS THE SLOPE. DEFECTS.md records the measured local slope
    at r80 as 58.2 dB/decade against a path-loss exponent giving 24.76 -- the
    difference is excess loss accumulating with distance, and a model computing only
    FSPL plus n underestimates it by more than a factor of two. It is built in here
    rather than discovered as a disagreement.
    """
    e = ENV[env]
    p = nlos_probability(d_m, e["nlos_scale_m"])
    if u_nlos >= p:
        return 0.0                      # link is LOS, zero penalty
    atten = e["nlos_atten_mu"] + e["nlos_atten_sd"] * u_atten
    diff = e["diffraction_mu"] + e["diffraction_sd"] * u_diff
    cap = (e["nlos_atten_mu"] + e["diffraction_mu"]) + 3.0 * e["nlos_atten_sd"]
    return max(0.0, min(atten + diff, cap))


def rx_power_dbm(tier, d_m, u_nlos, u_atten, u_diff, u_depth=0.0, overrides=None):
    """tx + gains - path_loss - nlos_excess - foliage. Gains are 0 dBi: the probe
    registers isotropic anchors and the engine's antennaGainDb defaults to 0."""
    o = overrides or {}
    phy = dict(TIER_PHY[tier]); phy.update(o.get("phy", {}))
    env_name = TIER_ENV[tier]
    n = o.get("n", ENV[env_name]["n"])
    h_t = o.get("h_t", ANCHOR_HEIGHT_M[tier])
    h_r = o.get("h_r", NODE_HEIGHT_M)
    # The engine works in 3-D throughout, so every distance-dependent term takes the
    # slant range, not the horizontal range it was swept at.
    d3 = slant_distance_m(d_m, h_t, h_r)
    # Two-ray REPLACES the log-distance path loss when it fires -- it is not additive.
    is_nlos = u_nlos < nlos_probability(d3, ENV[env_name]["nlos_scale_m"])
    if env_name == TWO_RAY_ENV and not is_nlos and not o.get("no_two_ray"):
        pl = two_ray_path_loss_db(phy["f"], d3, h_t, h_r)
    else:
        pl = path_loss_db(phy["f"], d3, n)
    ex = nlos_excess_db(u_nlos, u_atten, u_diff, d3, env_name)
    fol = foliage_loss_db(tier, u_depth, o)
    return phy["tx"] - pl - ex - fol


def margin_db(tier, d_m, u_nlos, u_atten, u_diff, u_depth=0.0, overrides=None):
    """rx_power - sensitivity. The quantity the probe's mean_margin_db is compared
    AGAINST, and which is computed here without reference to it."""
    o = overrides or {}
    sens = o.get("sensitivity", SENSITIVITY_DBM[tier]) + CODING_RATE_OFFSET_DB
    return rx_power_dbm(tier, d_m, u_nlos, u_atten, u_diff, u_depth, o) - sens


def reception_probability(margin, width=SOFT_THRESHOLD_WIDTH_DB):
    """Logistic, lora-physics-engine.cc:141-143."""
    x = margin / max(width, 1e-6)
    if x > 60.0:
        return 1.0
    if x < -60.0:
        return 0.0
    return 1.0 / (1.0 + math.exp(-x))


def pdr_at(tier, d_m, draws, overrides=None):
    """PDR averaged over corpus realizations. The DECISION is averaged, never Phi(mean).

    draws: list of (u_shadow, u_nlos, u_atten, u_diff) unit-normalised per realization.
    """
    o = overrides or {}
    env_name = TIER_ENV[tier]
    sd_los = o.get("shadow_sd", ENV[env_name]["shadow_sd"])
    ratio = ENV[env_name]["nlos_sigma_ratio"]
    scale = ENV[env_name]["nlos_scale_m"]
    tot = 0.0
    for (u_sh, u_nl, u_at, u_df) in draws:
        # u_df doubles as the foliage depth draw: both are unit-normal and independent
        # per realization in the corpus.
        m = margin_db(tier, d_m, u_nl, u_at, u_df, u_df, o)
        sd = sd_los * (ratio if u_nl < nlos_probability(d_m, scale) else 1.0)
        tot += reception_probability(m + sd * u_sh)
    return tot / len(draws)


def r80(tier, draws, distances, overrides=None):
    """Distance at which PDR crosses 0.80, linearly interpolated."""
    prev = None
    for d in distances:
        p = pdr_at(tier, d, draws, overrides)
        if prev is not None:
            (d0, p0) = prev
            if p0 >= 0.80 > p:
                return d0 + (p0 - 0.80) / (p0 - p) * (d - d0)
        prev = (d, p)
    return float("nan")
