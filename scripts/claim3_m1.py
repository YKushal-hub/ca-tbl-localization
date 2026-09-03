#!/usr/bin/env python3
"""Claim 3's M1 ratio under the b49df46 §4 rule, applied verbatim.

    PASS          ca_tbl_full meets (A, R) AND its M1 is lower than every single-band
                  arm that also meets (A, R), with the geometric mean of per-seed
                  ratios < 1.0 and the seed-level bootstrap 95% CI entirely below 1.0.
    FAIL          some single-band arm that also meets (A, R) achieves lower M1, CI
                  entirely below 1.0 in that direction.
    INCONCLUSIVE  the CI spans 1.0, or no arm other than ca_tbl_full meets (A, R).

"No competitor meets the requirement" is deliberately INCONCLUSIVE and not PASS.

A = 0.95 availability at R = 25 m. M1 is total energy, totalJ. The ratio is
ca_tbl_full / competitor, per seed, geometric mean, seed-level bootstrap.
"""
import csv, glob, math, os, random, sys

A_REQ, R_REQ = 0.95, 25.0
SINGLE_BAND = ("static_868", "static_2p4", "static_uwb")


def avail(d, arm, seed):
    p = os.path.join(d, "trace_%s_seed%d.csv" % (arm, seed))
    if not os.path.isfile(p):
        return None
    n = ok = 0
    for r in csv.DictReader(open(p, newline="")):
        n += 1
        if r["hadFix"] in ("1", "true", "True") and float(r["errM"]) <= R_REQ:
            ok += 1
    return ok / n if n else None


def load(root):
    cells = {}
    for sd in sorted(glob.glob(os.path.join(root, "seed_*"))):
        s = int(os.path.basename(sd).split("_")[1])
        f = os.path.join(sd, "arm_summary.csv")
        if not os.path.isfile(f):
            continue
        for r in csv.DictReader(open(f, newline="")):
            cells[(r["arm"], s)] = dict(J=float(r["totalJ"]), dir=sd)
    return cells


def boot(ratios, n=20000, seed=12345):
    rng = random.Random(seed)
    gm = lambda v: math.exp(sum(math.log(x) for x in v) / len(v))
    out = sorted(gm([rng.choice(ratios) for _ in ratios]) for _ in range(n))
    return out[int(0.025 * n)], out[int(0.975 * n)], gm(ratios)


def main():
    root = sys.argv[1] if len(sys.argv) > 1 else "results/p10_grid/c_ef0_legacy_dw800_b1500"
    cells = load(root)
    seeds = sorted({s for (_, s) in cells})
    if not seeds:
        print("POPULATION EMPTY at %s -- VACUOUS, not a pass" % root)
        return 1
    print("cell: %s" % root)
    print("seeds: %d\n" % len(seeds))

    meets = {}
    for arm in ("ca_tbl_full",) + SINGLE_BAND:
        n_ok = 0
        for s in seeds:
            d = cells.get((arm, s))
            if not d:
                continue
            a = avail(d["dir"], arm, s)
            if a is not None and a >= A_REQ:
                n_ok += 1
        meets[arm] = n_ok
        print("  %-18s meets (A=%.2f, R=%.0f) on %2d/%d seeds" % (arm, A_REQ, R_REQ, n_ok, len(seeds)))

    if meets["ca_tbl_full"] < len(seeds):
        print("\nca_tbl_full does not meet the requirement on every seed -> not adjudicable as PASS")
    competitors = [a for a in SINGLE_BAND if meets[a] == len(seeds)]
    print("\n  single-band arms meeting it on all seeds: %s"
          % (", ".join(competitors) if competitors else "NONE"))
    if not competitors:
        print("\n=== INCONCLUSIVE -- no competitor meets the requirement, nothing to compare against")
        print("    (deliberately not PASS, per the rule)")
        return 0

    verdicts = []
    for comp in competitors:
        ratios = []
        for s in seeds:
            a, b = cells.get(("ca_tbl_full", s)), cells.get((comp, s))
            if a and b and b["J"] > 0:
                ratios.append(a["J"] / b["J"])
        if len(ratios) < 2:
            continue
        lo, hi, g = boot(ratios)
        v = ("BELOW" if hi < 1.0 else ("ABOVE" if lo > 1.0 else "SPANS"))
        verdicts.append((comp, g, lo, hi, v))
        print("\n  M1 ratio ca_tbl_full / %s" % comp)
        print("    geometric mean %.4f   seed-level bootstrap 95%% CI [%.4f, %.4f]  -> %s 1.0"
              % (g, lo, hi, v))
        print("    mean J: ca_tbl_full %.3f, %s %.3f"
              % (sum(cells[("ca_tbl_full", s)]["J"] for s in seeds) / len(seeds), comp,
                 sum(cells[(comp, s)]["J"] for s in seeds) / len(seeds)))

    print("\n=== VERDICT under b49df46 §4")
    if any(v == "SPANS" for *_, v in verdicts):
        print("    INCONCLUSIVE -- a CI spans 1.0")
    elif all(v == "BELOW" for *_, v in verdicts):
        print("    PASS -- lower than every qualifying single-band arm, CI entirely below 1.0")
    else:
        print("    FAIL -- a qualifying single-band arm achieves lower M1")
    return 0


if __name__ == "__main__":
    sys.exit(main())
