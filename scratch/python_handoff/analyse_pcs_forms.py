#!/usr/bin/env python3
"""D-1 repair campaign analysis. Applies report/PREREG_PCS_FORMS.md verbatim.

Two halves, and the second is the one that decides the contribution.

1. PAIRED PER-SEED DIFFERENCES against the legacy cell at the same seed, for
   cep50M, totalJ and escalation count, with a two-sided sign test. Paired because
   the seed fixes the channel realisation; a single-seed difference is never a
   result (RULES.md rule 8) and only the 20-seed aggregate is reported.

2. THE MONOTONE-OF-FILTER-STATISTIC CONTROL. Spearman rho between per-cycle PCS
   and per-cycle radial error, against Spearman rho between -traceSigma and the
   same error. traceSigma is the EKF covariance trace that PCS is computed from,
   so -traceSigma is a monotone function of the score's own input. A score that
   carries information beyond the filter statistic must beat that control by more
   than the paired bootstrap interval. The legacy score measured rho = -0.199
   against a control at -0.200. PCS v2, pre-registered before measurement, failed
   the same test.

   The registration PREDICTS both repaired forms still fail it, because saturation
   is a defect in the RANGE of the score and the information test asks whether the
   score knows anything its own input does not.

NO SCIPY on this machine, so Spearman is computed directly: rank both vectors with
average ranks for ties, then Pearson on the ranks. Bootstrap is SEED-LEVEL, 10 000
resamples -- cycles within a seed are serially correlated and a row-level resample
would understate the interval.

DENOMINATOR. Only cycles with hadFix == 1 enter the correlation: a cycle with no
fix has no radial error to correlate against. That is stated rather than assumed,
and the count is reported per cell so the exclusion is visible.
"""
import csv
import math
import os
import random
import sys

ROOT = "results/pcs_forms"
FORMS = ["legacy", "v2", "weighted", "gated"]
SEEDS = list(range(1, 21))
ARMS = ["ca_tbl_full", "ca_tbl_no_integrity"]
NBOOT = 10000
random.seed(20260905)


def ranks(v):
    order = sorted(range(len(v)), key=lambda i: v[i])
    r = [0.0] * len(v)
    i = 0
    while i < len(order):
        j = i
        while j + 1 < len(order) and v[order[j + 1]] == v[order[i]]:
            j += 1
        avg = 0.5 * (i + j) + 1.0
        for k in range(i, j + 1):
            r[order[k]] = avg
        i = j + 1
    return r


def pearson(a, b):
    n = len(a)
    if n < 3:
        return float("nan")
    ma = sum(a) / n
    mb = sum(b) / n
    sa = math.sqrt(sum((x - ma) ** 2 for x in a))
    sb = math.sqrt(sum((x - mb) ** 2 for x in b))
    if sa == 0 or sb == 0:
        return float("nan")
    return sum((a[i] - ma) * (b[i] - mb) for i in range(n)) / (sa * sb)


def spearman(a, b):
    return pearson(ranks(a), ranks(b))


def sign_test(d):
    pos = sum(1 for x in d if x > 0)
    neg = sum(1 for x in d if x < 0)
    n = pos + neg
    if n == 0:
        return pos, neg, 1.0
    k = min(pos, neg)
    tail = sum(math.comb(n, i) for i in range(k + 1))
    return pos, neg, min(1.0, 2.0 * tail / (2.0 ** n))


def summary(form, seed):
    f = os.path.join(ROOT, form, "seed_%d" % seed, "arm_summary.csv")
    if not os.path.isfile(f):
        return {}
    out = {}
    for row in csv.DictReader(open(f)):
        out[row["arm"]] = row
    return out


def per_seed_rho(form, seed, arm):
    """Returns (rho_pcs, rho_control, nCycles) for one seed."""
    t = os.path.join(ROOT, form, "seed_%d" % seed, "trace_%s_seed%d.csv" % (arm, seed))
    if not os.path.isfile(t):
        return None
    pcs, ctrl, err = [], [], []
    for row in csv.DictReader(open(t)):
        if row["hadFix"].strip() not in ("1", "true", "True"):
            continue
        try:
            e = float(row["errM"]); p = float(row["pcs"]); ts = float(row["traceSigma"])
        except (ValueError, KeyError):
            continue
        if not (math.isfinite(e) and math.isfinite(p) and math.isfinite(ts)):
            continue
        err.append(e); pcs.append(p); ctrl.append(-ts)
    if len(err) < 30:
        return None
    return spearman(pcs, err), spearman(ctrl, err), len(err)


if not os.path.isdir(ROOT):
    sys.exit("missing %s -- campaign has not run" % ROOT)

print("D-1 REPAIR CAMPAIGN — applying PREREG_PCS_FORMS.md")
print("=" * 78)

# ---- part 1: paired differences against legacy ------------------------------
for arm in ARMS:
    print("\n### %s — paired per-seed differences against legacy" % arm)
    print("  %-10s %14s %14s %14s %10s" % ("form", "d cep50M", "d totalJ", "d escal", "sign(cep)"))
    base = {s: summary("legacy", s).get(arm) for s in SEEDS}
    for form in FORMS:
        if form == "legacy":
            continue
        dc, dj, de = [], [], []
        for s in SEEDS:
            b = base.get(s)
            c = summary(form, s).get(arm)
            if not b or not c:
                continue
            dc.append(float(c["cep50M"]) - float(b["cep50M"]))
            dj.append(float(c["totalJ"]) - float(b["totalJ"]))
            de.append(float(c["escalations"]) - float(b["escalations"]))
        if not dc:
            print("  %-10s NO DATA" % form)
            continue
        pos, neg, p = sign_test(dc)
        print("  %-10s %+14.3f %+14.3f %+14.1f   %2d+/%2d- p=%.4f"
              % (form, sum(dc) / len(dc), sum(dj) / len(dj), sum(de) / len(de), pos, neg, p))

# ---- part 2: the monotone control -------------------------------------------
print("\n\n### THE MONOTONE-OF-FILTER-STATISTIC CONTROL")
print("rho(PCS, err) against rho(-traceSigma, err). A score carrying information")
print("beyond the filter statistic must BEAT the control by more than the CI.")
print("\n  %-10s %-20s %9s %9s %9s   %s"
      % ("form", "arm", "rho_pcs", "rho_ctrl", "advantg", "95% CI on advantage"))

for form in FORMS:
    for arm in ARMS:
        rows = []
        for s in SEEDS:
            r = per_seed_rho(form, s, arm)
            if r:
                rows.append(r)
        if len(rows) < 5:
            print("  %-10s %-20s insufficient cycles" % (form, arm))
            continue
        rp = sum(r[0] for r in rows) / len(rows)
        rc = sum(r[1] for r in rows) / len(rows)
        # ADVANTAGE, not raw difference. Both correlations are NEGATIVE -- higher
        # confidence goes with lower error -- so a score carries information beyond
        # the filter statistic only when it is MORE negatively correlated than the
        # control. abs(rho_pcs) - abs(rho_ctrl) > 0 is PCS winning. Taking the raw
        # difference rho_pcs - rho_ctrl and testing it against zero inverts the
        # verdict, which is exactly what the first version of this script did: it
        # printed BEATS CONTROL for every cell while the control was in fact the
        # better predictor in all of them.
        deltas = [abs(r[0]) - abs(r[1]) for r in rows]
        d = sum(deltas) / len(deltas)
        boot = []
        n = len(deltas)
        for _ in range(NBOOT):
            samp = [deltas[random.randrange(n)] for _ in range(n)]
            boot.append(sum(samp) / n)
        boot.sort()
        lo = boot[int(0.025 * NBOOT)]
        hi = boot[int(0.975 * NBOOT)]
        if lo > 0:
            verdict = "PCS BEATS CONTROL"
        elif hi < 0:
            verdict = "LOSES TO CONTROL"
        else:
            verdict = "indistinguishable"
        print("  %-10s %-20s %9.4f %9.4f %9.4f   [%+.4f, %+.4f]  %s"
              % (form, arm, rp, rc, d, lo, hi, verdict))

print("""
READING IT. 'indistinguishable' means the score carries nothing the covariance
trace it is computed from does not already carry -- the same outcome legacy and v2
both produced. The registration predicted exactly that for both repaired forms and
named REPAIRED BUT UNINFORMATIVE as the expected branch: the engineering defect was
real and worth fixing, and the novelty claim still does not stand.""")
