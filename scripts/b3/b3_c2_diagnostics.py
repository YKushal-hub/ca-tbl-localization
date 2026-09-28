#!/usr/bin/env python3
"""B3 regeneration, C2 (PCS forms) -- SUPPLEMENTARY diagnostics, not registered.

The registered analyser, scratch/python_handoff/analyse_pcs_forms.py, is run unchanged
apart from ROOT (prereg C2). This file measures what that analyser does not print, so its
verdicts can be read correctly. It computes no verdict.

  1. The monotone control's denominator. The analyser correlates over hadFix cycles only
     (analyse_pcs_forms.py:107) and does not print how many; this does, per form x arm.
     It also counts seeds on which PCS is CONSTANT over those cycles, because Spearman is
     undefined for a constant vector: pearson() returns NaN when a standard deviation is 0
     (:70-71), the NaN propagates into the mean, and a NaN interval satisfies neither
     `lo > 0` nor `hi < 0`, so the analyser prints "indistinguishable" (:188-193) for a
     statistic that does not exist.
  2. The escalation trigger. On how many cycles is pcs < pcsThresholdUp[activeTier]?
     Thresholds as coded: legacy / weighted / gated 1.55 / 1.70 / 2.30
     (ca-tbl-policy.cc:57-59); v2 0.0 on every tier (ApplyV2Thresholds, :714-722).
     Denominators: every trace row (20 seeds x 1440 cycles = 28 800) and the hadFix rows.
     Window: the whole 3600 s run; every cycle is a complete observation, so there is no
     censoring rule to apply.
  3. Byte identity between forms and against fixed_threshold, per arm (control flow,
     rule 4): identical rows mean the form did not change a single decision.

    python3 scripts/b3/b3_c2_diagnostics.py
"""
import collections
import csv
import math
import os

ROOT = "results/regen27_pcs_forms"
FORMS = ["legacy", "v2", "weighted", "gated"]
SEEDS = list(range(1, 21))
ARMS = ["ca_tbl_full", "ca_tbl_no_integrity"]
LEG = {"868MHz": 1.55, "2.4GHz": 1.70, "UWB": 2.30}


def summary(form, seed):
    p = os.path.join(ROOT, form, "seed_%d" % seed, "arm_summary.csv")
    with open(p, newline="") as f:
        return {r["arm"]: r for r in csv.DictReader(f)}


def trace(form, seed, arm):
    p = os.path.join(ROOT, form, "seed_%d" % seed, "trace_%s_seed%d.csv" % (arm, seed))
    with open(p, newline="") as f:
        return list(csv.DictReader(f))


def section1_and_2():
    print("1-2. monotone-control denominator, PCS constancy, and trigger rate")
    print("%-9s %-20s %6s %6s %6s %12s %22s %22s %9s" % (
        "form", "arm", "rows", "hadFix", "used", "constSeeds", "trigger/all", "trigger/hadFix",
        "medTrace"))
    for form in FORMS:
        for arm in ARMS:
            n = nf = nu = nt = nft = 0
            const = 0
            ts = []
            modal = collections.Counter()
            for s in SEEDS:
                vals = set()
                for r in trace(form, s, arm):
                    n += 1
                    thr = 0.0 if form == "v2" else LEG[r["activeTier"]]
                    p = float(r["pcs"])
                    trig = p < thr
                    nt += trig
                    ts.append(float(r["traceSigma"]))
                    if r["hadFix"].strip() not in ("1", "true", "True"):
                        continue
                    nf += 1
                    nft += trig
                    e, t = float(r["errM"]), float(r["traceSigma"])
                    if math.isfinite(e) and math.isfinite(p) and math.isfinite(t):
                        nu += 1
                        vals.add(p)
                        modal[round(p, 4)] += 1
                if len(vals) <= 1:
                    const += 1
            ts.sort()
            print("%-9s %-20s %6d %6d %6d %9d/20 %6d/%-6d %.4f %6d/%-6d %.4f %9.0f" % (
                form, arm, n, nf, nu, const, nt, n, nt / n, nft, nf, nft / nf, ts[len(ts) // 2]))
            print("%31s modal PCS on used cycles: %s" % ("", modal.most_common(3)))


def rowkey(r):
    d = dict(r)
    d.pop("arm")
    return d


def section3():
    print("\n3. byte identity, seeds (of 20) on which the whole row is identical")
    for fa, fb in (("v2", "weighted"), ("weighted", "gated"), ("v2", "gated"),
                   ("legacy", "gated"), ("legacy", "v2")):
        c = collections.Counter()
        for s in SEEDS:
            a, b = summary(fa, s), summary(fb, s)
            for arm in a:
                c[arm] += a[arm] == b[arm]
        print("  %-8s vs %-8s %s" % (fa, fb, dict(sorted(c.items()))))
    print("  same form, policy arm vs fixed_threshold (arm name excluded):")
    for form in FORMS:
        for arm in ARMS:
            esc = c2 = whole = 0
            for s in SEEDS:
                R = summary(form, s)
                a, b = R[arm], R["fixed_threshold"]
                esc += a["escalations"] == b["escalations"]
                c2 += a["cycles_2p4"] == b["cycles_2p4"]
                whole += rowkey(a) == rowkey(b)
            print("    %-9s %-20s escalations equal %2d/20, cycles_2p4 equal %2d/20, whole row %2d/20"
                  % (form, arm, esc, c2, whole))
    print("  v2 vs weighted traces: columns that differ (rows, over both policy arms x 20 seeds)")
    diff = collections.Counter()
    for s in SEEDS:
        for arm in ARMS:
            ta, tb = trace("v2", s, arm), trace("weighted", s, arm)
            if len(ta) != len(tb):
                diff["__rowcount__"] += 1
                continue
            for ra, rb in zip(ta, tb):
                for k in ra:
                    if ra[k] != rb[k]:
                        diff[k] += 1
    print("   ", dict(diff))


def section4():
    print("\n4. per form x arm, mean over 20 seeds (context for the verdict lines)")
    for form in FORMS:
        for arm in ARMS:
            rs = [summary(form, s)[arm] for s in SEEDS]
            m = lambda k: sum(float(r[k]) for r in rs) / 20.0
            fx = sum(float(r["fixCycles"]) / float(r["steps"]) for r in rs) / 20.0
            print("  %-9s %-20s cep50 %7.2f m  totalJ %6.2f J  escalations %6.1f  fix/steps %.3f  suspects %6.1f"
                  % (form, arm, m("cep50M"), m("totalJ"), m("escalations"), fx, m("suspects")))


if __name__ == "__main__":
    section1_and_2()
    section3()
    section4()
