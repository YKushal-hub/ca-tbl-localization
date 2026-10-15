#!/usr/bin/env python3
"""Score D-8 stage 1 (report/PREREG_D8_SENSITIVITY.md). Committed before the first cell.

  Probe: r80 per tier, fix off and on (PDR through 0.80, interpolated, as make_figures.py).
    Abort if fix-off does not reproduce probe_v7 (1515.3 / 1149.5 / 12.8 m) to 0.1 m.
    P1: tier-2 r80 ratio on/off in [1.4, 2.1].  P2: tiers 1 and 3 move < 1 %.
  Campaign: P3 static_2p4 A_fix rises at sparse (CI above 0); every arm's paired A_fix,
    cep50M, radio energy, escalations; static_868/static_uwb byte-identity reported; A0 v2.

Usage: d8_analysis.py <root> <outdir>
"""

import csv
import hashlib
import math
import os
import random
import sys

SEEDS = list(range(701, 721))
CFGS = ["ref", "sparse"]
ARMS = ["static_868", "static_2p4", "static_uwb", "fixed_threshold", "ca_tbl_no_integrity",
        "ca_tbl_full"]
PROBE_V7 = {"868MHz": 1515.3, "2.4GHz": 1149.5, "UWB": 12.8}
A_REQ, R_REQ, CYC, NBOOT = 0.95, 25.0, 1440, 10000
KAPPAS = [0.25, 0.5, 1.0, 2.0]
REPORT, OUT = [], []


def say(s=""):
    print(s)
    REPORT.append(s)


def r80(path):
    rows = list(csv.DictReader(open(path)))
    out = {}
    for t in sorted({r["tier"] for r in rows}):
        pts = sorted((float(r["distance_m"]), float(r["pdr"])) for r in rows if r["tier"] == t)
        for (d0, p0), (d1, p1) in zip(pts, pts[1:]):
            if p0 >= 0.8 > p1:
                out[t] = d0 + (p0 - 0.8) * (d1 - d0) / (p0 - p1)
                break
    return out


def check(d, nrows=6):
    rid = os.path.join(d, "run_id.txt")
    if not os.path.exists(rid):
        return ["no sidecar"]
    text = open(rid).read()
    probs = [] if " dirty 0 " in text else ["not dirty 0"]
    if "digest sha256" not in text:
        return probs + ["no digest"]
    for line in text.split("digest sha256", 1)[1].strip().splitlines():
        h, name = line.split(None, 1)
        p = os.path.join(d, name.strip())
        if not os.path.exists(p) or hashlib.sha256(open(p, "rb").read()).hexdigest() != h:
            probs.append("digest " + name.strip())
    return probs


def afix(path):
    ok = 0
    for r in csv.DictReader(open(path)):
        if r["hadFix"] == "1" and float(r["errM"]) <= R_REQ:
            ok += 1
    return ok / CYC


def boot(vals, rng, f=lambda v: v):
    if not vals:
        return None, None, None
    m = sum(vals) / len(vals)
    bs = sorted(sum(vals[rng.randrange(len(vals))] for _ in vals) / len(vals) for _ in range(NBOOT))
    return f(m), f(bs[int(0.025 * NBOOT)]), f(bs[int(0.975 * NBOOT) - 1])


def fm(v, f="%+.5f"):
    return "nan" if v is None else f % v


def verdict(dT, dB, N, lo, hi, k):
    if dT == 0 and dB == 0:
        return "INCONCLUSIVE (requirement unmet by any arm)"
    dD = dT - dB
    allow = min(2.0, 1.0 + k * dD / N)
    if dD > 0 and hi < allow:
        return "PASS"
    if dD <= 0 or lo > allow:
        return "FAIL"
    return "INCONCLUSIVE"


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    root, outdir = argv[1:]
    os.makedirs(outdir, exist_ok=True)
    rng = random.Random(20261019)

    say("=== probe r80")
    probe = {}
    for cond in ("off", "on"):
        d = os.path.join(root, "probe_" + cond)
        probs = check(d)
        if probs:
            say("  probe_%s unusable: %s -- stage 1 stops" % (cond, "; ".join(probs)))
            return 1
        probe[cond] = r80(os.path.join(d, "probe_range_sweep.csv"))
    repro = all(abs(probe["off"].get(t, -1) - v) <= 0.1 for t, v in PROBE_V7.items())
    say("  fix off reproduces probe_v7: %s  %s" % ("YES" if repro else "NO",
        ", ".join("%s %.1f" % (t, probe["off"].get(t, float('nan'))) for t in PROBE_V7)))
    if not repro:
        say("ABORT: no baseline (registered abort condition)")
        return 1
    for t in PROBE_V7:
        a, b = probe["off"][t], probe["on"].get(t)
        say("  %-7s off %8.1f m  on %8s  ratio %s" % (t, a, "nan" if b is None else "%.1f m" % b,
                                                      "nan" if b is None else "%.3f" % (b / a)))
        OUT.append({"metric": "r80_" + t, "cfg": "probe", "arm": "", "estimate": "" if b is None else "%.6g" % b,
                    "lo": "%.6g" % a, "hi": "", "note": "lo column = fix off"})
    ratio2 = probe["on"]["2.4GHz"] / probe["off"]["2.4GHz"]
    say("  P1 tier-2 ratio %.3f in [1.4, 2.1]: %s" % (ratio2, "HOLDS" if 1.4 <= ratio2 <= 2.1 else "FAILS"))
    p2 = all(abs(probe["on"][t] / probe["off"][t] - 1.0) < 0.01 for t in ("868MHz", "UWB"))
    say("  P2 tiers 1 and 3 move < 1 %%: %s" % ("HOLDS" if p2 else "FAILS"))

    say("\n=== campaign completeness")
    data, excluded = {}, []
    for cfg in CFGS:
        for cond in ("off", "on"):
            for s in SEEDS:
                d = os.path.join(root, "%s_%s_s%d" % (cfg, cond, s))
                probs = check(d) if os.path.isdir(d) else ["missing"]
                rows = {}
                if not probs:
                    rows = {r["arm"]: r for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv")))}
                    if sorted(rows) != sorted(ARMS):
                        probs.append("rows %d" % len(rows))
                if probs:
                    excluded.append((d, probs))
                    continue
                data[(cfg, cond, s)] = {"rows": rows, "afix": {
                    a: afix(os.path.join(d, "trace_%s_seed%d.csv" % (a, s))) for a in ARMS}}
    say("  usable %d of %d" % (len(data), 2 * 2 * len(SEEDS)))
    for d, p in excluded:
        say("  EXCLUDED %s: %s" % (d, "; ".join(p)))
    if len(excluded) > 0.10 * 4 * len(SEEDS):
        say("ABORT: more than 10% excluded")
        return 1

    for cfg in CFGS:
        paired = [s for s in SEEDS if (cfg, "off", s) in data and (cfg, "on", s) in data]
        say("\n=== %s: paired on - off, n = %d" % (cfg, len(paired)))
        for a in ARMS:
            da = [data[(cfg, "on", s)]["afix"][a] - data[(cfg, "off", s)]["afix"][a] for s in paired]
            dc = [float(data[(cfg, "on", s)]["rows"][a]["cep50M"]) - float(data[(cfg, "off", s)]["rows"][a]["cep50M"])
                  for s in paired]
            rj = [math.log(float(data[(cfg, "on", s)]["rows"][a]["totalJ"]) /
                           float(data[(cfg, "off", s)]["rows"][a]["totalJ"])) for s in paired]
            de = [int(data[(cfg, "on", s)]["rows"][a]["escalations"]) - int(data[(cfg, "off", s)]["rows"][a]["escalations"])
                  for s in paired]
            ident = sum(1 for s in paired if data[(cfg, "on", s)]["rows"][a] == data[(cfg, "off", s)]["rows"][a])
            ma, la, ha = boot(da, rng)
            mc, lc, hc = boot(dc, rng)
            gj, lj, hj = boot(rj, rng, math.exp)
            me, le, he = boot(de, rng)
            off_mean = sum(data[(cfg, "off", s)]["afix"][a] for s in paired) / len(paired)
            say("  %-20s A_fix %.4f %s [%s, %s]  cep50 %s m  radioJ x%s  esc %s  identical rows %d/%d" % (
                a, off_mean, fm(ma), fm(la), fm(ha), fm(mc, "%+.3f"), fm(gj, "%.4f"), fm(me, "%+.1f"),
                ident, len(paired)))
            OUT.append({"metric": "afix_diff", "cfg": cfg, "arm": a, "estimate": fm(ma, "%.6g"),
                        "lo": fm(la, "%.6g"), "hi": fm(ha, "%.6g"), "note": "off mean %.6g" % off_mean})
            if cfg == "sparse" and a == "static_2p4":
                say("  P3 static_2p4 A_fix rises at sparse: %s" % (
                    "HOLDS" if la is not None and la > 0 else "FAILS"))
        say("  A0 v2 (A_fix, radio energy), ca_tbl_full vs static_2p4:")
        for cond in ("off", "on"):
            ss = [s for s in SEEDS if (cfg, cond, s) in data]
            N = len(ss)
            dT = sum(1 for s in ss if data[(cfg, cond, s)]["afix"]["ca_tbl_full"] >= A_REQ)
            dB = sum(1 for s in ss if data[(cfg, cond, s)]["afix"]["static_2p4"] >= A_REQ)
            logs = [math.log(float(data[(cfg, cond, s)]["rows"]["ca_tbl_full"]["totalJ"]) /
                             float(data[(cfg, cond, s)]["rows"]["static_2p4"]["totalJ"])) for s in ss]
            g, lo, hi = boot(logs, rng, math.exp)
            v = {k: verdict(dT, dB, N, lo, hi, k) for k in KAPPAS}
            say("    fix %-3s D_T %2d/%d D_B %2d/%d rho %.3f [%.3f, %.3f] -> %s%s" % (
                cond, dT, N, dB, N, g, lo, hi, v[1.0],
                "" if len(set(v.values())) == 1 else "  band: " + ", ".join("%g:%s" % (k, v[k]) for k in KAPPAS)))
            OUT.append({"metric": "A0v2", "cfg": cfg, "arm": "fix_" + cond, "estimate": "%.6g" % g,
                        "lo": "%.6g" % lo, "hi": "%.6g" % hi,
                        "note": "D_T=%d D_B=%d N=%d verdict=%s" % (dT, dB, N, v[1.0])})

    with open(os.path.join(outdir, "d8_analysis.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=["metric", "cfg", "arm", "estimate", "lo", "hi", "note"])
        w.writeheader()
        w.writerows(OUT)
    with open(os.path.join(outdir, "d8_report.txt"), "w") as fh:
        fh.write("\n".join(REPORT) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
