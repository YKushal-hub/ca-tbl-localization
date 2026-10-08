"""UWB reach decomposition with the repo's independent link-budget model
(src/catbl-propagation-study/linkbudget.py), one term at a time.
Reads nothing from the engine; uses the study's own frozen corpus builder."""
import sys, math, hashlib, json
import os
sys.path.insert(0, os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "catbl-propagation-study"))
import corpus, linkbudget as lb

c = corpus.build()
draws = [(r["anchors"][0]["u_shadow"], r["anchors"][0]["u_nlos"],
          r["anchors"][0]["u_excess"], r["anchors"][0]["u_fade"]) for r in c["realizations"]]
print("corpus n=%d sha256(json)=%s" % (c["n"], hashlib.sha256(json.dumps(c).encode()).hexdigest()[:16]))
D = [1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 17, 20, 25, 30, 35, 40, 50, 60, 70, 80,
     90, 100, 120, 140, 160, 180, 200, 250, 300, 400]

def r(over, q=0.80):
    if q == 0.80:
        return lb.r80("UWB", draws, D, over)
    prev = None
    for d in D:
        p = lb.pdr_at("UWB", d, draws, over)
        if prev and prev[1] >= q > p:
            return prev[0] + (prev[1] - q) / (prev[1] - p) * (d - prev[0])
        prev = (d, p)
    return float("nan")

NOVEG = {"veg": {"amax": 0.0}}
rows = [
    ("as simulated (TX -16.47 dBm, sens -104 dBm, 0 dBi, sparse shrubs with the clamped x5.28, shadow 5 dB, Rician)", {}),
    ("datasheet radio: TX -14.3 dBm (-41.3 dBm/MHz x 499.2 MHz), sens -99.3 dBm (DW3000 ch9 850k/1024)",
     {"phy": {"tx": -14.3}, "sensitivity": -99.3}),
    ("  ... and no foliage", {"phy": {"tx": -14.3}, "sensitivity": -99.3, **NOVEG}),
    ("  ... and no foliage, no shadowing", {"phy": {"tx": -14.3}, "sensitivity": -99.3, **NOVEG, "shadow_sd": 0.0}),
    ("  ... and no foliage, no shadowing, no fading",
     {"phy": {"tx": -14.3}, "sensitivity": -99.3, **NOVEG, "shadow_sd": 0.0, "no_fading": True}),
    ("simulated radio, no foliage", {**NOVEG}),
    ("simulated radio, no foliage, no shadowing, no fading", {**NOVEG, "shadow_sd": 0.0, "no_fading": True}),
]
for name, o in rows:
    print("%-100s r80 %6.1f m   r50 %6.1f m" % (name, r(o), r(o, 0.5)))
# Free-space reference: range where FSPL equals the datasheet budget, 0 dBi.
for ch, f, bud in (("ch9", 7.9872e9, 85.0), ("ch5", 6.4896e9, 86.0)):
    fs1 = lb.fspl_at_reference(f, 1.0)
    print("free space, %s, %0.0f dB budget, 0 dBi: %.1f m; with +3 dBi each end: %.1f m"
          % (ch, bud, 10 ** ((bud - fs1) / 20), 10 ** ((bud + 6 - fs1) / 20)))
print("foliage at the corpus median depth (sparse shrubs, engine scaling): %.2f dB" % lb.foliage_loss_db("UWB", 0.0))
# linkbudget.py carries the engine's per-tier foliage scaling since 2026-10-09: the mean of gamma
# x5.28 for UWB (lora-uwb-physics-parameters.cc:116), with the bounds unscaled and the draw
# clamped (lora-distribution-sampler.cc:76), so E[gamma] is 1.80x, not 5.28x. Without it, and
# with the x5.28 taken at face value (unclamped, which the engine does NOT do):
U = {"veg_scale": 1.0}
F = {"veg": {"gamma": 0.04 * 5.28}, "veg_scale": 1.0}
print("without the engine's foliage scaling: r80 %.1f m, r50 %.1f m, foliage at median depth %.2f dB"
      % (r(U), r(U, 0.5), lb.foliage_loss_db("UWB", 0.0, U)))
print("x5.28 unclamped (NOT the engine): r80 %.1f m, r50 %.1f m, foliage at median depth %.2f dB"
      % (r(F), r(F, 0.5), lb.foliage_loss_db("UWB", 0.0, F)))
