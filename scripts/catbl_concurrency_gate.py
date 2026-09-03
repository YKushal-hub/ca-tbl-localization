#!/usr/bin/env python3
"""G16 -- the anchor-survey schedule asserted the concurrency that actually occurred.

WHY THIS EXISTS. Two scheduling defects in one build produced collision rates that
read as delivery rates. The first fired every intra-site pair at one instant; the
second selected one pair per SITE while claiming one pair globally. Both were caught
only because tier-2 sites gave an unmistakable 0.98-against-0.00 contrast between the
first-transmitting site and the rest. A deployment without that contrast -- one site,
or uniform geometry -- would have passed either artefact off as a physics result.

Vigilance did not catch them. Geometry did, and geometry will not be there next time.

WHAT IT CHECKS, AND THE POPULATION HOLE THAT WIDENED IT. The first version counted only
SURVEY transmissions, and so reported a concurrency of 1 while a beacon was transmitting
into the same channel. The beacon turned out to be the DOMINANT contention source --
worth 18 percentage points on the UWB adjacent link. The gate succeeded because its
population excluded the thing it exists to catch, which is the fourth instance of that
class here (TRACKED_ELSEWHERE, the empty population passing, G9's hardcoded allow-list,
and now this) and the first found in a gate built the same week.

So the population is now every transmission the tier carries concurrent with an
exchange, reported by class:

  surveyConcurrent  transmissions the SCHEDULE controls -> ASSERTED against the bound
  beaconConcurrent  discovery beacons in the same epoch -> REPORTED, never asserted
  totalConcurrent   the physically relevant number

Asserting on beacons would fail the gate on correct behaviour: beacon traffic is a
property of the scenario, not a scheduling defect. Assert on what the schedule
controls; report what it does not.

Concurrency is counted PER TIER because tier 2 and tier 3 are different radios and
cannot interfere; counting across tiers would report violations that are not.

An empty trace is VACUOUS, not a pass, and is reported as such.
"""
import csv, collections, sys, os


def check(path, max_concurrent=1):
    if not os.path.isfile(path):
        return None, "no such file: %s" % path
    rows = list(csv.DictReader(open(path, newline="")))
    if not rows:
        return None, "trace has no rows -- VACUOUS, not a pass"
    if "surveyConcurrent" not in rows[0]:
        if "concurrentInTier" in rows[0]:
            return None, ("trace has concurrentInTier but no surveyConcurrent -- written "
                          "by a build predating the widened population, so it cannot show "
                          "beacon contention and is not a pass")
        return None, ("trace has no concurrency columns -- written by a build predating "
                      "the detector; it cannot be checked and is not a pass")
    worst = 0
    bad = []
    per_epoch = collections.Counter()
    beacon_worst = 0
    beacon_nonzero = 0
    for r in rows:
        c = int(r["surveyConcurrent"])
        b = int(r["beaconConcurrent"])
        worst = max(worst, c)
        beacon_worst = max(beacon_worst, b)
        if b > 0:
            beacon_nonzero += 1
        per_epoch[(r["t"], r["tier"])] += 1
        if c > max_concurrent:
            bad.append(r)
    # independent recount from the rows themselves, not from the emitted column:
    # a detector that only reports its own counter cannot catch a counter defect
    recount_worst = max(per_epoch.values())
    return dict(rows=len(rows), worst=worst, recount=recount_worst, bad=bad,
                beacon_worst=beacon_worst, beacon_nonzero=beacon_nonzero,
                epochs=len({r["t"] for r in rows}),
                tiers=sorted({r["tier"] for r in rows})), None


def main():
    if len(sys.argv) < 2:
        print("usage: catbl_concurrency_gate.py <trace_anchor_ranges_*.csv> [max_concurrent]")
        return 2
    path = sys.argv[1]
    mx = int(sys.argv[2]) if len(sys.argv) > 2 else 1
    res, err = check(path, mx)
    print("--- G16 anchor-survey concurrency ---")
    if err:
        print("G16 FAIL: %s" % err)
        return 1
    print("population: %d exchanges over %d epochs, tiers %s"
          % (res["rows"], res["epochs"], ",".join(res["tiers"])))
    print("schedule asserts at most %d concurrent transmission(s) per tier per epoch" % mx)
    print("  survey concurrency, max       : %d   (ASSERTED)" % res["worst"])
    print("  independent recount from rows : %d" % res["recount"])
    print("  beacon concurrency, max       : %d   (REPORTED, not asserted)"
          % res["beacon_worst"])
    print("  exchanges with a beacon present: %d of %d = %.1f%%"
          % (res["beacon_nonzero"], res["rows"],
             100.0 * res["beacon_nonzero"] / res["rows"]))
    if res["beacon_worst"] == 0 and res["rows"] > 0:
        print("  NOTE: zero beacons anywhere in this trace. That is either a quiesced")
        print("        run or a broken counter -- the column cannot distinguish them,")
        print("        so it is reported rather than treated as a clean result.")
    if res["worst"] != res["recount"]:
        print("G16 FAIL: the emitted counter and the recount disagree -- the detector "
              "itself is wrong, which is the one failure a self-reported counter cannot see")
        return 1
    if res["bad"]:
        print("G16 FAIL: %d exchange(s) exceeded the asserted concurrency" % len(res["bad"]))
        for r in res["bad"][:5]:
            print("    t=%s tier=%s pair=%s-%s survey=%s beacon=%s"
                  % (r["t"], r["tier"], r["anchorA"], r["anchorB"],
                     r["surveyConcurrent"], r["beaconConcurrent"]))
        print("  a delivery rate measured under this schedule is partly a collision rate")
        return 1
    print("=== G16 PASS ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
