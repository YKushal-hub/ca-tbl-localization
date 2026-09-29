#!/usr/bin/env python3
"""C1 with the evidence gate off -- the registered analysis, and nothing more.

report/PREREG_C1_NOGATE.md registers this file together with its driver
(scripts/b3/c1_nogate_driver.sh) in one commit, before any cell runs. It computes, in
the registered order:

  0  completeness       40 cells, every one exactly 2 lines, digest verifying, the
                        registered invocation, the registration commit, dirty 0, no traces,
                        and a run.log whose INTEGRITY line is present in every 'on' cell and
                        absent from every 'off' cell. Any failure: refuse to pool, exit 2.
  1  population         the refusal counts from each 'on' run.log, and off/on byte identity
                        of the data row per seed. NO POPULATION if N + M = 0 on all 20
                        seeds, or if the rows are byte-identical on all 20 seeds.
  2  primary endpoints  d = on - off per seed for totalJ and rmseM: mean, median, sd, exact
                        two-sided sign test (ties excluded from n and counted on the line),
                        percentile seed bootstrap 95% CI (B = 20 000, fixed seed).
  3  verdicts           the registered rules, on seeds 1..20 (primary) and on seeds 1..8
                        (reported for comparability of the VERDICT with the original only).
  4  secondary          escalations, cycles_uwb, cycles_2p4, fixCycles, suspects, noFixCycles:
                        descriptive, no verdict.
  5  manipulation M0    'off' here (gate off) against results/regen27_integrity/off_s1..8
                        (gate on), SAME BINARY: did the flag change any row?
  6  supplementary S2   'on' here against results/regen27_integrity/{off,on}_s1..8: does the
                        witness alone reproduce what the evidence gate did? No verdict.

    python3 scripts/b3/c1_nogate_analyse.py [ROOT]

ROOT defaults to results/regen27_integrity_nogate. Run from the worktree root. Read-only.

SIGN CONVENTION. Every difference is on - off. NEGATIVE means the cross-check arm used less
energy or had lower RMSE, which is the direction the original claim predicts.
"""
import csv
import hashlib
import math
import os
import random
import re
import subprocess
import sys

ROOT = sys.argv[1] if len(sys.argv) > 1 else "results/regen27_integrity_nogate"
B3_ROOT = "results/regen27_integrity"
SEEDS = list(range(1, 21))
SEEDS_ORIG = list(range(1, 9))
B = 20000
BOOT_SEED = 20260929
ALPHA = 0.05
BIN = "./build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
REG_FILES = ["report/PREREG_C1_NOGATE.md", "scripts/b3/c1_nogate_driver.sh",
             "scripts/b3/c1_nogate_analyse.py"]
INTEG_RE = re.compile(r"INTEGRITY: escalations refused -- IMU inconsistency (\d+), "
                      r"weak corroboration (\d+)")
SHS_RE = re.compile(r"SHS: engine at .* error ([0-9.]+) m; last disagreement ([0-9.]+) m "
                    r"against tolerance ([0-9.]+) m")


def args_for(tag, s):
    a = ("--seed=%d --duration=600 --boxHalf=1000 --nFinders=4 --finderMode=1 "
         "--sendHandshakeFrames=true --trace=false --arms=ca_tbl_full "
         "--escalationRequiresEvidence=false" % s)
    if tag == "on":
        a += " --imuCrossCheck=true --corroborationCheck=true"
    return a


# ------------------------------------------------------------------ statistics --
def binom_two_sided(k, n):
    if n == 0:
        return float("nan")
    pmf = lambda i: math.comb(n, i) * (0.5 ** n)
    obs = pmf(k)
    return min(1.0, sum(pmf(i) for i in range(n + 1) if pmf(i) <= obs + 1e-12))


def sign_test(diffs):
    neg = sum(1 for d in diffs if d < 0)
    pos = sum(1 for d in diffs if d > 0)
    ties = sum(1 for d in diffs if d == 0)
    return neg, pos, ties, binom_two_sided(neg, neg + pos)


def pctile(xs, q):
    s = sorted(xs)
    k = (len(s) - 1) * q
    lo, hi = int(k), min(int(k) + 1, len(s) - 1)
    return s[lo] + (s[hi] - s[lo]) * (k - lo)


def boot_ci(diffs, rng):
    n = len(diffs)
    means = [sum(rng.choice(diffs) for _ in range(n)) / float(n) for _ in range(B)]
    return pctile(means, 0.025), pctile(means, 0.975)


def mean(xs):
    return sum(xs) / float(len(xs))


def median(xs):
    s = sorted(xs)
    n = len(s)
    return s[n // 2] if n % 2 else 0.5 * (s[n // 2 - 1] + s[n // 2])


def sd(xs):
    m = mean(xs)
    return (sum((x - m) ** 2 for x in xs) / (len(xs) - 1)) ** 0.5 if len(xs) > 1 else 0.0


# ---------------------------------------------------------------- completeness --
def head_sha():
    return subprocess.check_output(["git", "rev-parse", "HEAD"]).decode().strip()


def reg_sha():
    return subprocess.check_output(["git", "log", "-1", "--format=%H", "--"] +
                                   REG_FILES).decode().strip()


def check_cell(tag, s, reg):
    """Return (row_dict, raw_data_line, integ, shs, errors)."""
    d = os.path.join(ROOT, "%s_s%d" % (tag, s))
    errs = []
    summ = os.path.join(d, "arm_summary.csv")
    rid = os.path.join(d, "run_id.txt")
    log = os.path.join(d, "run.log")
    for p in (summ, rid, log):
        if not os.path.exists(p):
            errs.append("missing %s" % p)
    if errs:
        return None, None, None, None, errs
    lines = open(summ).read().splitlines()
    if len(lines) != 2:
        errs.append("%s has %d lines, registered 2" % (summ, len(lines)))
    traces = [f for f in os.listdir(d) if f.startswith("trace_") and f.endswith(".csv")]
    if traces:
        errs.append("%s holds %d trace files, registered 0" % (d, len(traces)))
    rl = open(rid).read().splitlines()
    if len(rl) < 3 or not rl[1].endswith(" %s %s --outdir=%s" % (BIN, args_for(tag, s), d)):
        errs.append("%s invocation line is not the registered one" % rid)
    m = re.match(r"^source ([0-9a-f]{7,40}) dirty 0 profile optimized$", rl[2] if len(rl) > 2 else "")
    if not m or not reg.startswith(m.group(1)):
        errs.append("%s source line %r is not 'source <registration commit> dirty 0 "
                    "profile optimized'" % (rid, rl[2] if len(rl) > 2 else ""))
    if "digest sha256" not in rl:
        errs.append("%s has no digest block" % rid)
    else:
        block = rl[rl.index("digest sha256") + 1:]
        names = set()
        for ln in block:
            h, _, name = ln.partition("  ")
            names.add(name)
            fp = os.path.join(d, name)
            if not os.path.exists(fp) or hashlib.sha256(open(fp, "rb").read()).hexdigest() != h:
                errs.append("%s digest line for %s does not verify" % (rid, name))
        if "arm_summary.csv" not in names:
            errs.append("%s digest block has no arm_summary.csv line" % rid)
    txt = open(log).read()
    integ = INTEG_RE.findall(txt)
    shs = SHS_RE.findall(txt)
    if tag == "on" and len(integ) != 1:
        errs.append("%s: %d INTEGRITY lines, registered exactly 1 in an 'on' cell" % (log, len(integ)))
    if tag == "off" and integ:
        errs.append("%s: INTEGRITY line in an 'off' cell -- the witness ran" % log)
    row = None
    if len(lines) == 2:
        row = next(csv.DictReader(lines))
        if row.get("arm") != "ca_tbl_full" or row.get("seed") != str(s):
            errs.append("%s row is arm %s seed %s" % (summ, row.get("arm"), row.get("seed")))
        # rmseM is 0.0 for an EMPTY error vector (ca_tbl_causal_sim.cc:6052): not a
        # measurement, so it may not enter a paired difference (rule 4).
        if float(row.get("rmseM", "0")) <= 0.0:
            errs.append("%s rmseM is 0.0 -- empty error vector, not a measurement" % summ)
    return (row, lines[1] if len(lines) == 2 else None,
            tuple(int(x) for x in integ[0]) if integ else None,
            tuple(float(x) for x in shs[0]) if shs else None, errs)


def load_b3(tag, s):
    p = os.path.join(B3_ROOT, "%s_s%d" % (tag, s), "arm_summary.csv")
    lines = open(p).read().splitlines()
    if len(lines) != 2:
        raise SystemExit("B3 cell %s has %d lines" % (p, len(lines)))
    return next(csv.DictReader(lines)), lines[1]


# ----------------------------------------------------------------------- rules --
def half_verdict(diffs, lo, hi):
    """Registered rule (prereg section 5). Returns (label, detail)."""
    neg, pos, ties, p = sign_test(diffs)
    sig = (not math.isnan(p)) and p < ALPHA
    if hi < 0 and sig and neg > pos:
        return "FAVOURS-ON", "CI entirely below 0 and sign test p < 0.05, majority negative"
    if lo > 0 or (sig and pos > neg):
        return "FAVOURS-OFF", "CI entirely above 0, or sign test p < 0.05 majority positive"
    return "NO-DETECTED-DIFFERENCE", "neither rule met"


def analyse(pairs, label, rng):
    out = {}
    print("  %s -- %d paired seeds" % (label, len(pairs)))
    for metric, unit in (("totalJ", "J"), ("rmseM", "m")):
        diffs = [float(on[metric]) - float(off[metric]) for s, off, on in pairs]
        neg, pos, ties, p = sign_test(diffs)
        lo, hi = boot_ci(diffs, rng)
        v, why = half_verdict(diffs, lo, hi)
        out[metric] = v
        print("    d%s (on - off): mean %+.4f %s  median %+.4f  sd %.4f  "
              "range [%+.4f, %+.4f]" % (metric, mean(diffs), unit, median(diffs), sd(diffs),
                                        min(diffs), max(diffs)))
        print("      95%% seed bootstrap CI [%+.4f, %+.4f] %s (B=%d, seed %d)"
              % (lo, hi, unit, B, BOOT_SEED))
        print("      sign test: %d negative / %d positive / %d exact ties, n=%d, "
              "exact two-sided p = %s" % (neg, pos, ties, neg + pos,
                                          "nan" if math.isnan(p) else "%.3g" % p))
        print("      rule: %s (%s)" % (v, why))
    e, a = out["totalJ"], out["rmseM"]
    p11 = {"FAVOURS-ON": "REPRODUCES", "FAVOURS-OFF": "SIGN FLIPS",
           "NO-DETECTED-DIFFERENCE": "NOT SUPPORTED"}[e]
    p12 = {"FAVOURS-ON": "IMPROVES (the original's form reproduces)",
           "FAVOURS-OFF": "WORSE (falsifies 'no worse')",
           "NO-DETECTED-DIFFERENCE": "NOT SHOWN WORSE (not an equivalence result)"}[a]
    if e == "FAVOURS-ON" and a != "FAVOURS-OFF":
        joint = "REPRODUCES" + (" IN FULL" if a == "FAVOURS-ON" else " (energy; accuracy not shown worse)")
    elif e == "FAVOURS-OFF" or a == "FAVOURS-OFF":
        joint = "FALSIFIED"
    else:
        joint = "NOT SUPPORTED"
    print("    P1.1 energy   : %s" % p11)
    print("    P1.2 accuracy : %s" % p12)
    print("    C1 (gate off) : %s" % joint)
    print()
    return joint


def main():
    rng = random.Random(BOOT_SEED)
    reg = reg_sha()
    print("C1 with the evidence gate off -- root %s" % ROOT)
    print("registration commit %s ; HEAD %s\n" % (reg, head_sha()))

    # 0 completeness
    errs, cells = [], {}
    for s in SEEDS:
        for tag in ("off", "on"):
            row, raw, integ, shs, e = check_cell(tag, s, reg)
            errs += e
            cells[(tag, s)] = (row, raw, integ, shs)
    print("0. COMPLETENESS")
    if errs:
        print("REFUSING TO POOL -- %d completeness failures:" % len(errs))
        for e in errs:
            print("  " + e)
        return 2
    print("  40/40 cells: 2 lines each (40 data rows, 20 per setting), digests verify,")
    print("  registered invocation, source %s dirty 0 profile optimized, 0 traces," % reg[:7])
    print("  INTEGRITY line in 20/20 'on' logs and 0/20 'off' logs\n")

    # 1 population
    print("1. POPULATION (rule 4) -- refusals per 600 s run of 240 cycles, from run.log")
    tot_n = tot_m = 0
    ident = []
    for s in SEEDS:
        n, m = cells[("on", s)][2]
        tot_n += n
        tot_m += m
        same = cells[("off", s)][1] == cells[("on", s)][1]
        if same:
            ident.append(s)
        shs = cells[("on", s)][3]
        print("  seed %2d  IMU inconsistency %3d  weak corroboration %3d  off/on row %s"
              "   SHS end error %s m" % (s, n, m, "IDENTICAL" if same else "differs  ",
                                        "%.1f" % shs[0] if shs else "n/a"))
    print("  total: IMU %d, corroboration %d over 20 runs; rows byte-identical on %d/20 seeds %s"
          % (tot_n, tot_m, len(ident), ident if ident else ""))
    no_pop = (tot_n + tot_m == 0) or (len(ident) == len(SEEDS))
    print("  POPULATION: %s\n" % ("NO POPULATION" if no_pop else "PRESENT"))

    # 2-3 primary endpoints and verdicts
    print("2-3. PRIMARY ENDPOINTS AND VERDICTS")
    pairs = [(s, cells[("off", s)][0], cells[("on", s)][0]) for s in SEEDS]
    if no_pop:
        print("  NO POPULATION -> C1 (gate off) WITHDRAWN as unsupported; the endpoints below")
        print("  are printed for the record and carry no verdict.\n")
    v20 = analyse(pairs, "PRIMARY, seeds 1..20", rng)
    v8 = analyse([p for p in pairs if p[0] in SEEDS_ORIG],
                 "COMPARABILITY OF THE VERDICT ONLY, seeds 1..8 (no magnitude comparison)", rng)
    print("  BINDING VERDICT (seeds 1..20): %s" % ("NO POPULATION" if no_pop else v20))
    print("  seeds 1..8 verdict, for comparability only: %s\n" % ("NO POPULATION" if no_pop else v8))

    # 4 secondary
    print("4. SECONDARY, descriptive (no verdict): off -> on per seed")
    cols = ("escalations", "deescalations", "cycles_868", "cycles_2p4", "cycles_uwb",
            "cyclesInUwbCoverage", "fixCycles", "noFixCycles", "suspects", "j_uwb")
    print("  seed  " + "  ".join("%-17s" % c for c in cols))
    for s, off, on in pairs:
        print("  %4d  " % s + "  ".join("%-17s" % ("%s->%s" % (off[c], on[c])) for c in cols))
    for c in ("escalations", "cycles_uwb", "fixCycles", "j_uwb"):
        d = [float(on[c]) - float(off[c]) for s, off, on in pairs]
        neg, pos, ties, p = sign_test(d)
        print("  d%-12s mean %+9.3f  %d-/%d+/%d ties" % (c, mean(d), neg, pos, ties))
    print()

    # 5 manipulation check M0
    print("5. MANIPULATION CHECK M0 -- gate OFF 'off' cells here vs gate ON 'off' cells of")
    print("   %s, seeds 1..8, same binary (control flow only)" % B3_ROOT)
    n_same = 0
    for s in SEEDS_ORIG:
        b3row, b3raw = load_b3("off", s)
        here = cells[("off", s)]
        same = b3raw == here[1]
        n_same += same
        print("  seed %d  row %s  escalations %s -> %s  cycles_uwb %s -> %s" % (
            s, "IDENTICAL" if same else "differs  ", b3row["escalations"],
            here[0]["escalations"], b3row["cycles_uwb"], here[0]["cycles_uwb"]))
    print("  M0: the flag changed the 'off' row on %d/8 seeds%s\n" % (
        8 - n_same, "  -- FLAG HAD NO EFFECT" if n_same == 8 else ""))

    # 6 supplementary S2
    print("6. SUPPLEMENTARY S2 -- 'on' here (witness, no evidence gate) vs %s, seeds 1..8" % B3_ROOT)
    for s in SEEDS_ORIG:
        _, b3off = load_b3("off", s)
        _, b3on = load_b3("on", s)
        here = cells[("on", s)][1]
        print("  seed %d  vs B3 off %s   vs B3 on %s" % (
            s, "IDENTICAL" if here == b3off else "differs", "IDENTICAL" if here == b3on else "differs"))
    return 0


if __name__ == "__main__":
    sys.exit(main())
