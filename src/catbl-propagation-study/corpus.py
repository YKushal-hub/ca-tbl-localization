#!/usr/bin/env python3
"""Frozen realization corpus. Generated once, replayed by every experiment.

WHY A FROZEN CORPUS. The ns-3 scenario's arms consume the RNG stream at
arm-dependent rates, so a cross-arm difference has always carried a channel-draw
component (Simulation.md section 3.4 / ca_tbl_causal_sim.cc:1765). Generating the
realizations up front and replaying them removes that entirely: every parameter
level sees byte-identical channel conditions, so a difference between levels is
attributable to the parameter and to nothing else.

DRAWS ARE STORED UNIT-NORMALISED AND SCALED AT USE TIME. A shadowing draw is a
unit normal; sigma_shadow multiplies it when the experiment runs. That is what
lets ONE corpus serve a sigma_shadow sweep as a PAIRED comparison. Drawing
independently at each level would put sampling noise on the very axis being
measured -- the same defect as comparing two campaigns instead of two arms.

The corpus is seeded, reproducible, and digested. Every experiment records the
digest; two experiments with different digests are NOT comparable and the
analysis must refuse to pool them rather than doing it silently.
"""
import hashlib, json, math, os, random, sys

CORPUS_VERSION = 1
DEFAULT_N = 2000
DEFAULT_SEED = 20260819


def build(n=DEFAULT_N, seed=DEFAULT_SEED, max_anchors=30, area_half_m=10000.0):
    """One realization = a node, its anchors, and unit-normalised channel draws.

    Geometry spans the full range the sweeps will explore (P14 goes to 10 km), not
    just the baseline -- a corpus drawn only at baseline cannot serve a spacing
    sweep without extrapolating past its own support.
    """
    rng = random.Random(seed)
    reals = []
    for i in range(n):
        # node at origin; anchors on a ring of radius drawn per realization, with
        # azimuth jitter so geometry is not degenerate-symmetric.
        ring_r = math.exp(rng.uniform(math.log(5.0), math.log(area_half_m)))
        anchors = []
        for k in range(max_anchors):
            th = 2.0 * math.pi * k / max_anchors + rng.uniform(-0.15, 0.15)
            rr = ring_r * math.exp(rng.gauss(0.0, 0.25))
            anchors.append({
                "d": rr,
                "az": th,
                # unit draws -- scaled at use time by the experiment's parameters
                "u_shadow": rng.gauss(0.0, 1.0),
                "u_fade": rng.random(),
                "u_nlos": rng.random(),
                "u_excess": rng.expovariate(1.0),
            })
        reals.append({"id": i, "ring_r": ring_r, "anchors": anchors})
    return {"version": CORPUS_VERSION, "n": n, "seed": seed,
            "max_anchors": max_anchors, "realizations": reals}


def digest_of(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(1 << 20), b""):
            h.update(chunk)
    return h.hexdigest()


def main():
    out = os.path.join(os.path.dirname(os.path.abspath(__file__)), "results", "corpus.json")
    n = int(sys.argv[1]) if len(sys.argv) > 1 else DEFAULT_N
    c = build(n=n)
    with open(out, "w") as f:
        json.dump(c, f)
    d = digest_of(out)
    with open(out + ".sha256", "w") as f:
        f.write(d + "\n")
    print("  corpus: n=%d seed=%d max_anchors=%d" % (c["n"], c["seed"], c["max_anchors"]))
    print("  path:   %s (%.1f MB)" % (out, os.path.getsize(out) / 1e6))
    print("  sha256: %s" % d)
    return 0


if __name__ == "__main__":
    sys.exit(main())
