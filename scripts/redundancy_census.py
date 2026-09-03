#!/usr/bin/env python3
"""Is tier-2 exclusion geometrically unavailable, as the redundancy account claims?

THE CLAIM UNDER TEST. Tier 2 deploys three anchors per facility site, and a 2-D range
fix needs three. So a tag in range of one site has a fix and a residual (detection) but
nothing left to leave out (exclusion), and exclusion needs four, identification five.
If tags are usually in range of one site, exclusion is unavailable by construction and
the integrity mechanism was placed in a geometry that cannot support it.

That is a claim about how many anchors a tag can actually reach, and this repository
already contains 57 600 cycles of the answer. It is measured here rather than swept.

THREE INSTRUMENTS, AND TWO OF THEM ARE WRONG FOR THIS QUESTION.

  nAnchors   the physics outcome -- anchors that delivered a range. Mean 6.07. But it
             DIFFERS BETWEEN POLICIES on 6.8% of cycles, because the policy changes the
             estimate, which changes escalation, which changes what is counted.
             Conditioning on it is conditioning on a collider.

  vis2p4     the scenario's geometric label. Exactly policy-invariant (0 of 57 600
             differ), which is what it was built for. But it counts anchors within
             `nominalRangeM` = 900 m, and the source states that 900 m is a CENSORED
             LOWER BOUND (`ca_tbl_causal_sim.cc:217-218`) -- the sweep ended there with
             PDR still 0.804. So the label caps at 3, reporting one site, for a range
             the tier comfortably exceeds. It measures the label's conservatism.

  this file  geometric count recomputed from trueX/trueY -- which is EXACTLY
             policy-invariant, 0 of 57 600 differ -- against the effective range, with
             the anchor positions reconstructed from BuildDeployment's own formula.

The third is validated against the second-best evidence available: at the analytic
model's tier-2 r80 of 1580.6 m it predicts a mean of 6.03 anchors in range, against a
measured `nAnchors` mean of 6.07. Agreement to 0.7% on an observable the model was not
fitted to.
"""
import csv, math, collections, glob, os, sys

ROOT = "results/h1_power"
R_OK = 25.0
BOX_HALF = 2000.0
T2_RING_M = 150.0
T2_PER_SITE = 3
NODE_H = 1.5
ANCHOR_H = 6.0
EFFECTIVE_RANGE_M = 1580.6   # linkbudget.py r80 for 2.4 GHz; the 900 m label is censored


def tier2_anchors(per_site=T2_PER_SITE, ring=T2_RING_M, box_half=BOX_HALF):
    f = box_half * 0.47
    out = []
    for (cx, cy) in ((-f, -f), (f, -f), (f, f), (-f, f)):
        for k in range(per_site):
            th = 2 * math.pi * k / per_site + 0.3
            out.append((cx + ring * math.cos(th), cy + ring * math.sin(th), ANCHOR_H))
    return out


def visible(anchors, x, y, rng):
    return sum(1 for a in anchors if math.dist((x, y, NODE_H), a) <= rng)


def paired(d, seeds, min_cycles=30, min_seeds=8):
    ds = []
    for s in seeds:
        a, b = d[("none", s)], d[("exclude", s)]
        if a[0] < min_cycles or b[0] < min_cycles:
            continue
        ds.append(b[1] / b[0] - a[1] / a[0])
    if len(ds) < min_seeds:
        return None
    n = len(ds); m = sum(ds) / n
    var = sum((x - m) ** 2 for x in ds) / (n - 1)
    return m, 1.959963984540054 * math.sqrt(var / n), n


def main():
    seeds = [s for s in range(1, 41)
             if os.path.isfile("%s/h1_none_seed%d/trace_ca_tbl_full_seed%d.csv" % (ROOT, s, s))]
    if not seeds:
        print("no h1_power traces found under %s" % ROOT); return 1
    anchors = tier2_anchors()
    print("POPULATION: %d seeds, tier-2-active cycles only, %d reconstructed tier-2 anchors"
          % (len(seeds), len(anchors)))

    dist = collections.Counter()
    strata = collections.defaultdict(lambda: collections.defaultdict(lambda: [0, 0]))
    nanchors_sum = nanchors_n = 0
    for s in seeds:
        base = list(csv.DictReader(open(
            "%s/h1_none_seed%d/trace_ca_tbl_full_seed%d.csv" % (ROOT, s, s), newline="")))
        geo = [visible(anchors, float(r["trueX"]), float(r["trueY"]), EFFECTIVE_RANGE_M)
               for r in base]
        for r, g in zip(base, geo):
            if r["activeTier"] != "2.4GHz":
                continue
            dist[g] += 1
            try:
                nanchors_sum += int(r["nAnchors"]); nanchors_n += 1
            except ValueError:
                pass
        for pol in ("none", "exclude"):
            rows = list(csv.DictReader(open(
                "%s/h1_%s_seed%d/trace_ca_tbl_full_seed%d.csv" % (ROOT, pol, s, s), newline="")))
            for r, g in zip(rows, geo):
                if r["activeTier"] != "2.4GHz":
                    continue
                ok = r["hadFix"] in ("1", "true", "True") and float(r["errM"]) <= R_OK
                key = ("<=2 no fix" if g <= 2 else "3 detect only" if g == 3
                       else "4 exclusion" if g == 4 else "5+ identification")
                c = strata[key][(pol, s)]; c[0] += 1; c[1] += 1 if ok else 0

    tot = sum(dist.values())
    mean_geo = sum(k * v for k, v in dist.items()) / tot
    print("\n--- CROSS-VALIDATION of the reconstructed geometry")
    print("  geometric mean anchors in range at %.1f m : %.2f" % (EFFECTIVE_RANGE_M, mean_geo))
    print("  measured  mean nAnchors (physics outcome) : %.2f"
          % (nanchors_sum / nanchors_n if nanchors_n else float("nan")))

    print("\n--- IS EXCLUSION AVAILABLE?  tier-2-active cycles, n = %d" % tot)
    g = lambda p: 100.0 * sum(v for k, v in dist.items() if p(k)) / tot
    print("  <= 2  no fix possible                : %6.2f%%" % g(lambda k: k <= 2))
    print("     3  fix + residual, DETECT only    : %6.2f%%" % g(lambda k: k == 3))
    print("  >= 4  EXCLUSION possible             : %6.2f%%" % g(lambda k: k >= 4))
    print("  >= 5  IDENTIFICATION possible        : %6.2f%%" % g(lambda k: k >= 5))

    print("\n--- exclusion advantage within each stratum, paired by seed")
    print("  REPORTED, NOT A VERDICT. See the caveat below.")
    print("  %-22s %10s %24s %7s" % ("redundancy", "advantage", "95% CI", "seeds"))
    for key in ("3 detect only", "4 exclusion", "5+ identification"):
        r = paired(strata[key], seeds)
        if r is None:
            print("  %-22s  (too few seeds clear the cycle minimum)" % key); continue
        m, hw, n = r
        print("  %-22s %+10.4f  [%+.4f, %+.4f] %7d%s"
              % (key, m, m - hw, m + hw, n, "  excludes zero" if (m - hw) * (m + hw) > 0 else ""))

    print("""
  CAVEAT, which is why the stratified column is not a verdict. The EKF carries state, so
  a cycle's outcome depends on every prior cycle's branch, not only on this cycle's
  redundancy. Stratifying by current-cycle geometry cannot attribute an availability
  difference to the current cycle's branch: under `exclude` the filter arrives at a given
  cycle having taken different decisions earlier. A per-cycle stratification of a
  path-dependent estimator answers a question about accumulated state, not about the
  branch. The availability fractions above do not have this problem -- they are pure
  geometry.""")
    return 0


if __name__ == "__main__":
    sys.exit(main())
