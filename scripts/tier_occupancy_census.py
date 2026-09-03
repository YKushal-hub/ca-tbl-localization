#!/usr/bin/env python3
"""Which configurations actually put work on tier 3?

WHY. Novelty part 1 self-calibrates tier-3 anchors. In the one configuration measured
so far, tier 3 supplied 61 of 6030 fixes -- 1% -- and the tier-1 floor supplied 98.4%.
A quiet-window comparison registered on tier-3-served cycles was therefore VACUOUS
before it launched. That was one configuration, and it lacked the escalation-driving
flags used elsewhere in the corpus; other cells have recorded far higher UWB occupancy.

So the question is not "does tier 3 matter" but "in WHICH configurations does it", and
the corpus already on disk answers it. This is a census, not a hypothesis: no
prediction is registered on the outcome.

OCCUPANCY IS NOT SERVICE, and conflating them is what produced the vacuous campaign:

  occupied   activeTier == T          the policy selected T for this cycle
  serving    fixTier == T and hadFix  T actually produced the position

A tier can be selected on most cycles and supply almost no fixes -- which is exactly
the selection-not-fusion structure already on record, where the floor supplies the fix
whenever the active tier fails to solve. Both columns are reported, never one.
"""
import csv, glob, os, re, sys, collections

TIERS = ("868MHz", "2.4GHz", "UWB")
FLAGS_OF_INTEREST = ("duration", "boxHalf", "gwGrid", "dwell", "escalationRequiresEvidence",
                     "floorOnDemand", "nlosSource", "pcsForm", "env", "envFacility",
                     "injectFaultAnchors", "faultMultiplicity", "suspectRecovery",
                     "uwbAnchors", "t2Anchors", "uwbTriad")


def flags_from_sidecar(d):
    p = os.path.join(d, "run_id.txt")
    if not os.path.isfile(p):
        return {}
    try:
        line = open(p, errors="replace").read().split("\n")[1]
    except IndexError:
        return {}
    out = {}
    for m in re.finditer(r"--([A-Za-z0-9_]+)=([^\s]+)", line):
        out[m.group(1)] = m.group(2)
    return out


def census_dir(d):
    occ = collections.Counter()
    srv = collections.Counter()
    cycles = 0
    fixes = 0
    for t in sorted(glob.glob(os.path.join(d, "trace_ca_tbl_*.csv")) +
                    glob.glob(os.path.join(d, "trace_static_*.csv"))):
        if "anchor_ranges" in t:
            continue
        try:
            rows = csv.DictReader(open(t, newline="", errors="replace"))
            for r in rows:
                if "activeTier" not in r:
                    break
                cycles += 1
                occ[r.get("activeTier", "")] += 1
                if r.get("hadFix") in ("1", "true", "True"):
                    fixes += 1
                    srv[r.get("fixTier", "")] += 1
        except Exception:
            continue
    return cycles, fixes, occ, srv


def main():
    roots = sorted({os.path.dirname(p) for p in
                    glob.glob("results/**/trace_ca_tbl_*.csv", recursive=True)})
    roots = [r for r in roots if r]
    rows = []
    for d in roots:
        cycles, fixes, occ, srv = census_dir(d)
        if cycles == 0 or fixes == 0:
            continue
        rows.append(dict(dir=d, cycles=cycles, fixes=fixes,
                         occ={t: occ[t] / cycles for t in TIERS},
                         srv={t: srv[t] / fixes for t in TIERS},
                         flags=flags_from_sidecar(d)))
    if not rows:
        print("POPULATION EMPTY: no results directory has a readable ca_tbl trace.")
        print("VACUOUS, not a pass.")
        return 1
    rows.sort(key=lambda r: -r["srv"]["UWB"])
    print("TIER OCCUPANCY vs SERVICE -- %d directories with traces, sorted by tier-3 FIX share\n"
          % len(rows))
    print("  %-42s %8s %8s | %-22s | %-22s"
          % ("directory", "cycles", "fixes", "OCCUPIED (activeTier)", "SERVING (fixTier)"))
    print("  %-42s %8s %8s | %6s %7s %7s | %6s %7s %7s"
          % ("", "", "", "868", "2.4", "UWB", "868", "2.4", "UWB"))
    for r in rows[:40]:
        print("  %-42s %8d %8d | %5.1f%% %6.1f%% %6.1f%% | %5.1f%% %6.1f%% %6.1f%%"
              % (r["dir"][:42], r["cycles"], r["fixes"],
                 100 * r["occ"]["868MHz"], 100 * r["occ"]["2.4GHz"], 100 * r["occ"]["UWB"],
                 100 * r["srv"]["868MHz"], 100 * r["srv"]["2.4GHz"], 100 * r["srv"]["UWB"]))
    if len(rows) > 40:
        print("  ... %d more, all with a lower tier-3 fix share" % (len(rows) - 40))

    print("\n--- the distinction the vacuous campaign confounded")
    worst = max(rows, key=lambda r: r["occ"]["UWB"] - r["srv"]["UWB"])
    print("  largest occupancy-minus-service gap on tier 3: %s" % worst["dir"])
    print("    occupied %.1f%%  but serving %.1f%%  -- a tier can be selected and still not fix"
          % (100 * worst["occ"]["UWB"], 100 * worst["srv"]["UWB"]))

    print("\n--- flags of the top tier-3-serving configurations")
    for r in rows[:5]:
        f = r["flags"]
        kv = " ".join("%s=%s" % (k, f[k]) for k in FLAGS_OF_INTEREST if k in f)
        print("  %-42s uwb_fix %.1f%%" % (r["dir"][:42], 100 * r["srv"]["UWB"]))
        print("      %s" % (kv if kv else "(no sidecar flags recorded)"))

    n_material = sum(1 for r in rows if r["srv"]["UWB"] >= 0.05)
    print("\n--- verdict input")
    print("  directories with tier-3 fix share >= 5%%: %d of %d" % (n_material, len(rows)))
    print("  best tier-3 fix share observed          : %.1f%%  (%s)"
          % (100 * rows[0]["srv"]["UWB"], rows[0]["dir"]))
    return 0


if __name__ == "__main__":
    sys.exit(main())
