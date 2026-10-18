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
# The engine re-derives foliage per tier by ITU-R P.833's f^0.75 from 868 MHz, multiplying the
# MEAN of specificAttenuationDbPerM only (lora-2p4-physics-parameters.cc:77, x2.17;
# lora-uwb-physics-parameters.cc:116, x5.28). The bounds stay at 0.2*gamma and 1.8*gamma of the
# unscaled value, and SampleNormal CLAMPS (lora-distribution-sampler.cc:76), so the draws pile
# up at the upper bound: E[gamma] = 1.79x at 2.4 GHz and 1.80x at UWB, not 2.17x and 5.28x.
# Omitted here until 2026-10-09 (HANDOFF_2026_10_08 item 7); "veg_scale" overrides it.
TIER_VEG_SCALE = {"868MHz": 1.0, "2.4GHz": 2.17, "UWB": 5.28}
FOLIAGE_DEPTH_LOG_MU = math.log(100.0)   # lora-physics-parameters.cc:293
FOLIAGE_DEPTH_LOG_SD = 0.6               # lora-physics-parameters.cc:293
FOLIAGE_DEPTH_MAX_M = 500.0              # lora-physics-parameters.cc:293
FOLIAGE_CLAMP_DB = 35.0                  # lora-physics-engine.cc:527


# The engine SAMPLES both foliage parameters per realization, it does not use their
# means: lora-physics-parameters.cc:318-319
#     specificAttenuationDbPerM = Normal(gamma, gamma*0.25, gamma*0.2, gamma*1.8)
#     maxAttenuationDb          = Normal(amax,  amax*0.2,   amax*0.5,  amax*2.0)
# A_veg saturates in both, so E[A_veg] < A_veg(E[gamma], E[A_max]) and evaluating at the
# means over-attenuates. THIRD instance of this class here, after the lognormal depth and
# the distributed antenna heights; see experiments/JENSEN_FOLIAGE_REGISTRATION.md.
# Deterministic quadrature, so no corpus draw is added and its digest is untouched.
_VEG_NODES = {}


def _veg_nodes(g0, a0, k=21):
    key = (g0, a0, k)
    if key in _VEG_NODES:
        return _VEG_NODES[key]
    def tn(mu, sd, lo, hi):
        xs = [(lo + (hi - lo) * (i + 0.5) / k, 0.0) for i in range(k)]
        xs = [(x, math.exp(-0.5 * ((x - mu) / sd) ** 2)) for x, _ in xs]
        tot = sum(w for _, w in xs)
        return [(x, w / tot) for x, w in xs]
    nodes = [(g, a, wg * wa)
             for g, wg in tn(g0, g0 * 0.25, g0 * 0.2, g0 * 1.8)
             for a, wa in tn(a0, a0 * 0.2, a0 * 0.5, a0 * 2.0)]
    _VEG_NODES[key] = nodes
    return nodes


def _veg_nodes_scaled(g0, a0, scale, k=41):
    """The engine's draw when only gamma's mean is scaled: Normal(scale*g0, 0.25*g0) CLAMPED to
    [0.2*g0, 1.8*g0], so the mass beyond each bound sits AT the bound; A_max as in _veg_nodes."""
    key = (g0, a0, scale, k)
    if key in _VEG_NODES:
        return _VEG_NODES[key]
    mu, sd, lo, hi = g0 * scale, g0 * 0.25, g0 * 0.2, g0 * 1.8
    cdf = lambda x: 0.5 * (1.0 + math.erf((x - mu) / (sd * math.sqrt(2.0))))  # noqa: E731
    gs = [(lo, cdf(lo)), (hi, 1.0 - cdf(hi))]
    for i in range(k):
        x0, x1 = lo + (hi - lo) * i / k, lo + (hi - lo) * (i + 1) / k
        gs.append((0.5 * (x0 + x1), cdf(x1) - cdf(x0)))
    tot = sum(w for _, w in gs)
    an = [(a, wa) for g, a, wa in _veg_nodes(g0, a0) if g == _veg_nodes(g0, a0)[0][0]]
    ta = sum(w for _, w in an)
    nodes = [(g, a, (wg / tot) * (wa / ta)) for g, wg in gs for a, wa in an]
    _VEG_NODES[key] = nodes
    return nodes


def foliage_loss_db(tier, u_depth, overrides=None):
    """E[A_veg] for one realization's depth, integrated over the engine's gamma and
    A_max distributions. u_depth is a unit-normal draw from the corpus."""
    o = overrides or {}
    veg = dict(VEGETATION[TIER_VEG[tier]])
    veg.update(o.get("veg", {}))
    scale = o.get("veg_scale", TIER_VEG_SCALE[tier])
    depth = math.exp(FOLIAGE_DEPTH_LOG_MU + FOLIAGE_DEPTH_LOG_SD * u_depth)
    depth = min(max(depth, 0.0), FOLIAGE_DEPTH_MAX_M)
    g0, a0 = veg["gamma"], veg["amax"]
    if depth <= 0.0 or g0 <= 0.0 or a0 <= 0.0:
        return 0.0
    if o.get("veg_at_mean"):
        a = a0 * (1.0 - math.exp(-depth * g0 / a0))
    elif scale != 1.0:
        a = sum(w * a1 * (1.0 - math.exp(-depth * g1 / a1))
                for g1, a1, w in _veg_nodes_scaled(g0, a0, scale))
    else:
        a = sum(w * a1 * (1.0 - math.exp(-depth * g1 / a1))
                for g1, a1, w in _veg_nodes(g0, a0))
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


# --- Rician small-scale fading -------------------------------------------------
# lora-fading-submodule.cc:40-74. The engine draws a Rician envelope r with E[r^2] = 1
# and charges -20*log10(r) as loss (:74), clamped at -20 dB (lora-physics-engine.cc:451).
# CLASSIFICATION: PHYSICS, not transcription. The Rician envelope of a dominant component
# plus diffuse scatter is standard and is written here from its own definition; only the
# K-factor VALUES are taken from the engine, as parameters, and cited.
#   K ~ Normal(8, 2, 4, 15)  RURAL_OPEN      lora-physics-parameters.cc:110
#   K ~ Normal(3, 1, 1, 6)   RURAL_FORESTED  lora-physics-parameters.cc:116
# Only the LOS branch is modelled: nlos_frac is 0.000-0.004 on both RURAL_OPEN tiers, and
# on 868 the NLOS excess term already carries the obstruction. Registered as a known
# omission rather than an assumed-zero.
RICIAN_K = {"RURAL_OPEN": (8.0, 2.0, 4.0, 15.0),      # lora-physics-parameters.cc:110
            "RURAL_FORESTED": (3.0, 1.0, 1.0, 6.0)}   # lora-physics-parameters.cc:116
FADING_CLAMP_DB = -20.0   # lora-physics-engine.cc:451
_FADE_NODES = {}


def _log_rician_pdf(r, nu, s2):
    """log of the Rician envelope pdf, computed in log space.

    The naive form multiplies exp(-(r^2+nu^2)/(2 s2)) by I0(r*nu/s2). At large K the
    argument of I0 grows without bound (K=1000 gives ~2000) and a Taylor series for I0
    overflows, returning a wrong value silently -- it reported +7.5 dB of mean fade loss
    where the K->infinity limit must be 0 dB. Both limits are now asserted at import.
    """
    if r <= 0.0:
        return float("-inf")
    x = r * nu / s2
    if x < 500.0:
        # I0 by series, then take the log; safe in this range.
        t, acc = 1.0, 1.0
        for k in range(1, 200):
            t *= (x * x) / (4.0 * k * k)
            acc += t
            if t < 1e-17 * acc:
                break
        log_i0 = math.log(acc)
    else:
        # I0(x) ~ e^x / sqrt(2 pi x) for large x.
        log_i0 = x - 0.5 * math.log(2.0 * math.pi * x)
    return math.log(r / s2) - (r * r + nu * nu) / (2.0 * s2) + log_i0


def fading_nodes(env_name, n_r=160, n_k=9, k_override=None):
    """Deterministic (loss_db, weight) quadrature over the Rician fade distribution,
    integrated over the K-factor's own truncated normal. No RNG and no corpus draw, so
    adding fading does not disturb results/corpus.json or its digest."""
    cache = (env_name, k_override)
    if cache in _FADE_NODES:
        return _FADE_NODES[cache]
    if k_override is None:
        kmu, ksd, klo, khi = RICIAN_K[env_name]
    else:
        kmu = max(k_override, 1e-9); ksd = max(kmu * 0.25, 1e-9)
        klo, khi = max(kmu - 3 * ksd, 0.0), kmu + 3 * ksd
    ks = []
    for i in range(n_k):
        k = klo + (khi - klo) * (i + 0.5) / n_k
        ks.append((k, math.exp(-0.5 * ((k - kmu) / ksd) ** 2)))
    kw = sum(w for _, w in ks)
    nodes = []
    for k, wk in ks:
        nu = math.sqrt(k / (k + 1.0))
        s2 = 1.0 / (2.0 * (k + 1.0))
        rmax = nu + 5.0 * math.sqrt(s2)
        for j in range(n_r):
            r = rmax * (j + 0.5) / n_r
            # Rician pdf on the envelope
            lp = _log_rician_pdf(r, nu, s2)
            pdf = math.exp(lp) if lp > -700.0 else 0.0
            loss = max(-20.0 * math.log10(max(r, 1e-4)), FADING_CLAMP_DB)
            nodes.append((loss, pdf * (rmax / n_r) * (wk / kw)))
    tot = sum(w for _, w in nodes)
    nodes = [(l, w / tot) for l, w in nodes]
    _FADE_NODES[cache] = nodes
    return nodes


def mean_fading_loss_db(env_name, k_override=None):
    """E[-20 log10 r]. Positive is loss."""
    return sum(l * w for l, w in fading_nodes(env_name, k_override=k_override))


def _assert_fading_limits():
    """Rayleigh limit E[-20 log10 r] = 10*gamma/ln10 = 2.507 dB as K->0, and 0 dB as
    K->infinity. Derived before the code was trusted; both are checked here so a
    numerical failure cannot pass silently again."""
    lo = mean_fading_loss_db("RURAL_OPEN", 1e-6)
    hi = mean_fading_loss_db("RURAL_OPEN", 1e5)
    rayleigh = 10.0 * 0.5772156649015329 / math.log(10.0)
    assert abs(lo - rayleigh) < 0.05, "K->0 gives %.3f dB, Rayleigh is %.3f" % (lo, rayleigh)
    assert abs(hi) < 0.05, "K->infinity gives %.3f dB, must approach 0" % hi


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


def nlos_excess_db(u_nlos, u_atten, u_diff, d_m, env, overrides=None):
    """Excess loss on a link drawn NLOS. lora-nlos-submodule.cc:84-93.

    THIS IS THE TERM THAT SETS THE SLOPE. DEFECTS.md records the measured local slope
    at r80 as 58.2 dB/decade against a path-loss exponent giving 24.76 -- the
    difference is excess loss accumulating with distance, and a model computing only
    FSPL plus n underestimates it by more than a factor of two. It is built in here
    rather than discovered as a disagreement.
    """
    e = ENV[env]
    p = nlos_probability(d_m, (overrides or {}).get("nlos_scale_m", e["nlos_scale_m"]))
    if u_nlos >= p:
        return 0.0                      # link is LOS, zero penalty
    atten = e["nlos_atten_mu"] + e["nlos_atten_sd"] * u_atten
    diff = e["diffraction_mu"] + e["diffraction_sd"] * u_diff
    cap = (e["nlos_atten_mu"] + e["diffraction_mu"]) + 3.0 * e["nlos_atten_sd"]
    return max(0.0, min(atten + diff, cap))


KNOWN_OVERRIDES = frozenset((
    "phy", "n", "sensitivity", "shadow_sd", "veg", "veg_at_mean", "veg_scale",
    "h_t", "h_r", "no_two_ray", "no_fading", "margin_offset",
    "nlos_scale_m", "rician_k",
))


def _check_overrides(o):
    """An unrecognised override key is a sweep that runs and measures nothing. That
    failure has cost this project enough that it is now an exception, not a silent
    no-op: `nlos_scale_m` was swept for a whole experiment before anyone noticed the
    three read sites ignored it."""
    if o:
        bad = set(o) - KNOWN_OVERRIDES
        if bad:
            raise KeyError("unrecognised override(s): %s" % sorted(bad))


def rx_power_dbm(tier, d_m, u_nlos, u_atten, u_diff, u_depth=0.0, overrides=None):
    """tx + gains - path_loss - nlos_excess - foliage. Gains are 0 dBi: the probe
    registers isotropic anchors and the engine's antennaGainDb defaults to 0."""
    o = overrides or {}
    _check_overrides(o)
    phy = dict(TIER_PHY[tier]); phy.update(o.get("phy", {}))
    env_name = TIER_ENV[tier]
    n = o.get("n", ENV[env_name]["n"])
    h_t = o.get("h_t", ANCHOR_HEIGHT_M[tier])
    h_r = o.get("h_r", NODE_HEIGHT_M)
    # The engine works in 3-D throughout, so every distance-dependent term takes the
    # slant range, not the horizontal range it was swept at.
    d3 = slant_distance_m(d_m, h_t, h_r)
    # Two-ray REPLACES the log-distance path loss when it fires -- it is not additive.
    is_nlos = u_nlos < nlos_probability(
        d3, o.get("nlos_scale_m", ENV[env_name]["nlos_scale_m"]))
    if env_name == TWO_RAY_ENV and not is_nlos and not o.get("no_two_ray"):
        pl = two_ray_path_loss_db(phy["f"], d3, h_t, h_r)
    else:
        pl = path_loss_db(phy["f"], d3, n)
    ex = nlos_excess_db(u_nlos, u_atten, u_diff, d3, env_name, o)
    fol = foliage_loss_db(tier, u_depth, o)
    return phy["tx"] - pl - ex - fol


def margin_db(tier, d_m, u_nlos, u_atten, u_diff, u_depth=0.0, overrides=None):
    """rx_power - sensitivity. The quantity the probe's mean_margin_db is compared
    AGAINST, and which is computed here without reference to it."""
    o = overrides or {}
    sens = o.get("sensitivity", SENSITIVITY_DBM[tier]) + CODING_RATE_OFFSET_DB
    fade = (0.0 if o.get("no_fading")
            else mean_fading_loss_db(TIER_ENV[tier], o.get("rician_k")))
    # Robustness axes for the OFAT campaign. `margin_offset` is added to the margin:
    # terrain is applied as a NEGATIVE offset (extra loss), the unattributed-residual
    # perturbation as either sign. Neither is ever used to fit anything -- both exist so
    # that a conclusion can be recomputed under them and classified.
    return (rx_power_dbm(tier, d_m, u_nlos, u_atten, u_diff, u_depth, o) - sens - fade
            + o.get("margin_offset", 0.0))


def reception_probability(margin, width=SOFT_THRESHOLD_WIDTH_DB):
    """Logistic, lora-physics-engine.cc:141-143."""
    x = margin / max(width, 1e-6)
    if x > 60.0:
        return 1.0
    if x < -60.0:
        return 0.0
    return 1.0 / (1.0 + math.exp(-x))


_FADE_TABLE = {}
_FT_LO, _FT_HI, _FT_STEP = -60.0, 80.0, 0.05


def faded_reception_probability(x, env_name, k_override=None):
    """P(receive | pre-fading margin x), convolving the logistic with the Rician fade
    distribution. Tabulated once per environment on a 0.05 dB grid; the convolution is
    what averages the DECISION over fading, rather than applying a mean fade to the
    margin and then deciding once."""
    tab = _FADE_TABLE.get((env_name, k_override))
    if tab is None:
        nodes = fading_nodes(env_name, k_override=k_override)
        n = int((_FT_HI - _FT_LO) / _FT_STEP) + 1
        tab = []
        for i in range(n):
            m = _FT_LO + i * _FT_STEP
            tab.append(sum(w * reception_probability(m - l) for l, w in nodes))
        _FADE_TABLE[(env_name, k_override)] = tab
    if x <= _FT_LO:
        return tab[0]
    if x >= _FT_HI:
        return tab[-1]
    f = (x - _FT_LO) / _FT_STEP
    i = int(f)
    return tab[i] + (f - i) * (tab[i + 1] - tab[i])


def pdr_at(tier, d_m, draws, overrides=None):
    """PDR averaged over corpus realizations. The DECISION is averaged, never Phi(mean).

    draws: list of (u_shadow, u_nlos, u_atten, u_diff) unit-normalised per realization.
    """
    o = overrides or {}
    _check_overrides(o)
    env_name = TIER_ENV[tier]
    sd_los = o.get("shadow_sd", ENV[env_name]["shadow_sd"])
    ratio = ENV[env_name]["nlos_sigma_ratio"]
    scale = o.get("nlos_scale_m", ENV[env_name]["nlos_scale_m"])
    tot = 0.0
    for (u_sh, u_nl, u_at, u_df) in draws:
        # u_df doubles as the foliage depth draw: both are unit-normal and independent
        # per realization in the corpus.
        o2 = dict(o); o2["no_fading"] = True
        m = margin_db(tier, d_m, u_nl, u_at, u_df, u_df, o2)
        sd = sd_los * (ratio if u_nl < nlos_probability(d_m, scale) else 1.0)
        if o.get("no_fading"):
            tot += reception_probability(m + sd * u_sh)
        else:
            tot += faded_reception_probability(m + sd * u_sh, env_name,
                                               o.get("rician_k"))
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


_assert_fading_limits()
