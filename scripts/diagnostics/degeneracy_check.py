#!/usr/bin/env python3
"""Per-MECHANISM degeneracy screen.

    degeneracy_check.py <result-dir> [mechanism]

"Degenerate" is only meaningful relative to the mechanism a campaign is trying to
measure. The previous version returned ONE verdict per run, derived mostly from tier
occupancy, and that is a proxy for "this arm never exercises its mechanism" -- for
the TIERING mechanism only.

It was wrong twice over, in the same call:

  * it failed a Claim 9 pre-flight in which the integrity gate fired 86 times, the
    arms differed in 37 fields and escalation occurred. Tier occupancy is not what
    Claim 9 is about.
  * it flipped verdict on 1.4 percentage points of that incidental quantity:
    campaign_recovery's own cells sit at 97.8% on one tier and passed; the new
    pre-flight sat at 99.2% and failed. Same configuration, same flags.

So: a verdict per mechanism, the underlying quantity printed continuously beside
every verdict, and no single verdict for a run unless a mechanism is named.

Mechanisms:
  tiering    tier occupancy, escalations, demotions
  integrity  suspect count, arm divergence, gate eligibility
  trigger    trigger population, tp + fp
  energy     per-tier energy draw against per-tier occupancy

Exit: 0 if the named mechanism is LIVE, 1 if DEGENERATE, 2 if no mechanism was named
(all four are reported and no single verdict is returned).
"""
import csv, os, sys

HYBRID = {"ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold"}
STATIC = {"static_868": "cycles_868", "static_2p4": "cycles_2p4", "static_uwb": "cycles_uwb"}
MECHS = ("tiering", "integrity", "trigger", "energy")


def rows(d):
    f = os.path.join(d, "arm_summary.csv")
    if not os.path.exists(f) or os.path.getsize(f) == 0:
        return None
    return {r["arm"]: r for r in csv.DictReader(open(f))}


def g(r, k):
    try:
        return float(r.get(k, 0) or 0)
    except (TypeError, ValueError):
        return 0.0


def tiering(R):
    out = []
    for arm, r in R.items():
        if arm not in HYBRID:
            if arm in STATIC:
                other = sum(g(r, k) for k in ("cycles_868", "cycles_2p4", "cycles_uwb")
                            if k != STATIC[arm])
                out.append((arm, "BROKEN" if other else "LIVE",
                            "%d cycles on tiers it does not own" % other))
            continue
        c = {"868": g(r, "cycles_868"), "2p4": g(r, "cycles_2p4"), "uwb": g(r, "cycles_uwb")}
        tot = sum(c.values()) or 1
        top = max(c, key=c.get)
        frac = c[top] / tot
        esc = g(r, "escalations")
        why = "%.1f%% on %s, %d escalations" % (100 * frac, top, esc)
        out.append((arm, "DEGENERATE" if (frac > 0.99 and esc == 0) else "LIVE", why))
    return out


def integrity(R):
    out = []
    f, n = R.get("ca_tbl_full"), R.get("ca_tbl_no_integrity")
    if not f:
        return [("ca_tbl_full", "ABSENT", "no ca_tbl_full arm in this run")]
    sus = g(f, "suspects")
    diff = len([k for k in f if k != "arm" and n and f[k] != n.get(k)]) if n else -1
    why = "%d suspects" % sus + (", %d fields differ from no_integrity" % diff if diff >= 0
                                 else ", no no_integrity arm to compare")
    live = sus > 0 and (diff > 0 or diff == -1)
    out.append(("ca_tbl_full", "LIVE" if live else "DEGENERATE", why))
    return out


def trigger(R):
    out = []
    for arm, r in R.items():
        if arm not in HYBRID:
            continue
        pop = g(r, "trig_tp") + g(r, "trig_fp")
        out.append((arm, "LIVE" if pop > 0 else "DEGENERATE", "tp+fp = %d" % pop))
    return out


# A tier with zero active cycles still draws CAD/idle listening energy, and that is
# correct behaviour, not a defect: static_2p4 draws 0.014 J on 868 and 0.008 J on UWB
# while occupying neither. The first version of this rule flagged ANY non-zero draw on
# a zero-cycle tier and reported 66.5% of the corpus degenerate -- an artifact of the
# rule, not a property of the data. It now requires a MATERIAL share.
#
# ENERGY_MATERIAL_FRAC is ours, not physical: the point below which a draw on an
# unoccupied tier is indistinguishable from idle listening.
ENERGY_MATERIAL_FRAC = 0.01


def energy(R):
    out = []
    for arm, r in R.items():
        tot = g(r, "totalJ") or sum(g(r, k) for k in ("j_868", "j_2p4", "j_uwb")) or 1.0
        bad = []
        for t, cyc, j in (("868", "cycles_868", "j_868"), ("2p4", "cycles_2p4", "j_2p4"),
                          ("uwb", "cycles_uwb", "j_uwb")):
            draw = g(r, j)
            if g(r, cyc) == 0 and draw > ENERGY_MATERIAL_FRAC * tot:
                bad.append("%s draws %.3f J (%.1f%% of total) on 0 cycles"
                           % (t, draw, 100 * draw / tot))
        out.append((arm, "DEGENERATE" if bad else "LIVE",
                    "; ".join(bad) if bad
                    else "no unoccupied tier draws >%.0f%% of total"
                         % (100 * ENERGY_MATERIAL_FRAC)))
    return out


FN = {"tiering": tiering, "integrity": integrity, "trigger": trigger, "energy": energy}


def main():
    if len(sys.argv) < 2:
        print(__doc__); return 2
    d = sys.argv[1]
    mech = sys.argv[2] if len(sys.argv) > 2 else None
    R = rows(d)
    if R is None:
        print("  NO arm_summary.csv -- the run produced nothing")
        return 1 if mech else 2
    if mech is None:
        print("  NO MECHANISM NAMED. Reporting all four; no single verdict is returned.")
        print("  A run degenerate for one mechanism may be a perfectly good test of another.")
        for m in MECHS:
            print("  --- %s" % m)
            for arm, v, why in FN[m](R):
                print("      %-22s %-11s %s" % (arm, v, why))
        return 2
    if mech not in FN:
        print("  unknown mechanism %r; known: %s" % (mech, ", ".join(MECHS)))
        return 2
    res = FN[mech](R)
    for arm, v, why in res:
        print("  %-22s %-11s %s" % (arm, v, why))
    bad = any(v in ("DEGENERATE", "BROKEN") for _, v, _ in res)
    print("  VERDICT for mechanism %r: %s" % (mech, "DEGENERATE" if bad else "LIVE"))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
