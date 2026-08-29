#!/usr/bin/env python3
"""p10_env_v7 read-out.

Metric is JOINT and per-cycle, computed from the trace, never from arm_summary
aggregates: a seed meets spec iff
    (cycles carrying a fix whose errM <= R) / 1440 >= A
Population is all 1440 cycles, not fix-cycles. Calibrated against the two
published anchors on results/p10_plane_fix (ca_tbl_full 19/20, static_2p4 14/20).

seeds_meeting_spec is an OBSERVABLE, not a threshold. No >=15/20 pass rule is
applied anywhere in this file.
"""
import csv, hashlib, os, re, sys, glob, statistics as st

A, R, CYC = 0.95, 25.0, 1440
R80 = 1149.5            # measured post-D1, tier 2
BOXES = [1500, 2000, 2500, 3000, 4000, 5000, 6000, 7500]
ARMS = ["static_2p4", "static_868", "static_uwb",
        "ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold"]
ROOT = "results/p10_env_v7"
SEEDS = range(1, 21)


def census():
    """Attempted / complete / digest-verified. Returns the usable cell set."""
    usable, incomplete, digest_fail, provs = set(), [], [], {}
    for b in BOXES:
        for s in SEEDS:
            d = "%s/b%d_seed%d" % (ROOT, b, s)
            if not os.path.isdir(d):
                incomplete.append((b, s, "no directory")); continue
            tr = len(glob.glob(os.path.join(d, "trace_*.csv")))
            asum = os.path.join(d, "arm_summary.csv")
            rows = (sum(1 for _ in open(asum)) - 1) if os.path.exists(asum) else -1
            rid = os.path.join(d, "run_id.txt")
            txt = open(rid).read() if os.path.exists(rid) else ""
            lines = txt.splitlines()
            provs.setdefault(lines[2] if len(lines) > 2 else "NONE", []).append((b, s))
            nd = ok = 0
            for line in txt.split("digest sha256", 1)[-1].strip().splitlines():
                m = re.match(r"([0-9a-f]{64})\s+(.+)$", line.strip())
                if not m:
                    continue
                nd += 1
                p = os.path.join(d, m.group(2))
                if os.path.exists(p) and hashlib.sha256(
                        open(p, "rb").read()).hexdigest() == m.group(1):
                    ok += 1
                else:
                    digest_fail.append((b, s, m.group(2)))
            if tr == 6 and rows == 6 and nd == 7 and ok == 7:
                usable.add((b, s))
            else:
                incomplete.append((b, s, "traces=%d rows=%s digest=%d/%d" % (tr, rows, ok, nd)))
    return usable, incomplete, digest_fail, provs


def joint(b, s, arm):
    """Fraction of ALL cycles carrying an actionable fix. None if unreadable."""
    f = "%s/b%d_seed%d/trace_%s_seed%d.csv" % (ROOT, b, s, arm, s)
    if not os.path.exists(f):
        return None
    act = 0
    for r in csv.DictReader(open(f)):
        if r.get("hadFix") in ("1", "true", "True"):
            try:
                if float(r["errM"]) <= R:
                    act += 1
            except (ValueError, KeyError):
                pass
    return act / CYC


def totalJ(b, s, arm):
    f = "%s/b%d_seed%d/arm_summary.csv" % (ROOT, b, s)
    if not os.path.exists(f):
        return None
    for r in csv.DictReader(open(f)):
        if r["arm"] == arm:
            return float(r["totalJ"])
    return None


def main():
    usable, incomplete, digest_fail, provs = census()
    print("=== 1. COMPLETENESS CENSUS ===")
    print("  attempted        %d  (%d boxes x %d seeds)" % (len(BOXES) * 20, len(BOXES), 20))
    print("  usable           %d" % len(usable))
    print("  incomplete       %d" % len(incomplete))
    for c in incomplete[:10]:
        print("      %s" % (c,))
    print("  digest failures  %d" % len(digest_fail))
    for c in digest_fail[:10]:
        print("      %s" % (c,))
    print("  exclusions       %s" % ("none" if not incomplete and not digest_fail
                                     else "the %d cells above" % (len(incomplete) + len(digest_fail))))
    print("\n=== 2. PROVENANCE ===")
    for k, v in provs.items():
        print("  %-46s  %d cells" % (k, len(v)))
        if "dirty 0" not in k:
            print("      *** NOT CLEAN -- these cells are unciteable ***")

    print("\n=== 3-5. PER-ARM, static_2p4 FIRST ===")
    table = {}
    for arm in ARMS:
        print("\n  --- %s ---" % arm)
        print("  %8s %8s %14s %12s %12s" % ("boxHalf", "s/r80", "meets_spec", "mean M1 J", "avail_mean"))
        for b in BOXES:
            cells = [s for s in SEEDS if (b, s) in usable]
            if not cells:
                print("  %8d %8.2f %14s %12s %12s" % (b, 0.47 * b / R80, "no data", "-", "-"))
                continue
            av = [joint(b, s, arm) for s in cells]
            av = [x for x in av if x is not None]
            meets = sum(1 for x in av if x >= A)
            js = [totalJ(b, s, arm) for s in cells]
            js = [x for x in js if x is not None]
            table[(arm, b)] = (meets, len(av), st.mean(js) if js else float("nan"),
                               st.mean(av) if av else float("nan"))
            print("  %8d %8.2f %10d/%-3d %12.3f %12.4f"
                  % (b, 0.47 * b / R80, meets, len(av),
                     st.mean(js) if js else float("nan"),
                     st.mean(av) if av else float("nan")))

    print("\n=== 6. VERDICT PER BOX SIZE ===")
    print("  PASS         a tiered arm meets spec on more seeds than every single-band arm")
    print("  FAIL         a single-band arm meets spec and wins M1")
    print("  INCONCLUSIVE no arm meets spec on any seed")
    print("\n  %8s %8s %28s %28s   %s" % ("boxHalf", "s/r80", "best single-band", "best tiered", "verdict"))
    single = ["static_2p4", "static_868", "static_uwb"]
    tiered = ["ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold"]
    for b in BOXES:
        if not any((a, b) in table for a in ARMS):
            continue
        sb = [(table[(a, b)][0], -table[(a, b)][2], a) for a in single if (a, b) in table]
        tb = [(table[(a, b)][0], -table[(a, b)][2], a) for a in tiered if (a, b) in table]
        sb.sort(reverse=True); tb.sort(reverse=True)
        bs, bt = sb[0], tb[0]
        s_meets, s_j, s_a = bs[0], -bs[1], bs[2]
        t_meets, t_j, t_a = bt[0], -bt[1], bt[2]
        if s_meets == 0 and t_meets == 0:
            v = "INCONCLUSIVE"
        elif s_meets > 0 and s_j <= t_j:
            v = "FAIL"
        elif t_meets > s_meets:
            v = "PASS"
        else:
            v = "FAIL"
        print("  %8d %8.2f %18s %2d/20 %6.1fJ %18s %2d/20 %6.1fJ   %s"
              % (b, 0.47 * b / R80, s_a, s_meets, s_j, t_a, t_meets, t_j, v))

    print("\n=== 7. CROSSOVER ===")
    prev = None
    cross = []
    for b in BOXES:
        if ("static_2p4", b) not in table or ("ca_tbl_full", b) not in table:
            continue
        d = table[("static_2p4", b)][0] - table[("ca_tbl_full", b)][0]
        if prev is not None and (prev > 0) != (d > 0):
            cross.append(b)
        prev = d
    print("  seeds_meeting_spec(static_2p4) - seeds_meeting_spec(ca_tbl_full):")
    for b in BOXES:
        if ("static_2p4", b) in table:
            print("    boxHalf %5d (s/r80 %.2f):  %+d" % (b, 0.47 * b / R80,
                  table[("static_2p4", b)][0] - table[("ca_tbl_full", b)][0]))
    print("  ordering changes at: %s" % (cross if cross else "nowhere in the swept range"))


if __name__ == "__main__":
    main()
