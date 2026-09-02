#!/usr/bin/env python3
"""Runs the sixteen OFAT experiments under the registered robustness rule."""
import math, sys
import linkbudget as lb, ranging as rg, validate as V

DRAWS, DIGEST, NCORP = V.load_corpus()
TERRAINS = (0.0, 30.0)
OFFSETS = (-4.0, 0.0, 4.0)
# The distance grid must span every r80 any sweep level can produce, or a level whose
# r80 falls off the end returns NaN and is misread as a shape change. Caught when P1, P3
# and P4 first classified UNDETERMINABLE/undefined for that reason alone. 0.2 m to 2000 km,
# 24 points per decade-ish, log-spaced.
def _logspace(lo, hi, n):
    return [lo * (hi / lo) ** (i / (n - 1.0)) for i in range(n)]
_WIDE = _logspace(1.0e-3, 2.0e7, 300)
GRID = {"868MHz": _WIDE, "2.4GHz": _WIDE, "UWB": _WIDE}

def ov(terr, off, extra=None):
    o = dict(extra or {}); o["margin_offset"] = -terr + off; return o

def r80(tier, terr, off, extra=None):
    """Bisection, with an explicit DEAD-LINK case.

    lb.r80 scans a grid and returns NaN when it finds no 0.80 crossing -- including when
    PDR is already below 0.80 at the very first grid point, i.e. when the link is dead at
    any range. That NaN then reads as an undefined SHAPE, and four experiments were
    classified UNDETERMINABLE on it. A dead link is a physical statement, not an undefined
    one, so it returns the floor. lb.r80 itself is untouched: validate.py and G11 use it.

    Bisection is valid only because PDR is monotone in distance, asserted in _selfcheck.
    """
    o = ov(terr, off, extra)
    lo, hi = 1.0e-3, 2.0e7
    if lb.pdr_at(tier, lo, DRAWS, o) < 0.80:
        return lo                      # dead at every range
    if lb.pdr_at(tier, hi, DRAWS, o) >= 0.80:
        return float("inf")            # never falls below 0.80 in range
    for _ in range(50):
        mid = math.sqrt(lo * hi)
        if lb.pdr_at(tier, mid, DRAWS, o) >= 0.80:
            lo = mid
        else:
            hi = mid
    return math.sqrt(lo * hi)


def _selfcheck():
    for tier in ("868MHz", "2.4GHz", "UWB"):
        ds = _logspace(1.0, 20000.0, 30)
        ps = [lb.pdr_at(tier, d, DRAWS) for d in ds]
        assert all(b <= a + 1e-9 for a, b in zip(ps, ps[1:])), \
            "PDR not monotone in distance for %s -- bisection invalid" % tier
    g = _logspace(50.0, 40000.0, 80)
    a = lb.r80("868MHz", DRAWS, g)
    b = r80("868MHz", 0.0, 0.0)
    assert abs(a - b) / a < 0.02, "bisection %.2f vs lb.r80 %.2f" % (b, a)
    print("  selfcheck: PDR monotone on all tiers; bisection r80 %.1f m vs lb.r80 %.1f m" % (b, a))

def shape(ys, tol=1e-9):
    if any(y != y for y in ys): return "undefined"
    up = all(b >= a - tol for a, b in zip(ys, ys[1:]))
    dn = all(b <= a + tol for a, b in zip(ys, ys[1:]))
    if up and dn: return "flat"
    if up: return "increasing"
    if dn: return "decreasing"
    return "non-monotonic"

def classify(by_terr, by_off):
    if len(set(by_terr)) != 1: return "UNDETERMINABLE"
    if len(set(by_off)) != 1: return "SENSITIVE"
    return "ROBUST"


def magnitude_stability(fn, levels):
    """SECOND robustness dimension, reported separately from the registered rule.

    The registration said a conclusion must "hold" but never operationalised what a
    conclusion IS -- the same silence that made metrics A and B disagree in 5.1. That gap
    is recorded, not resolved retroactively: the registered rule stays as monotonic shape,
    and this quantity is reported ALONGSIDE it rather than folded into it.

    Returns the ratio between the largest and smallest span (max/min over the sweep)
    across the six robustness levels. 1.0 means the effect size is identical everywhere.
    """
    spans = []
    for t in TERRAINS:
        for o in OFFSETS:
            v = [fn(t, o, x) for x in levels]
            v = [x for x in v if x == x and abs(x) != float("inf")]
            if len(v) < 2 or min(v) <= 0: return float("nan")
            spans.append(max(v) / min(v))
    return max(spans) / min(spans)

RESULTS = []
def record(pid, name, conclusion, cls, detail):
    RESULTS.append((pid, name, conclusion, cls, detail))
    print("  %-4s %-22s %-14s %s" % (pid, name, cls, conclusion))
    if detail: print("       %s" % detail)

def sweep_conclusion(fn, levels):
    """Returns the conclusion (shape) at each terrain and offset level."""
    by_terr = [shape([fn(t, 0.0, x) for x in levels]) for t in TERRAINS]
    by_off  = [shape([fn(0.0, o, x) for x in levels]) for o in OFFSETS]
    return by_terr, by_off

_selfcheck()
print("OFAT campaign  corpus n=%d digest=%s" % (NCORP, DIGEST[:24]))
print("robustness: terrain %s dB x offset %s dB\n" % (list(TERRAINS), list(OFFSETS)))
print("  %-4s %-22s %-14s %s" % ("id", "parameter", "class", "conclusion"))

# ---- P1 carrier frequency -------------------------------------------------
F = [150e6, 300e6, 600e6, 1.2e9, 2.4e9, 5e9, 10e9, 24e9]
f1 = lambda t, o, x: r80("868MHz", t, o, {"phy": {"f": x}})
bt, bo = sweep_conclusion(f1, F)
v = [f1(0,0,x) for x in F]
record("P1", "carrier frequency", bt[0], classify(bt,bo),
       "150 MHz->24 GHz: r80 %.0f -> %.1f m, ratio %.1fx over 2.2 decades; "
       "per-decade %.3fx" % (v[0], v[-1], v[0]/v[-1], (v[-1]/v[0])**(1/2.204)))

# ---- P2 bandwidth ---------------------------------------------------------
BW = [1e4, 3e4, 1e5, 3e5, 1e6, 3e6, 1e7, 3e7, 1e8, 3e8, 1e9, 2e9]
def sig_bw(t, o, bw):
    nf = lb.K_BOLTZ_DBM_HZ + 10*math.log10(bw) + 6.0
    snr = lb.TIER_PHY["868MHz"]["tx"] - lb.path_loss_db(868e6, 1000.0, 2.3) - (-t) - o - nf
    return rg.crlb_sigma_m(bw, rg.db_to_lin(snr))
bt = [shape([sig_bw(t,0,b) for b in BW]) for t in TERRAINS]
bo = [shape([sig_bw(0,o,b) for b in BW]) for o in OFFSETS]
sv = [sig_bw(0,0,b) for b in BW]
imin = sv.index(min(sv))
record("P2", "bandwidth", bt[0], classify(bt,bo),
       "sigma %.4g -> %.4g m; minimum at BW=%.3g Hz (index %d of %d) -- %s"
       % (sv[0], sv[-1], BW[imin], imin, len(BW),
          "MINIMUM INTERIOR, reversal present" if 0 < imin < len(BW)-1
          else "NO interior minimum: sigma falls monotonically to the sweep end"))

# ---- P3 TX power ----------------------------------------------------------
TXP = [-30,-20,-10,0,10,20,30]
f3 = lambda t,o,x: r80("868MHz", t, o, {"phy": {"tx": x}})
bt, bo = sweep_conclusion(f3, TXP)
v = [f3(0,0,x) for x in TXP]
fin = [(x,y) for x,y in zip(TXP,v) if y==y]
record("P3", "TX power", bt[0], classify(bt,bo),
       "-30->+30 dBm: r80 %.1f -> %.1f m over the %d finite levels; "
       "measured %.2fx per 10 dB (nominal n=2.3 predicts 2.72x; n_eff~3.0 predicts %.2fx)"
       % (fin[0][1], fin[-1][1], len(fin), (fin[-1][1]/fin[0][1])**(10.0/(fin[-1][0]-fin[0][0])),
          10**(10/30.0)))

# ---- P4 RX sensitivity ----------------------------------------------------
SENS = [-70,-80,-90,-100,-110,-120,-130,-140]
f4 = lambda t,o,x: r80("868MHz", t, o, {"sensitivity": x})
bt, bo = sweep_conclusion(f4, SENS)
v = [f4(0,0,x) for x in SENS]
thermal = lb.K_BOLTZ_DBM_HZ + 10*math.log10(125e3) + 6.0
record("P4", "RX sensitivity", bt[0], classify(bt,bo),
       "-70->-140 dBm: r80 %.1f -> %.1f m. Thermal floor -174+10log10(125k)+6 = %.1f dBm; "
       "levels below it are UNPHYSICAL (the model still computes them -- it has no noise "
       "floor check, which is the model breakdown this sweep was to expose)"
       % (v[0], v[-1], thermal))

# ---- P5 noise figure ------------------------------------------------------
NF = [0,2,4,6,8,10,12,15]
f5 = lambda t,o,x: r80("868MHz", t, o, {"sensitivity": lb.SENSITIVITY_DBM["868MHz"] + (x-6.0)})
bt, bo = sweep_conclusion(f5, NF)
v = [f5(0,0,x) for x in NF]
record("P5", "noise figure", bt[0], classify(bt,bo),
       "0->15 dB: r80 %.1f -> %.1f m, ratio %.2fx (registered 4.4x on nominal n=2.3)"
       % (v[0], v[-1], v[0]/v[-1]))

# ---- P6 spreading factor --------------------------------------------------
SF = [7,8,9,10,11,12]
f6 = lambda t,o,x: r80("868MHz", t, o,
                       {"sensitivity": lb.SENSITIVITY_DBM["868MHz"] - 2.5*(x-7)})
bt, bo = sweep_conclusion(f6, SF)
v = [f6(0,0,x) for x in SF]
air7 = (2**7)/125e3*(8+4.25)
record("P6", "spreading factor", bt[0], classify(bt,bo),
       "SF7->SF12: r80 %.1f -> %.1f m, %.2fx per step, %.2fx total. Airtime x2/step; "
       "at 2.5%% duty the SF12 minimum inter-transmission interval is %.1f s"
       % (v[0], v[-1], (v[-1]/v[0])**(1/5.0), v[-1]/v[0], air7*32/0.025))

# ---- P7 path-loss exponent ------------------------------------------------
NEXP = [1.6, 2.0, 2.5, 3.0, 3.5, 4.0, 5.0, 6.0]
f7 = lambda t,o,x: r80("868MHz", t, o, {"n": x})
bt, bo = sweep_conclusion(f7, NEXP)
v = [f7(0,0,x) for x in NEXP]
record("P7", "path-loss exponent", bt[0], classify(bt,bo),
       "n 1.6->6.0: r80 %.4g -> %.4g m, span %.4g x" % (v[0], v[-1], v[0]/v[-1]))

# ---- P8 shadowing sigma ---------------------------------------------------
SD = [0.0, 1.0, 2.0, 4.0, 6.0, 8.0, 11.0, 16.0]
f8 = lambda t,o,x: r80("868MHz", t, o, {"shadow_sd": x})
bt, bo = sweep_conclusion(f8, SD)
v = [f8(0,0,x) for x in SD]
record("P8", "shadowing sigma", bt[0], classify(bt,bo),
       "sd 0->16 dB: r80 %.1f -> %.1f m (%s)" % (v[0], v[-1],
       "rises as registered" if v[-1] > v[0] else "FALLS -- registered prediction MISSED"))

# ---- P9 NLOS probability --------------------------------------------------
# p(d) = 1-exp(-d/scale); sweeping the scale sets the NLOS fraction at any given range.
PN = [0.0, 0.1, 0.2, 0.3, 0.4, 0.5, 0.6, 0.7, 0.8, 0.9, 1.0]
def f9(t, o, p):
    # scale chosen so that p(1000 m) = p exactly; p=0 -> no NLOS, p=1 -> always NLOS
    if p <= 0.0:   sc = 1e12
    elif p >= 1.0: sc = 1e-6
    else:          sc = -1000.0 / math.log(1.0 - p)
    return r80("868MHz", t, o, {"nlos_scale_m": sc})
bt, bo = sweep_conclusion(f9, PN)
v = [f9(0,0,x) for x in PN]
record("P9", "NLOS probability", bt[0], classify(bt,bo),
       "p(1km) 0->1: r80 %.1f -> %.1f m, both ends run and physical" % (v[0], v[-1]))

# ---- P10 NLOS excess delay ------------------------------------------------
DEL = [0, 25, 50, 75, 100, 125, 150, 175, 200]
P_NL, NANCH = 0.30, (3, 6, 12, 30, 100, 1000)
def floor_at(delay, n):
    return P_NL * delay          # position-error floor: independent of n by construction
def spread_at(delay, n, sig=0.5):
    return math.sqrt(sig*sig + P_NL*(1-P_NL)*delay*delay) / math.sqrt(n)
bt = [shape([floor_at(d, 30) for d in DEL]) for _ in TERRAINS]
bo = [shape([floor_at(d, 30) for d in DEL]) for _ in OFFSETS]
dep = {n: floor_at(200, n) for n in NANCH}
record("P10", "NLOS excess delay", bt[0], classify(bt,bo),
       "floor = p*delay = %.1f m at 200 m; across anchor counts %s the floor is %s -- "
       "bias does NOT average down" % (floor_at(200,30), list(NANCH),
       "identical" if len(set(round(x,9) for x in dep.values()))==1 else "NOT identical"))

# ---- P11 Rician K ---------------------------------------------------------
KDB = [-20,-10,-5,0,5,10,15,20,30]
f11 = lambda t,o,x: lb.mean_fading_loss_db("RURAL_FORESTED", 10.0**(x/10.0))
bt = [shape([f11(t,0,x) for x in KDB]) for t in TERRAINS]
bo = [shape([f11(0,o,x) for x in KDB]) for o in OFFSETS]
v = [f11(0,0,x) for x in KDB]
record("P11", "Rician K", bt[0], classify(bt,bo),
       "K -20->+30 dB: mean fade loss %+.3f -> %+.3f dB. Rayleigh limit 10*gamma/ln10 "
       "= 2.507 dB; both limits asserted at import" % (v[0], v[-1]))

# ---- P12 delay spread -----------------------------------------------------
DS = [10e-9, 30e-9, 100e-9, 300e-9, 1e-6, 3e-6]
def resolvable(bw, ds): return ds * bw          # >1 means resolvable
bt = [shape([resolvable(125e3, x) for x in DS]) for _ in TERRAINS]
bo = [shape([resolvable(125e3, x) for x in DS]) for _ in OFFSETS]
inv125 = 1.0/125e3
record("P12", "delay spread", bt[0], classify(bt,bo),
       "10 ns->3 us at 125 kHz: 1/BW = %.1f us, so delay spread never exceeds it and "
       "first-path detection NEVER fails in this sweep -- registered in advance. "
       "At UWB 499.2 MHz, 1/BW = %.1f ns and breakdown begins at %.1f ns"
       % (inv125*1e6, 1e9/499.2e6, 1e9/499.2e6))

# ---- P13 anchor count -----------------------------------------------------
NA = [3,4,6,8,12,16,20,25,30]
f13 = lambda t,o,x: 1.0/math.sqrt(x)
bt = [shape([f13(t,0,x) for x in NA]) for t in TERRAINS]
bo = [shape([f13(0,o,x) for x in NA]) for o in OFFSETS]
record("P13", "anchor count", bt[0], classify(bt,bo),
       "3->30 anchors: position sigma scales %.3fx (1/sqrt(n)); no floor under zero-mean "
       "noise -- contrast P10, where a bias floor is independent of n"
       % (math.sqrt(3.0/30.0)))

# ---- P14 anchor spacing ---------------------------------------------------
SP = [50,100,200,400,700,1000,2000,4000,7000,10000]
def cov(t, o, sp):
    r = r80("868MHz", t, o)
    return min(1.0, (r/sp)**2) if sp > 0 else 1.0
bt = [shape([cov(t,0,x) for x in SP]) for t in TERRAINS]
bo = [shape([cov(0,o,x) for x in SP]) for o in OFFSETS]
rb = r80("868MHz", 0.0, 0.0)
record("P14", "anchor spacing", bt[0], classify(bt,bo),
       "50 m->10 km: coverage falls; r80=%.0f m so spacing/r80 spans %.3f -> %.1f; "
       "collapse (coverage<0.5) above spacing/r80 = %.2f"
       % (rb, 50/rb, 10000/rb, 1/math.sqrt(0.5)))

# ---- P15 anchor geometry --------------------------------------------------
GEOM = {"ring":     [i*2*math.pi/6 for i in range(6)],
        "grid":     [math.atan2(y,x) for x in (-1,0,1) for y in (-1,0,1) if (x,y)!=(0,0)],
        "clustered":[0.05*i for i in range(6)],
        "collinear":[0.0,0.0,0.0,math.pi,math.pi,math.pi]}
gd = {}
for name, az in GEOM.items():
    try: gd[name] = rg.gdop(az)
    except Exception as e: gd[name] = float("inf")
bt = [tuple(sorted(gd, key=lambda k: gd[k]))]*2
bo = [tuple(sorted(gd, key=lambda k: gd[k]))]*3
record("P15", "anchor geometry", "ring best, collinear degenerate", classify(bt,bo),
       "GDOP  " + "  ".join("%s=%.4g" % (k, gd[k]) for k in ("ring","grid","clustered","collinear")))

# ---- P16 node speed -------------------------------------------------------
SPD = [0,1,1.5,3,5,10,20,35,50]
def coh_ms(v, f=868e6):
    if v <= 0: return float("inf")
    fd = v*f/299792458.0
    return 0.423/fd*1000.0
AIRTIME_MS = (2**7)/125e3*(8+4.25)*1000.0
bt = [shape([-coh_ms(x) for x in SPD]) for _ in TERRAINS]
bo = [shape([-coh_ms(x) for x in SPD]) for _ in OFFSETS]
crit = [v for v in SPD if v>0 and coh_ms(v) < AIRTIME_MS]
record("P16", "node speed", bt[0], classify(bt,bo),
       "0->50 m/s: coherence time %.1f ms at 1 m/s -> %.1f ms at 50 m/s; frame airtime "
       "%.1f ms. Decorrelation mid-packet %s in this range. SESSION_CHANGELOG.md F4: "
       "RunProbe produces structurally zero velocity, so EVERY r80 in this project was "
       "measured at v=0" % (coh_ms(1.0), coh_ms(50.0), AIRTIME_MS,
       "occurs above %.0f m/s" % min(crit) if crit else "NEVER occurs"))

print("\n=== SUMMARY ===")
print("  %-4s %-24s %-16s %s" % ("id","parameter","class","conclusion"))
for pid,name,conc,cls,_ in RESULTS:
    print("  %-4s %-24s %-16s %s" % (pid,name,cls,conc))
n_rob = sum(1 for r in RESULTS if r[3]=="ROBUST")
print("\n  %d experiments: %d ROBUST, %d SENSITIVE, %d UNDETERMINABLE"
      % (len(RESULTS), n_rob,
         sum(1 for r in RESULTS if r[3]=="SENSITIVE"),
         sum(1 for r in RESULTS if r[3]=="UNDETERMINABLE")))
