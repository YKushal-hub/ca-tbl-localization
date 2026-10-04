#!/usr/bin/env python3
"""B3 regeneration -- the registered procedures that have no named analyser.

report/PREREG_B3_REGEN.md names an analyser for C1 (analyse_integrity_ci.py), C2 parts 1-2
(analyse_pcs_forms.py) and C4 (k1_analyse.py). For everything else it names a PROCEDURE,
and this file is that procedure written down, nothing more:

  p23    C2 P2.3   per-seed totalJ(gated) - totalJ(weighted) on ca_tbl_no_integrity,
                   20 pairs, two-sided sign test                       (prereg C2, P2.3)
  k10    C3        paired two-sided sign test per seed on cep95M of ca_tbl_full against
                   cep95_mlat / _wcl / _prox / _lkp of the same row, and against
                   static_868 cep95M at the same seed; plus expectation E3
  avail  C5        fixCycles/steps on ca_tbl_full per seed, mean over 20 seeds, and a
                   paired sign test wanmesh_move vs wanonly_move
  edge   C6        P6.1 byte-identity of the accuracy columns, P6.2 totalJ lower under edge
  imu    C7        paired sign test on cep95M, IMU ceiling against its C3 control

    python3 scripts/b3/b3_regen_analyse.py <p23|k10|avail|edge|imu|all>

Run from the worktree root. Read-only on results/.

COMPLETENESS BEFORE POOLING (rule 12). A K10/IMU cell is the concatenation of its four
chunk CSVs, header once, and must count exactly 120 data rows -- 20 seeds x 6 arms, each
arm holding seeds 7..26 exactly once -- or the cell is refused. A K1/EDGE cell is one file
of 120 data rows. This concatenation is the only transformation applied (prereg section 3).

SIGN CONVENTION. Every paired difference is printed as (comparator - subject), so a
POSITIVE difference means the SUBJECT is better (lower error, lower energy). The subject is
named on every line.

SIGN TEST. Exact two-sided binomial, p = 0.5, ties (exact zero differences) excluded from n
and COUNTED ON THE LINE, because an exact zero is a control-flow claim (rule 4), not a
measurement, and must be visible.

EXACT ZEROS IN THE RIVAL COLUMNS. The simulator's Percentile() returns 0.0 for an EMPTY
error vector (ca_tbl_causal_sim.cc:1383-1386), and a rival estimator pushes an error only
once it has produced a first estimate (:5934-5993). So cep95_<rival> == 0.0 means "never
produced an estimate", not "perfect". Such rows are counted and printed before any test,
and the test is reported both as registered and with those seeds removed.
"""
import csv
import math
import os
import sys

SEEDS = list(range(7, 27))
CHUNKS = (7, 12, 17, 22)
ARMS = ["static_868", "static_2p4", "static_uwb", "fixed_threshold",
        "ca_tbl_no_integrity", "ca_tbl_full"]
ALPHA = 0.05


# ------------------------------------------------------------------ statistics --
def binom_two_sided(k, n):
    if n == 0:
        return 1.0
    pmf = lambda i: math.comb(n, i) * (0.5 ** n)
    obs = pmf(k)
    return min(1.0, sum(pmf(i) for i in range(n + 1) if pmf(i) <= obs + 1e-12))


def sign_test(diffs):
    pos = sum(1 for d in diffs if d > 0)
    neg = sum(1 for d in diffs if d < 0)
    ties = sum(1 for d in diffs if d == 0)
    return pos, neg, ties, binom_two_sided(pos, pos + neg)


def mean(xs):
    return sum(xs) / len(xs) if xs else float("nan")


def median(xs):
    s = sorted(xs)
    n = len(s)
    if n == 0:
        return float("nan")
    return s[n // 2] if n % 2 else 0.5 * (s[n // 2 - 1] + s[n // 2])


def fmt_test(diffs):
    pos, neg, ties, p = sign_test(diffs)
    return "%2d+/%2d- ties %2d  p=%.4f  mean %+10.2f  median %+10.2f" % (
        pos, neg, ties, p, mean(diffs), median(diffs))


# ------------------------------------------------------------------ cell load --
def read_rows(path):
    with open(path, newline="") as f:
        rdr = csv.DictReader(f)
        return rdr.fieldnames, list(rdr)


def index_cell(rows, where):
    by = {}
    for r in rows:
        a, s = r["arm"], int(r["seed"])
        if s in by.setdefault(a, {}):
            sys.exit("REFUSING: %s has arm %s seed %d twice" % (where, a, s))
        by[a][s] = r
    for a in ARMS:
        if sorted(by.get(a, {})) != SEEDS:
            sys.exit("REFUSING: %s arm %s holds seeds %s, expected 7..26"
                     % (where, a, sorted(by.get(a, {}))))
    return by


def load_chunked(prefix):
    """prefix like results/regen27_k10/wanmesh_move -> concatenation of _s7.._s22."""
    header, allrows = None, []
    for c in CHUNKS:
        p = "%s_s%d/arm_summary.csv" % (prefix, c)
        if not os.path.isfile(p):
            sys.exit("REFUSING: missing %s" % p)
        h, rows = read_rows(p)
        if len(rows) != 30:
            sys.exit("REFUSING: %s has %d data rows, expected 30" % (p, len(rows)))
        if header is None:
            header = h
        elif h != header:
            sys.exit("REFUSING: header of %s differs from the first chunk" % p)
        allrows.extend(rows)
    if len(allrows) != 120:
        sys.exit("REFUSING: %s concatenates to %d rows, expected 120" % (prefix, len(allrows)))
    return index_cell(allrows, prefix)


def load_single(path):
    p = os.path.join(path, "arm_summary.csv")
    _, rows = read_rows(p)
    if len(rows) != 120:
        sys.exit("REFUSING: %s has %d data rows, expected 120" % (p, len(rows)))
    return index_cell(rows, path)


def f(r, k):
    return float(r[k])


# ------------------------------------------------------------------------ C2 --
def c2_p23():
    print("C2 P2.3 -- totalJ(gated) - totalJ(weighted), ca_tbl_no_integrity, seeds 1..20")
    print("registered: two-sided sign test p < 0.05 with a majority NEGATIVE (gated cheaper)")
    root = "results/regen27_pcs_forms"
    d = []
    for s in range(1, 21):
        rows = {}
        for form in ("gated", "weighted"):
            _, rr = read_rows(os.path.join(root, form, "seed_%d" % s, "arm_summary.csv"))
            if len(rr) != 6:
                sys.exit("REFUSING: %s/seed_%d has %d rows" % (form, s, len(rr)))
            rows[form] = {r["arm"]: r for r in rr}
        d.append(f(rows["gated"]["ca_tbl_no_integrity"], "totalJ") -
                 f(rows["weighted"]["ca_tbl_no_integrity"], "totalJ"))
    pos, neg, ties, p = sign_test(d)
    # printed here as subject - comparator, because the registration phrases it that way
    print("  gated - weighted: %d negative / %d positive / %d exact zero, two-sided p=%.4f"
          % (neg, pos, ties, p))
    print("  mean %+.3f J, median %+.3f J" % (mean(d), median(d)))
    print("  per seed: " + " ".join("%+.2f" % x for x in d))
    sig = p < ALPHA
    if sig and neg > pos:
        v = "REPRODUCES"
    elif sig and pos > neg:
        v = "SIGN FLIPS"
    else:
        v = "WITHDRAWN (not significant)"
    print("  P2.3 verdict under the registered rule: %s" % v)


# ------------------------------------------------------------------------ C3 --
K10_CELLS = ["wanonly_move", "sparse_move", "wanmesh_move"]
RIVALS = ["mlat", "wcl", "prox", "lkp"]


def c3_k10():
    print("C3 K10 -- subject ca_tbl_full, cep95 (m), 20 paired seeds (7..26) per cell,")
    print("14 400 s per seed, cep95 over every cycle of the run for the EKF and over every")
    print("cycle from the rival's FIRST estimate onward for a rival (it holds its last")
    print("estimate when it cannot solve; it is not charged before it has one).")
    print("positive difference = ca_tbl_full better")
    out = {}
    for cell in K10_CELLS:
        by = load_chunked("results/regen27_k10/%s" % cell)
        full = by["ca_tbl_full"]
        print("\n### %s  (120 rows, complete)" % cell)
        print("  means over 20 seeds: ca_tbl_full %.0f | static_868 %.0f | " % (
            mean([f(full[s], "cep95M") for s in SEEDS]),
            mean([f(by["static_868"][s], "cep95M") for s in SEEDS])) +
              " | ".join("%s %.0f" % (rv.upper(), mean([f(full[s], "cep95_" + rv) for s in SEEDS]))
                         for rv in RIVALS))
        print("  medians:             ca_tbl_full %.0f | static_868 %.0f | " % (
            median([f(full[s], "cep95M") for s in SEEDS]),
            median([f(by["static_868"][s], "cep95M") for s in SEEDS])) +
              " | ".join("%s %.0f" % (rv.upper(), median([f(full[s], "cep95_" + rv) for s in SEEDS]))
                         for rv in RIVALS))
        zero_mlat = [s for s in SEEDS if f(full[s], "mlat_solved") == 0]
        print("  ca_tbl_full rows with mlat_solved == 0: %d %s" % (len(zero_mlat), zero_mlat))
        res = {}
        for rv in RIVALS:
            zs = [s for s in SEEDS if f(full[s], "cep95_" + rv) == 0.0]
            d = [f(full[s], "cep95_" + rv) - f(full[s], "cep95M") for s in SEEDS]
            line = "  vs %-10s %s" % (rv.upper(), fmt_test(d))
            if zs:
                d2 = [f(full[s], "cep95_" + rv) - f(full[s], "cep95M") for s in SEEDS if s not in zs]
                line += "\n     %d seed(s) with cep95_%s == 0.0 (never estimated): %s; without them: %s" % (
                    len(zs), rv, zs, fmt_test(d2))
            print(line)
            res[rv] = sign_test(d)
        d = [f(by["static_868"][s], "cep95M") - f(full[s], "cep95M") for s in SEEDS]
        print("  vs %-10s %s" % ("static_868", fmt_test(d)))
        res["static_868"] = sign_test(d)
        out[cell] = res
    print("\n### P3.1 (MLAT beaten, p < 0.05, in every moving cell)")
    for cell in K10_CELLS:
        pos, neg, ties, p = out[cell]["mlat"]
        ok = p < ALPHA and pos > neg
        print("  %-14s %2d+/%2d- p=%.4f  %s" % (cell, pos, neg, p,
              "beats MLAT" if ok else ("LOSES to MLAT" if p < ALPHA else "not significant")))
    print("### P3.2 (not significantly different from static_868 in any moving cell)")
    for cell in K10_CELLS:
        pos, neg, ties, p = out[cell]["static_868"]
        print("  %-14s %2d+/%2d- ties %d p=%.4f  %s" % (cell, pos, neg, ties, p,
              "indistinguishable" if p >= ALPHA else
              ("ca_tbl_full BETTER" if pos > neg else "ca_tbl_full WORSE")))

    # E3 -- byte identity against results/L1_imuoff_s*, control flow (prereg section 1)
    print("\n### E3 -- regen27_k10/wanmesh_move_s{7,12,17,22} vs results/L1_imuoff_s{...}")
    print("registered as a test on seeds 8..26 only; seed 7 was seen in the timing probe")
    tot_same = tot = 0
    per_seed = {}
    for c in CHUNKS:
        a = "results/regen27_k10/wanmesh_move_s%d/arm_summary.csv" % c
        b = "results/L1_imuoff_s%d/arm_summary.csv" % c
        if not os.path.isfile(b):
            print("  %s missing" % b)
            continue
        la = open(a).read().splitlines()
        lb = open(b).read().splitlines()
        print("  chunk s%-2d  file byte-identical: %s  (%d vs %d lines, header equal: %s)"
              % (c, open(a, "rb").read() == open(b, "rb").read(), len(la), len(lb), la[0] == lb[0]))
        for x, y in zip(la[1:], lb[1:]):
            s = int(x.split(",")[1])
            per_seed.setdefault(s, [0, 0])
            per_seed[s][1] += 1
            if x == y:
                per_seed[s][0] += 1
    diff_seeds = [s for s in sorted(per_seed) if per_seed[s][0] != per_seed[s][1]]
    test_seeds = [s for s in sorted(per_seed) if s != 7]
    same_test = sum(1 for s in test_seeds if per_seed[s][0] == per_seed[s][1])
    print("  seeds whose 6 rows are all byte-identical: %d of %d tested seeds (8..26); seed 7: %s"
          % (same_test, len(test_seeds), per_seed.get(7)))
    print("  seeds with any differing row: %s" % diff_seeds)


# ------------------------------------------------------------------------ C5 --
def c5_avail():
    print("C5 AVAILABILITY -- fixCycles/steps on ca_tbl_full, per seed (7..26)")
    print("denominator: steps (every 2.5 s cycle of the 14 400 s run, 5 760 per seed);")
    print("numerator: cycles on which the node produced a fix. A per-cycle ratio: no event")
    print("straddles the window, so there is no censoring rule to apply.")
    av = {}
    for cell in K10_CELLS:
        by = load_chunked("results/regen27_k10/%s" % cell)
        full = by["ca_tbl_full"]
        steps = set(f(full[s], "steps") for s in SEEDS)
        av[cell] = {s: f(full[s], "fixCycles") / f(full[s], "steps") for s in SEEDS}
        print("  %-14s mean %.4f  median %.4f  min %.4f  max %.4f  steps per seed %s"
              % (cell, mean(list(av[cell].values())), median(list(av[cell].values())),
                 min(av[cell].values()), max(av[cell].values()), sorted(steps)))
    m = [mean(list(av[c].values())) for c in K10_CELLS]
    print("  ordering WAN only < sparse < WAN+mesh on the mean: %s" % (m[0] < m[1] < m[2]))
    d = [av["wanmesh_move"][s] - av["wanonly_move"][s] for s in SEEDS]
    pos, neg, ties, p = sign_test(d)
    print("  wanmesh_move - wanonly_move: %d+/%d- ties %d  two-sided p=%.6f  mean %+.4f"
          % (pos, neg, ties, p, mean(d)))
    d2 = [av["sparse_move"][s] - av["wanonly_move"][s] for s in SEEDS]
    d3 = [av["wanmesh_move"][s] - av["sparse_move"][s] for s in SEEDS]
    print("  (reported, not registered) sparse - wanonly: %s" % fmt_test(d2))
    print("  (reported, not registered) wanmesh - sparse: %s" % fmt_test(d3))
    print("  all arms, mean fixCycles/steps (reported so the claim arm is not read alone):")
    for cell in K10_CELLS:
        by = load_chunked("results/regen27_k10/%s" % cell)
        print("    %-14s " % cell + "  ".join(
            "%s %.4f" % (a, mean([f(by[a][s], "fixCycles") / f(by[a][s], "steps") for s in SEEDS]))
            for a in ARMS))


# ------------------------------------------------------------------------ C6 --
ACC_COLS = ["rmseM", "cep50M", "cep95M", "maxErrM", "rmseFixedM", "cep50FixedM", "fixCycles"]


def c6_edge():
    print("C6 EDGE -- results/regen27_edge/{nodeside,edge}, 20 seeds (7..26) x 6 arms, 3600 s")
    node = load_single("results/regen27_edge/nodeside")
    edge = load_single("results/regen27_edge/edge")
    print("\n  P6.2 -- totalJ, positive difference = edge cheaper (nodeside - edge)")
    energy_moved = {}
    for a in ARMS:
        d = [f(node[a][s], "totalJ") - f(edge[a][s], "totalJ") for s in SEEDS]
        frac = [1.0 - f(edge[a][s], "totalJ") / f(node[a][s], "totalJ") for s in SEEDS]
        pos, neg, ties, p = sign_test(d)
        energy_moved[a] = pos + neg > 0
        print("  %-20s %s  saving %.1f%% mean [min %.1f%%, max %.1f%%]  lower under edge in %d/20"
              % (a, fmt_test(d), 100 * mean(frac), 100 * min(frac), 100 * max(frac), pos))
    print("\n  P6.1 -- accuracy columns byte-identical per seed x arm (string compare)")
    for a in ARMS:
        n_same = 0
        bad = []
        for s in SEEDS:
            same = all(node[a][s][c] == edge[a][s][c] for c in ACC_COLS)
            n_same += same
            if not same:
                bad.append((s, [c for c in ACC_COLS if node[a][s][c] != edge[a][s][c]]))
        whole = sum(1 for s in SEEDS if all(node[a][s][k] == edge[a][s][k]
                                             for k in node[a][s] if k not in ("totalJ",)))
        print("  %-20s accuracy identical %2d/20  energy path exercised: %s  %s"
              % (a, n_same, "yes" if energy_moved[a] else "NO -- P6.1 not claimed on this arm",
                 ("differs: %s" % bad[:3]) if bad else ""))
    print("\n  columns that differ anywhere between nodeside and edge (count of seed x arm rows):")
    cnt = {}
    for a in ARMS:
        for s in SEEDS:
            for k in node[a][s]:
                if node[a][s][k] != edge[a][s][k]:
                    cnt[k] = cnt.get(k, 0) + 1
    print("   ", cnt)


# ------------------------------------------------------------------------ C7 --
def c7_imu():
    print("C7 IMU CEILING -- results/regen27_imu/on_s* against its control")
    print("results/regen27_k10/wanmesh_move_s* (same invocation minus --imuFusion=true")
    print("--imuIdeal=true). A CEILING: an ideal dead reckoner, never a system result.")
    print("positive difference = cep95 lower under the ceiling (control - ceiling)")
    on = load_chunked("results/regen27_imu/on")
    off = load_chunked("results/regen27_k10/wanmesh_move")
    for a in ARMS:
        d = [f(off[a][s], "cep95M") - f(on[a][s], "cep95M") for s in SEEDS]
        print("  %-20s control mean %8.0f med %8.0f | ceiling mean %8.0f med %8.0f | %s" % (
            a, mean([f(off[a][s], "cep95M") for s in SEEDS]), median([f(off[a][s], "cep95M") for s in SEEDS]),
            mean([f(on[a][s], "cep95M") for s in SEEDS]), median([f(on[a][s], "cep95M") for s in SEEDS]),
            fmt_test(d)))
    print("\n  unused-quantity check (rule 3): did the measurement stream move? per arm,")
    print("  seeds on which fixCycles / mac_tx / totalJ are byte-identical control vs ceiling")
    for a in ARMS:
        print("  %-20s fixCycles %2d/20  mac_tx %2d/20  totalJ %2d/20  mean fix/steps control %.4f ceiling %.4f" % (
            a,
            sum(off[a][s]["fixCycles"] == on[a][s]["fixCycles"] for s in SEEDS),
            sum(off[a][s]["mac_tx"] == on[a][s]["mac_tx"] for s in SEEDS),
            sum(off[a][s]["totalJ"] == on[a][s]["totalJ"] for s in SEEDS),
            mean([f(off[a][s], "fixCycles") / f(off[a][s], "steps") for s in SEEDS]),
            mean([f(on[a][s], "fixCycles") / f(on[a][s], "steps") for s in SEEDS])))


def main():
    what = sys.argv[1] if len(sys.argv) > 1 else "all"
    table = {"p23": c2_p23, "k10": c3_k10, "avail": c5_avail, "edge": c6_edge, "imu": c7_imu}
    todo = list(table) if what == "all" else [what]
    for w in todo:
        if w not in table:
            sys.exit("unknown section %s" % w)
        print("=" * 100)
        table[w]()
        print()
    return 0


if __name__ == "__main__":
    sys.exit(main())
