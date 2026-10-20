#!/usr/bin/env python3
"""IQ-TERM: is the IQ-impairment degradation the uncensused term behind G11's residuals?

Registered in experiments/IQ_TERM_REGISTRATION.md before any probe it reads was run.
Committed with that registration and not edited afterwards.

Reads two probe CSVs written with --probeNominalMargin (the default run, and the run with
--iqOverloadOnly), and the corpus. The model (linkbudget.py) never sees the probe: the
probe's columns are carried only to be compared against what the model computes, exactly
as validate.py does.

    python3 iq_term.py <default probe_range_sweep.csv> <overload-only probe_range_sweep.csv>

Exit 0 always; the verdict per prediction is printed and is the result.
"""
import csv, math, sys

import linkbudget as lb
from validate import load_corpus

# G11a's terrain-inactive population (experiments/G11A_REGISTRATION.md), unchanged.
G11A_POP = {"868MHz": (100.0,), "2.4GHz": (10.0, 25.0, 50.0), "UWB": (2.0, 5.0, 10.0, 20.0)}
MARGIN_TOL_DB = 3.0      # G11's registered margin criterion, unchanged
SHIFT_TOL_DB = 0.5       # Q4, registered with its reason
TIERS = ("868MHz", "2.4GHz", "UWB")


def load(path):
    rows = list(csv.DictReader(open(path)))
    need = ("mean_nominal_margin_db", "mean_iq_degradation_db")
    if not rows or any(k not in rows[0] for k in need):
        raise SystemExit("FATAL: %s lacks %s -- not a --probeNominalMargin run" % (path, need))
    return {(r["tier"], float(r["distance_m"])): r for r in rows}


def main():
    if len(sys.argv) != 3:
        raise SystemExit(__doc__)
    base, ovl = load(sys.argv[1]), load(sys.argv[2])
    draws, digest, n = load_corpus()
    print("corpus %d realizations, digest %s" % (n, digest[:16]))

    verdict = {}

    def mark(q, ok, why):
        verdict[q] = verdict.get(q, True) and ok
        if not ok:
            print("    %s FAILS: %s" % (q, why))

    print("\n%-7s %7s %7s | %8s %8s %8s | %8s %8s %8s" %
          ("tier", "dist", "pdr", "iq_dB", "nom_eng", "mdl", "gap_nom", "gap+iq", "shift"))
    model = {}
    for t in TIERS:
        for (tt, d), r in sorted(base.items(), key=lambda kv: (kv[0][0], kv[0][1])):
            if tt != t:
                continue
            iq = float(r["mean_iq_degradation_db"])
            nom = float(r["mean_nominal_margin_db"])
            mm = sum(lb.margin_db(t, d, u_nl, u_at, u_df, u_df) for (_, u_nl, u_at, u_df) in draws) / len(draws)
            model[(t, d)] = mm
            o = ovl.get((t, d))
            shift = float(o["mean_nominal_margin_db"]) - nom if o else float("nan")
            print("%-7s %7.0f %7s | %8.2f %8.2f %8.2f | %+8.2f %+8.2f %+8.2f" %
                  (t, d, r["pdr"], iq, nom, mm, mm - nom, mm - (nom + iq), shift))

            # Q1: the term is reached on every tier at every point (rule 4: not a control-flow zero)
            mark("Q1", iq > 0.0, "%s %.0f m iq %.3f" % (t, d, iq))
            # Q2: UWB saturates
            if t == "UWB":
                mark("Q2", iq >= (4.0 if d < 5.0 else 5.0), "UWB %.0f m iq %.2f" % (d, iq))
            # Q3: the IQ-imbalance floor at high SNR on the two narrow-band tiers' G11a points
            if t != "UWB" and d in G11A_POP[t]:
                mark("Q3", 0.4 <= iq <= 0.8, "%s %.0f m iq %.2f" % (t, d, iq))
            # Q4: the flag removes exactly this term (no clipping at probe distances)
            mark("Q4", o is not None and abs(shift - iq) <= SHIFT_TOL_DB,
                 "%s %.0f m shift %.2f vs iq %.2f" % (t, d, shift, iq))
            # Q5: on G11a's population, with the IQ term added back to the engine's nominal margin
            if d in G11A_POP[t]:
                g = mm - (nom + iq)
                if t == "2.4GHz":
                    mark("Q5a", abs(g) <= MARGIN_TOL_DB, "2.4 GHz %.0f m gap+iq %+.2f" % (d, g))
                if t == "868MHz":
                    mark("Q5b", g < -MARGIN_TOL_DB, "868 %.0f m gap+iq %+.2f (predicted < -3.0)" % (d, g))
                if t == "UWB" and d == 2.0:
                    mark("Q5c", g < -MARGIN_TOL_DB, "UWB 2 m gap+iq %+.2f (predicted < -3.0)" % g)
            # Q6: UWB beyond the bound, the saturated term over-explains the plateau
            if t == "UWB" and d >= 30.0:
                g = mm - (nom + iq)
                mark("Q6", -4.0 <= g <= -1.5, "UWB %.0f m gap+iq %+.2f (predicted in [-4.0, -1.5])" % (d, g))

    print()
    for q in ("Q1", "Q2", "Q3", "Q4", "Q5a", "Q5b", "Q5c", "Q6"):
        print("  %-4s %s" % (q, "HOLDS" if verdict.get(q, False) else "FAILS"))
    g11a = all(abs(model[(t, d)] - (float(base[(t, d)]["mean_nominal_margin_db"]) +
                                    float(base[(t, d)]["mean_iq_degradation_db"]))) <= MARGIN_TOL_DB
               for t in TIERS for d in G11A_POP[t])
    print("  G11a on the decode-independent margin with the IQ term accounted: %s" % ("PASS" if g11a else "FAIL"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
