#!/usr/bin/env python3
"""S6 (master plan v2): does the engine's packet delivery match the analytic outage model?

Goldsmith (Wireless Communications, eq. 2.41): P(received) = Phi(M / sigma), M the mean margin over
sensitivity and sigma the shadowing SD. The probe reports, per tier and distance, pdr, mean_margin_db and
shadow_sd_eff_db (the engine's own effective shadowing). Pass band (master plan v2 S6): |pdr - Phi(M/s)|
<= 0.03 at every row with n >= 200 and 0.02 <= pdr <= 0.98, per tier.
usage: s6_outage_check.py <probe_range_sweep.csv> [...]
"""
import csv, math, sys


def phi(x):
    return 0.5 * math.erfc(-x / math.sqrt(2))


def main():
    worst = {}
    lines = ["| file | tier | d (m) | pdr | Phi(M/s) | diff |", "|---|---|---|---|---|---|"]
    for f in sys.argv[1:]:
        for r in csv.DictReader(open(f)):
            n, p = int(r["n"]), float(r["pdr"])
            if n < 200 or not (0.02 <= p <= 0.98):
                continue
            s = float(r["shadow_sd_eff_db"])
            if s <= 0:
                continue
            q = phi(float(r["mean_margin_db"]) / s)
            dlt = p - q
            lines.append(f"| {f.split('/')[-2]} | {r['tier']} | {r['distance_m']} | {p:.3f} | {q:.3f} | {dlt:+.3f} |")
            worst[r["tier"]] = max(worst.get(r["tier"], 0.0), abs(dlt))
    print("\n".join(lines))
    print()
    for t, w in sorted(worst.items()):
        print(f"S6 {t}: max |pdr - Phi(M/s)| = {w:.3f} -> {'PASS' if w <= 0.03 else 'FAIL'}")


if __name__ == "__main__":
    main()
