#!/usr/bin/env python3
"""G16 -- the anchor-survey schedule asserted the concurrency that actually occurred.

WHY THIS EXISTS. Two scheduling defects in one build produced collision rates that
read as delivery rates. The first fired every intra-site pair at one instant; the
second selected one pair per SITE while claiming one pair globally. Both were caught
only because tier-2 sites gave an unmistakable 0.98-against-0.00 contrast between the
first-transmitting site and the rest. A deployment without that contrast -- one site,
or uniform geometry -- would have passed either artefact off as a physics result.

Vigilance did not catch them. Geometry did, and geometry will not be there next time.

WHAT IT CHECKS. `concurrentInTier` is emitted per anchor-to-anchor exchange and counts
transmissions already made by that tier in that epoch. Under a serialised schedule it
must never exceed 1. Concurrency is counted PER TIER because tier 2 and tier 3 are
different radios and cannot interfere with each other; counting across tiers would
report violations that are not violations.

An empty trace is VACUOUS, not a pass, and is reported as such.
"""
import csv, collections, sys, os


def check(path, max_concurrent=1):
    if not os.path.isfile(path):
        return None, "no such file: %s" % path
    rows = list(csv.DictReader(open(path, newline="")))
    if not rows:
        return None, "trace has no rows -- VACUOUS, not a pass"
    if "concurrentInTier" not in rows[0]:
        return None, ("trace has no concurrentInTier column -- written by a build "
                      "predating the detector; it cannot be checked and is not a pass")
    worst = 0
    bad = []
    per_epoch = collections.Counter()
    for r in rows:
        c = int(r["concurrentInTier"])
        worst = max(worst, c)
        per_epoch[(r["t"], r["tier"])] += 1
        if c > max_concurrent:
            bad.append(r)
    # independent recount from the rows themselves, not from the emitted column:
    # a detector that only reports its own counter cannot catch a counter defect
    recount_worst = max(per_epoch.values())
    return dict(rows=len(rows), worst=worst, recount=recount_worst, bad=bad,
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
    print("  emitted counter, max observed : %d" % res["worst"])
    print("  independent recount from rows : %d" % res["recount"])
    if res["worst"] != res["recount"]:
        print("G16 FAIL: the emitted counter and the recount disagree -- the detector "
              "itself is wrong, which is the one failure a self-reported counter cannot see")
        return 1
    if res["bad"]:
        print("G16 FAIL: %d exchange(s) exceeded the asserted concurrency" % len(res["bad"]))
        for r in res["bad"][:5]:
            print("    t=%s tier=%s pair=%s-%s concurrent=%s"
                  % (r["t"], r["tier"], r["anchorA"], r["anchorB"], r["concurrentInTier"]))
        print("  a delivery rate measured under this schedule is partly a collision rate")
        return 1
    print("=== G16 PASS ===")
    return 0


if __name__ == "__main__":
    sys.exit(main())
