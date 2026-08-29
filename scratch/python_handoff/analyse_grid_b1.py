#!/usr/bin/env python3
"""Stage B1 read-out: results/p10_grid, 320 cells.

Screen (registered in PREREGISTRATION.md §5, before B1 ran): carry forward a
(envFacility, nlosSource, dwell) combination iff BOTH
  1. static_2p4 fails spec at >= 2 of the four densities, AND
  2. ca_tbl_full meets spec at >= 1 density where static_2p4 does not.
Clause 2 separates "the architecture wins" from "everything broke".

The full surface is printed -- every cell, winning and losing alike.
"""
import csv, glob, hashlib, math, os, random, re, statistics as st

A, R, CYC = 0.95, 25.0, 1440
EFS = [0, 1, 3, 4]
EFNAME = {0: "RURAL_OPEN", 1: "RURAL_FOREST", 3: "URBAN_MOD", 4: "URBAN_DENSE"}
NSS = ["legacy", "propagation"]
DWS = [90, 800]
BOXES = [1500, 2000, 2500, 3000]
SEEDS = range(1, 6)
ARMS = ["static_2p4", "ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold",
        "static_868", "static_uwb"]
random.seed(20260828)


def d(ef, ns, dw, b, s):
    return "results/p10_grid/c_ef%d_%s_dw%d_b%d/seed_%d" % (ef, ns, dw, b, s)


def census():
    usable, inc, dbad, provs = set(), [], [], {}
    for ef in EFS:
        for ns in NSS:
            for dw in DWS:
                for b in BOXES:
                    for s in SEEDS:
                        p = d(ef, ns, dw, b, s)
                        tr = len(glob.glob(os.path.join(p, "trace_*.csv")))
                        a = os.path.join(p, "arm_summary.csv")
                        rows = (sum(1 for _ in open(a)) - 1) if os.path.exists(a) else -1
                        rid = os.path.join(p, "run_id.txt")
                        txt = open(rid).read() if os.path.exists(rid) else ""
                        ls = txt.splitlines()
                        provs.setdefault(ls[2] if len(ls) > 2 else "NONE", []).append(1)
                        nd = ok = 0
                        for line in txt.split("digest sha256", 1)[-1].strip().splitlines():
                            m = re.match(r"([0-9a-f]{64})\s+(.+)$", line.strip())
                            if not m:
                                continue
                            nd += 1
                            fp = os.path.join(p, m.group(2))
                            if os.path.exists(fp) and hashlib.sha256(
                                    open(fp, "rb").read()).hexdigest() == m.group(1):
                                ok += 1
                            else:
                                dbad.append((ef, ns, dw, b, s, m.group(2)))
                        if tr == 6 and rows == 6 and nd == 7 and ok == 7:
                            usable.add((ef, ns, dw, b, s))
                        else:
                            inc.append((ef, ns, dw, b, s, "tr=%d rows=%s dig=%d/%d" % (tr, rows, ok, nd)))
    return usable, inc, dbad, provs


def joint(ef, ns, dw, b, s, arm):
    f = os.path.join(d(ef, ns, dw, b, s), "trace_%s_seed%d.csv" % (arm, s))
    if not os.path.exists(f):
        return None
    a = 0
    for r in csv.DictReader(open(f)):
        if r.get("hadFix") in ("1", "true", "True"):
            try:
                if float(r["errM"]) <= R:
                    a += 1
            except (ValueError, KeyError):
                pass
    return a / CYC


def tj(ef, ns, dw, b, s, arm):
    f = os.path.join(d(ef, ns, dw, b, s), "arm_summary.csv")
    if not os.path.exists(f):
        return None
    for r in csv.DictReader(open(f)):
        if r["arm"] == arm:
            return float(r["totalJ"])
    return None


def gci(rr, B=10000):
    n = len(rr)
    lg = [math.log(x) for x in rr]
    pt = math.exp(sum(lg) / n)
    bs = sorted(math.exp(sum(lg[random.randrange(n)] for _ in range(n)) / n) for _ in range(B))
    return pt, bs[int(.025 * B)], bs[int(.975 * B)]


def main():
    usable, inc, dbad, provs = census()
    print("=== 1. COMPLETENESS CENSUS ===")
    print("  attempted 320   usable %d   incomplete %d   digest failures %d"
          % (len(usable), len(inc), len(dbad)))
    for c in inc[:8]:
        print("     %s" % (c,))
    for c in dbad[:8]:
        print("     %s" % (c,))
    print("  exclusions: %s" % ("none" if not inc and not dbad else "cells above"))
    print("\n=== 2. PROVENANCE ===")
    for k, v in provs.items():
        print("  %-46s %d cells%s" % (k, len(v), "" if "dirty 0" in k else "  *** NOT CLEAN ***"))

    print("\n=== 3. THE FULL SURFACE (320 cells) ===")
    print("  meets = seeds meeting spec out of 5. An OBSERVABLE, not a threshold.")
    print("  M1 = geometric mean of per-seed ca_tbl_full/static_2p4 ratios, bootstrap 95%% CI.\n")
    print("  %-12s %-11s %4s %6s | %5s %5s %5s %5s | %-22s"
          % ("envFacility", "nlosSource", "dw", "boxHalf", "2p4", "full", "noint", "fixed", "M1 ratio [95% CI]"))
    print("  " + "-" * 108)
    flagged, surface = [], {}
    for ef in EFS:
        for ns in NSS:
            for dw in DWS:
                for b in BOXES:
                    cells = [s for s in SEEDS if (ef, ns, dw, b, s) in usable]
                    m = {}
                    for arm in ARMS:
                        av = [joint(ef, ns, dw, b, s, arm) for s in cells]
                        m[arm] = sum(1 for x in av if x is not None and x >= A)
                    rr = [tj(ef, ns, dw, b, s, "ca_tbl_full") / tj(ef, ns, dw, b, s, "static_2p4")
                          for s in cells]
                    pt, lo, hi = gci(rr)
                    surface[(ef, ns, dw, b)] = (m, pt, lo, hi, len(cells))
                    print("  %-12s %-11s %4d %6d | %5s %5s %5s %5s | %6.3f [%6.3f, %6.3f]"
                          % (EFNAME[ef], ns, dw, b,
                             "%d/%d" % (m["static_2p4"], len(cells)),
                             "%d/%d" % (m["ca_tbl_full"], len(cells)),
                             "%d/%d" % (m["ca_tbl_no_integrity"], len(cells)),
                             "%d/%d" % (m["fixed_threshold"], len(cells)),
                             pt, lo, hi))
                print()

    print("=== 4. THE REGISTERED SCREEN ===")
    print("  carry forward iff (1) static_2p4 fails at >=2 densities AND")
    print("                    (2) ca_tbl_full meets at >=1 density where static_2p4 does not\n")
    print("  %-12s %-11s %4s | %8s %8s | %s" % ("envFacility", "nlosSource", "dw",
                                                "clause 1", "clause 2", "carried?"))
    for ef in EFS:
        for ns in NSS:
            for dw in DWS:
                n = len(SEEDS)
                fails = [b for b in BOXES if surface[(ef, ns, dw, b)][0]["static_2p4"] < n]
                c1 = len(fails) >= 2
                c2 = any(surface[(ef, ns, dw, b)][0]["ca_tbl_full"] >
                         surface[(ef, ns, dw, b)][0]["static_2p4"] and
                         surface[(ef, ns, dw, b)][0]["ca_tbl_full"] > 0 for b in fails)
                carry = c1 and c2
                if carry:
                    flagged.append((ef, ns, dw))
                print("  %-12s %-11s %4d | %8s %8s | %s"
                      % (EFNAME[ef], ns, dw, "PASS" if c1 else "fail",
                         "PASS" if c2 else "fail", "CARRY" if carry else "-"))
    print("\n  combinations carried forward: %d of 16" % len(flagged))
    for f in flagged:
        print("     envFacility=%s nlosSource=%s dwell=%d" % (EFNAME[f[0]], f[1], f[2]))
    if not flagged:
        print("\n  *** ABORT CONDITION 1: the screen flags no combination. ***")
        print("  Per PREREGISTRATION §7.1: stop, do not extend factor ranges.")
        print("  The grid contains no region satisfying all three conditions and the")
        print("  §1 hypothesis is falsified over the space registered.")
    return flagged


if __name__ == "__main__":
    main()
