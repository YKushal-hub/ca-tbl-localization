#!/usr/bin/env python3
"""Stage A read-out: p10_env_v8 (floorOnDemand=true) against p10_env_v7.

Verdicts are b49df46 §4 verbatim. Where that rule is silent -- any geometry where
an arm is not unanimous across seeds, the only registered arm-level binarization
(>=15/20) having been withdrawn -- the silence is REPORTED, not closed.

seeds_meeting_spec is an observable. It is never used as a decision rule here.
M1 is the geometric mean of per-seed ratios with a seed-level bootstrap CI.
"""
import csv, glob, hashlib, math, os, random, re, statistics as st

A, R, CYC, R80 = 0.95, 25.0, 1440, 1149.5
BOXES = [1500, 2000, 2500, 3000, 4000, 5000, 6000, 7500]
ARMS = ["static_2p4", "static_868", "static_uwb",
        "ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold"]
SEEDS = range(1, 21)
random.seed(20260827)


def cell(root, b, s):
    return "%s/b%d_seed%d" % (root, b, s)


def census(root):
    usable, incomplete, digest_fail, provs = set(), [], [], {}
    for b in BOXES:
        for s in SEEDS:
            d = cell(root, b, s)
            if not os.path.isdir(d):
                incomplete.append((b, s, "no dir")); continue
            tr = len(glob.glob(os.path.join(d, "trace_*.csv")))
            asum = os.path.join(d, "arm_summary.csv")
            rows = (sum(1 for _ in open(asum)) - 1) if os.path.exists(asum) else -1
            rid = os.path.join(d, "run_id.txt")
            txt = open(rid).read() if os.path.exists(rid) else ""
            ls = txt.splitlines()
            provs.setdefault(ls[2] if len(ls) > 2 else "NONE", []).append((b, s))
            nd = ok = 0
            for line in txt.split("digest sha256", 1)[-1].strip().splitlines():
                m = re.match(r"([0-9a-f]{64})\s+(.+)$", line.strip())
                if not m:
                    continue
                nd += 1
                p = os.path.join(d, m.group(2))
                if os.path.exists(p) and hashlib.sha256(open(p, "rb").read()).hexdigest() == m.group(1):
                    ok += 1
                else:
                    digest_fail.append((b, s, m.group(2)))
            if tr == 6 and rows == 6 and nd == 7 and ok == 7:
                usable.add((b, s))
            else:
                incomplete.append((b, s, "tr=%d rows=%s dig=%d/%d" % (tr, rows, ok, nd)))
    return usable, incomplete, digest_fail, provs


def joint(root, b, s, arm):
    f = "%s/trace_%s_seed%d.csv" % (cell(root, b, s), arm, s)
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


def tj(root, b, s, arm):
    f = os.path.join(cell(root, b, s), "arm_summary.csv")
    if not os.path.exists(f):
        return None
    for r in csv.DictReader(open(f)):
        if r["arm"] == arm:
            return float(r["totalJ"])
    return None


def geo_ci(ratios, B=10000):
    n = len(ratios)
    lg = [math.log(x) for x in ratios]
    pt = math.exp(sum(lg) / n)
    bs = sorted(math.exp(sum(lg[random.randrange(n)] for _ in range(n)) / n) for _ in range(B))
    return pt, bs[int(0.025 * B)], bs[int(0.975 * B)]


def meets(root, b, arm, usable):
    cells = [s for s in SEEDS if (b, s) in usable]
    av = [joint(root, b, s, arm) for s in cells]
    return sum(1 for x in av if x is not None and x >= A), len(av)


def verdict(fm, fn_, sm, sn, lo, hi):
    """b49df46 §4 verbatim. Returns (label, resolved?)."""
    f_unan = fm == fn_ or fm == 0
    s_unan = sm == sn or sm == 0
    if not (f_unan and s_unan):
        return ("rule SILENT (arm not unanimous; >=15/20 withdrawn)", False)
    if fm == 0:
        return ("INCONCLUSIVE (ca_tbl_full does not meet (A,R))", True)
    if sm == 0:
        return ("INCONCLUSIVE (no competitor meets (A,R))", True)
    if hi < 1.0:
        return ("PASS", True)
    if lo > 1.0:
        return ("FAIL (competitor achieves lower M1)", True)
    return ("INCONCLUSIVE (CI spans 1.0)", True)


def main():
    print("=== 1. COMPLETENESS CENSUS: p10_env_v8 ===")
    usable, inc, dbad, provs = census("results/p10_env_v8")
    print("  attempted 160   usable %d   incomplete %d   digest failures %d"
          % (len(usable), len(inc), len(dbad)))
    for c in inc[:8]:
        print("      incomplete %s" % (c,))
    for c in dbad[:8]:
        print("      digest     %s" % (c,))
    print("  exclusions: %s" % ("none" if not inc and not dbad else "the cells above"))

    print("\n=== 2. PROVENANCE ===")
    for k, v in provs.items():
        print("  %-46s %d cells%s" % (k, len(v), "" if "dirty 0" in k else "   *** NOT CLEAN ***"))

    print("\n=== 3. NULL CONTROL: static_2p4 must be byte-identical to v7 ===")
    allsame = True
    for b in BOXES:
        same = miss = 0
        for s in SEEDS:
            a = "results/p10_env_v7/b%d_seed%d/trace_static_2p4_seed%d.csv" % (b, s, s)
            c = "results/p10_env_v8/b%d_seed%d/trace_static_2p4_seed%d.csv" % (b, s, s)
            if not (os.path.exists(a) and os.path.exists(c)):
                miss += 1; continue
            if hashlib.sha256(open(a, 'rb').read()).hexdigest() == \
               hashlib.sha256(open(c, 'rb').read()).hexdigest():
                same += 1
        if same != 20:
            allsame = False
        print("  boxHalf %5d : %2d/20 identical%s" % (b, same, "" if same == 20 else "   <== MOVED"))
    print("  VERDICT: %s" % ("null control PASSES -- flag confined to hybrid arms"
                             if allsame else
                             "*** NULL CONTROL FAILS -- STOP, nothing else is readable ***"))
    if not allsame:
        return

    print("\n=== 4. THE FULL SURFACE ===")
    print("  M1 ratio = ca_tbl_full / static_2p4, geometric mean of per-seed ratios,")
    print("  seed-level bootstrap 95%% CI, 10000 resamples. PASS needs CI entirely < 1.0.")
    print("  seeds_meeting_spec is an observable, printed for every arm, used as no rule.\n")
    hdr = "  %7s %6s | " % ("boxHalf", "s/r80") + " ".join("%9s" % a[:9] for a in ARMS)
    print(hdr); print("  " + "-" * (len(hdr) - 2))
    for b in BOXES:
        row = "  %7d %6.2f | " % (b, 0.47 * b / R80)
        for arm in ARMS:
            m, n = meets("results/p10_env_v8", b, arm, usable)
            row += "%9s" % ("%d/%d" % (m, n))
        print(row)

    print("\n  %7s %6s | %26s | %26s | %s"
          % ("boxHalf", "s/r80", "v7 M1 ratio [95% CI]", "v8 M1 ratio [95% CI]", "v8 verdict"))
    for b in BOXES:
        cells = [s for s in SEEDS if (b, s) in usable]
        r7 = [tj("results/p10_env_v7", b, s, "ca_tbl_full") / tj("results/p10_env_v7", b, s, "static_2p4")
              for s in cells]
        r8 = [tj("results/p10_env_v8", b, s, "ca_tbl_full") / tj("results/p10_env_v8", b, s, "static_2p4")
              for s in cells]
        p7, l7, h7 = geo_ci(r7)
        p8, l8, h8 = geo_ci(r8)
        fm, fn_ = meets("results/p10_env_v8", b, "ca_tbl_full", usable)
        sm, sn = meets("results/p10_env_v8", b, "static_2p4", usable)
        v, _ = verdict(fm, fn_, sm, sn, l8, h8)
        print("  %7d %6.2f | %8.3f [%6.3f,%6.3f] | %8.3f [%6.3f,%6.3f] | %s"
              % (b, 0.47 * b / R80, p7, l7, h7, p8, l8, h8, v))

    print("\n=== 5-6. ENERGY, ca_tbl_full ===")
    print("  %7s %12s %12s %9s %14s %14s"
          % ("boxHalf", "v7 J", "v8 J", "delta", "v7 avail_mean", "v8 avail_mean"))
    for b in BOXES:
        cells = [s for s in SEEDS if (b, s) in usable]
        j7 = st.mean(tj("results/p10_env_v7", b, s, "ca_tbl_full") for s in cells)
        j8 = st.mean(tj("results/p10_env_v8", b, s, "ca_tbl_full") for s in cells)
        a7 = st.mean(joint("results/p10_env_v7", b, s, "ca_tbl_full") for s in cells)
        a8 = st.mean(joint("results/p10_env_v8", b, s, "ca_tbl_full") for s in cells)
        print("  %7d %12.3f %12.3f %+8.1f%% %14.4f %14.4f" % (b, j7, j8, 100 * (j8 - j7) / j7, a7, a8))


if __name__ == "__main__":
    main()
