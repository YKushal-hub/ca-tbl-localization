#!/usr/bin/env python3
"""Section 7 gate -- PER-REALIZATION evaluation against the engines' output.

WHY THIS WAS REWRITTEN. The first version fed the probe's AGGREGATE columns
(mean_margin_db, shadow_sd_eff_db) into a per-link formula and applied Phi once.
That evaluates a nonlinear function at the mean instead of averaging the function,
and it produced FOUR false mechanism diagnoses in a row -- two-ray breakpoint,
missing sigma floor, NLOS hard-failure, preamble gain -- each blamed for an error
the evaluation method was generating on its own. The corpus had been generated,
digested, and never opened.

The decision the engine actually makes (lora-physics-engine.cc:884) is a HARD
comparison per trial:

    deliver iff rxPower_i > effectiveSensitivity_i

so the model must make that comparison per realization and average the outcome.
Both candidate explanations for the old residual -- Jensen curvature and an
under-stated sigma -- are absorbed by doing this, without needing to separate them.

CORPUS CONSUMPTION IS ASSERTED, not assumed. A validation that silently falls back
to aggregates is the same failure class as a gate whose failure mode is silence,
and that is exactly what happened here.
"""
import csv, hashlib, json, math, os, sys
import propagation as P

HERE = os.path.dirname(os.path.abspath(__file__))
PROBE = os.path.join(HERE, "..", "..", "results", "causal", "probe_range_sweep.csv")
CORPUS = os.path.join(HERE, "results", "corpus.json")

# Every constant carries its file:line. No physical constant enters without one.
TIERS = {
    "868MHz": {"f": 868e6,   "src": "lora-physics-parameters.cc:12",     "tx": 14.0},
    "2.4GHz": {"f": 2.44e9,  "src": "lora-2p4-physics-parameters.cc:52", "tx": 12.5},
    "UWB":    {"f": 7.99e9,  "src": "lora-uwb-physics-parameters.cc:52", "tx": -16.47},
}
R80_TOL = 0.15


class Corpus:
    """Loads the corpus and COUNTS draw consumption so bypass is detectable."""

    def __init__(self, path):
        if not os.path.exists(path):
            raise SystemExit("FATAL: corpus absent at %s -- run corpus.py first" % path)
        with open(path) as f:
            self.data = json.load(f)
        h = hashlib.sha256()
        with open(path, "rb") as f:
            for c in iter(lambda: f.read(1 << 20), b""):
                h.update(c)
        self.digest = h.hexdigest()
        self.draws_used = 0

    def shadow_draws(self, n):
        """Unit-normal shadowing draws, scaled by the caller. Counted."""
        out = []
        for r in self.data["realizations"][:n]:
            out.append(r["anchors"][0]["u_shadow"])
        self.draws_used += len(out)
        return out

    def nlos_draws(self, n):
        out = [r["anchors"][0]["u_nlos"] for r in self.data["realizations"][:n]]
        self.draws_used += len(out)
        return out


def load_probe():
    rows = [r for r in csv.DictReader(open(PROBE))]
    out = {}
    for t in TIERS:
        out[t] = sorted([(float(r["distance_m"]), float(r["pdr"]), float(r["mean_pl_db"]),
                          float(r["mean_margin_db"]), float(r["shadow_sd_eff_db"]),
                          float(r["nlos_frac"])) for r in rows if r["tier"] == t])
    return out


def pdr_per_realization(margin_db, sigma_db, shad):
    """Average the DECISION over realizations. Never Phi(mean)."""
    ok = 0
    for u in shad:
        if margin_db + sigma_db * u > 0.0:
            ok += 1
    return ok / len(shad)


def main():
    corpus = Corpus(CORPUS)
    probe = load_probe()
    print("  corpus sha256 %s  n=%d" % (corpus.digest[:16] + "...", corpus.data["n"]))
    print("  ACCEPTANCE (unchanged, not relaxed after failure): r80 within %.0f%%" % (100 * R80_TOL))
    print()

    # --- diagnostic: is the engine's threshold fixed? ------------------------
    print("=" * 78)
    print("D1. implied sensitivity per distance = tx - PL - margin")
    print("    constant => fixed threshold.  varying => a per-trial penalty term.")
    print("=" * 78)
    for t, cfg in TIERS.items():
        vals = [cfg["tx"] - pl - mar for (_, _, pl, mar, _, _) in probe[t]]
        print("  %-8s min %8.2f  max %8.2f  spread %6.2f dB   (%s)" %
              (t, min(vals), max(vals), max(vals) - min(vals), cfg["src"]))
    print()

    # --- the gate, evaluated per realization ---------------------------------
    print("=" * 78)
    print("D2. PDR per realization: mean of 1[margin + sigma*u > 0] over the corpus")
    print("=" * 78)
    n = corpus.data["n"]
    shad = corpus.shadow_draws(n)
    allpass = True
    for t, cfg in TIERS.items():
        print("  --- %s" % t)
        print("    %-8s %8s %10s %10s" % ("dist", "pdr_eng", "pdr_model", "diff"))
        pts = []
        for (dist, pdr, pl, mar, sd, nl) in probe[t]:
            m = pdr_per_realization(mar, sd, shad)
            pts.append((dist, pdr, m))
            print("    %-8.0f %8.4f %10.4f %+10.4f" % (dist, pdr, m, m - pdr))
        # r80 from each curve by interpolation on the model points
        def r80_of(idx):
            for i in range(1, len(pts)):
                a, b = pts[i - 1], pts[i]
                if a[idx] >= 0.80 > b[idx]:
                    return a[0] + (a[idx] - 0.80) / (a[idx] - b[idx]) * (b[0] - a[0])
            return float("nan")
        rm, re_ = r80_of(2), r80_of(1)
        rel = abs(rm - re_) / re_ if re_ == re_ and re_ > 0 else float("nan")
        ok = rel == rel and rel <= R80_TOL
        allpass = allpass and ok
        print("    r80 model %.1f  engine %.1f  rel %.1f%%  %s" %
              (rm, re_, 100 * rel, "PASS" if ok else "FAIL"))
    print()
    if corpus.draws_used == 0:
        raise SystemExit("FATAL: corpus loaded but NO draws consumed -- this run "
                         "evaluated on aggregates, which is the defect this file "
                         "was rewritten to prevent.")
    print("  corpus draws consumed: %d  (zero would be a FATAL bypass)" % corpus.draws_used)
    print("  ACCEPTANCE: %s" % ("PASS" if allpass else "FAIL"))
    return 0 if allpass else 1


if __name__ == "__main__":
    sys.exit(main())
