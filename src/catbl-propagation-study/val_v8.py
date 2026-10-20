#!/usr/bin/env python3
"""VAL-V8: G11's reconciliation, unchanged criterion, against the engine as it is (experiments/VAL_V8_REGISTRATION.md).

Committed with the registration, before the model is evaluated against results/probe_v8; not edited afterwards.
The probe is read for COMPARISON ONLY, exactly as validate.py does; linkbudget.py never sees it.

    python3 val_v8.py [<probe_v8 csv>]

Tiers 2/3 are evaluated in the configuration 50fc8620 made the engine's default: the wide-area environment
(ENV_RURAL_FORESTED) and vegetation (VEG_DENSE_FOREST). Every value is the engine's declared default.
"""
import os
import sys

import linkbudget as lb
import validate as v

HERE = os.path.dirname(os.path.abspath(__file__))
PROBE_V8 = os.path.join(HERE, "..", "..", "results", "probe_v8", "probe_range_sweep.csv")
PROBE_V7 = v.PROBE
OVR = {"868MHz": {},
       "2.4GHz": {"env": "RURAL_FORESTED", "veg": dict(lb.VEGETATION["VEG_DENSE_FOREST"])},
       "UWB": {"env": "RURAL_FORESTED", "veg": dict(lb.VEGETATION["VEG_DENSE_FOREST"])}}
G11A_POP = {"868MHz": (100.0,), "2.4GHz": (10.0, 25.0, 50.0), "UWB": (2.0, 5.0, 10.0, 20.0)}
EXPECT_TOL_DB = 1.5


def main():
    v.PROBE = sys.argv[1] if len(sys.argv) > 1 else PROBE_V8
    probe8 = v.load_probe_for_comparison()
    v.PROBE = PROBE_V7
    probe7 = v.load_probe_for_comparison()
    draws, digest, n = v.load_corpus()
    print("corpus %d realizations, digest %s; probe %s" % (n, digest[:16], os.path.relpath(sys.argv[1] if len(sys.argv) > 1 else PROBE_V8)))
    allpass, expect_ok = True, True
    for t in ("868MHz", "2.4GHz", "UWB"):
        o = OVR[t]
        pts = probe8[t]
        p7 = {d: m for (d, _, m) in probe7[t]}
        dists = [p[0] for p in pts]
        print("\n  --- %s   overrides %s" % (t, o or "none"))
        print("    %-8s %9s %9s | %11s %11s %9s | %9s %9s" %
              ("dist", "pdr_eng", "pdr_mdl", "margin_eng", "margin_mdl", "gap_dB", "gap_v7", "moved"))
        worst = 0.0
        mdl7 = {}
        for (d, pe, m_probe) in pts:
            pm = lb.pdr_at(t, d, draws, o)
            mm = sum(lb.margin_db(t, d, u_nl, u_at, u_df, u_df, o) for (_, u_nl, u_at, u_df) in draws) / len(draws)
            gap = mm - m_probe
            worst = max(worst, abs(gap))
            g7 = ""
            mv = ""
            if d in G11A_POP[t] and d in p7:
                m7 = sum(lb.margin_db(t, d, u_nl, u_at, u_df, u_df) for (_, u_nl, u_at, u_df) in draws) / len(draws)
                gap7 = m7 - p7[d]
                g7 = "%+9.2f" % gap7
                ok = abs(gap - gap7) <= EXPECT_TOL_DB
                if t != "868MHz":
                    expect_ok = expect_ok and ok
                mv = "%+9.2f%s" % (gap - gap7, "" if ok else " *")
            print("    %-8.0f %9.4f %9.4f | %11.2f %11.2f %+9.2f | %9s %s" % (d, pe, pm, m_probe, mm, gap, g7, mv))
        rm = lb.r80(t, draws, dists, o)
        re_, prev = None, None
        for (d, pe, _) in pts:
            if prev is not None and prev[1] >= 0.80 > pe:
                re_ = prev[0] + (prev[1] - 0.80) / (prev[1] - pe) * (d - prev[0])
                break
            prev = (d, pe)
        if re_ is None and pts and pts[0][1] < 0.80:
            re_ = 0.0
        rel = abs(rm - re_) / re_ if (re_ and rm == rm) else float("nan")
        ok_r = rel == rel and rel <= v.R80_TOL
        ok_p = worst <= v.MARGIN_TOL_DB
        allpass = allpass and ok_r and ok_p
        print("    r80  model %8.1f   engine %8.1f   rel %6.1f%%  %s" % (rm, re_ if re_ is not None else float("nan"),
                                                                       100 * rel, "PASS" if ok_r else "FAIL"))
        print("    margin worst deviation %.2f dB  %s" % (worst, "PASS" if ok_p else "FAIL"))
    print("\n  G11 criterion on today's engine: %s" % ("PASS" if allpass else "FAIL"))
    print("  registered expectation (2.4 GHz and UWB G11a gaps within %.1f dB of probe_v7's): %s"
          % (EXPECT_TOL_DB, "HOLDS" if expect_ok else "FAILS"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
