#!/usr/bin/env python3
"""Score A3 (report/PREREG_A3_INTRA_CYCLE_HANDSHAKE.md). Committed before the first cell.

  P1  static arms: every arm_summary column identical off vs on (0 of 60 differing)
  P2  on: handshakePreemptions == 0 on every row, and intraCycleHandshakes > 0 summed over
      the dynamic arms (reachability of the exact zero)
  P3  tier1RangingBlocked per arm, from A3_CENSUS on each on-cell's stderr
  P4  paired per-seed cep50M and A_fix differences (on - off), dynamic arms; registered:
      ca_tbl_full's cep50M CI includes 0
Also: escalations, duty_868 (<= 0.025), radio energy ratio.

Usage: a3_handshake_analysis.py <root> <outdir>
"""

import csv
import hashlib
import math
import os
import random
import re
import sys

SEEDS = list(range(601, 621))
ARMS = ["static_868", "static_2p4", "static_uwb", "fixed_threshold", "ca_tbl_no_integrity",
        "ca_tbl_full"]
STATIC, DYNAMIC = ARMS[:3], ARMS[3:]
R_REQ, CYC, NBOOT = 25.0, 1440, 10000
CENSUS = re.compile(r"A3_CENSUS arm=(\S+) seed=(\d+) intraCycleHandshakes=(\d+) "
                    r"tier1RangingBlocked=(\d+) handshakePreemptions=(\d+)")
REPORT, OUT = [], []


def say(s=""):
    print(s)
    REPORT.append(s)


def check(d):
    rid = os.path.join(d, "run_id.txt")
    if not os.path.exists(rid):
        return ["no sidecar"]
    text = open(rid).read()
    probs = [] if " dirty 0 " in text else ["not dirty 0"]
    if "digest sha256" not in text:
        return probs + ["no digest"]
    names = []
    for line in text.split("digest sha256", 1)[1].strip().splitlines():
        h, name = line.split(None, 1)
        name = name.strip()
        names.append(name)
        p = os.path.join(d, name)
        if not os.path.exists(p) or hashlib.sha256(open(p, "rb").read()).hexdigest() != h:
            probs.append("digest " + name)
    if len([n for n in names if n.startswith("trace_")]) != 6:
        probs.append("traces not 6")
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


def fm(v, f="%+.4f"):
    return "nan" if v is None else f % v


def main(argv):
    if len(argv) != 3:
        print(__doc__)
        return 2
    root, outdir = argv[1:]
    os.makedirs(outdir, exist_ok=True)
    rng = random.Random(20261018)
    data, excluded = {}, []
    for cond in ("off", "on"):
        for s in SEEDS:
            d = os.path.join(root, "%s_s%d" % (cond, s))
            probs = check(d) if os.path.isdir(d) else ["missing"]
            rows = {}
            census = {}
            if not probs:
                rows = {r["arm"]: r for r in csv.DictReader(open(os.path.join(d, "arm_summary.csv")))}
                if sorted(rows) != sorted(ARMS):
                    probs.append("rows %d" % len(rows))
                if cond == "on":
                    err = open(d + ".stderr").read() if os.path.exists(d + ".stderr") else ""
                    for m in CENSUS.finditer(err):
                        census[m.group(1)] = tuple(int(x) for x in m.group(3, 4, 5))
                    if sorted(census) != sorted(ARMS):
                        probs.append("A3_CENSUS lines %d, not 6" % len(census))
            if probs:
                excluded.append((d, probs))
                continue
            data[(cond, s)] = {"rows": rows, "census": census,
                               "afix": {a: afix(os.path.join(d, "trace_%s_seed%d.csv" % (a, s)))
                                        for a in DYNAMIC}}
    say("=== completeness: usable %d of %d" % (len(data), 2 * len(SEEDS)))
    for d, p in excluded:
        say("  EXCLUDED %s: %s" % (d, "; ".join(p)))
    if len(excluded) > 0.10 * 2 * len(SEEDS):
        say("ABORT: more than 10% excluded")
        return 1
    paired = [s for s in SEEDS if ("off", s) in data and ("on", s) in data]

    say("\n=== P1 static arms identical off vs on")
    bad = 0
    for s in paired:
        for a in STATIC:
            if data[("off", s)]["rows"][a] != data[("on", s)]["rows"][a]:
                bad += 1
    say("  %d of %d row pairs differ -> P1 %s" % (bad, len(paired) * 3, "HOLDS" if bad == 0 else "FAILS: STOP"))

    say("\n=== P2 / P3 under the flag")
    pre = sum(int(data[("on", s)]["rows"][a]["handshakePreemptions"]) for s in paired for a in ARMS)
    intra = sum(data[("on", s)]["census"][a][0] for s in paired for a in DYNAMIC)
    say("  handshakePreemptions total %d; intraCycleHandshakes total %d -> P2 %s" % (
        pre, intra, "HOLDS" if pre == 0 and intra > 0 else "FAILS"))
    for a in DYNAMIC:
        bl = [data[("on", s)]["census"][a][1] for s in paired]
        ih = [data[("on", s)]["census"][a][0] for s in paired]
        say("  %-20s intra-cycle handshakes mean %.1f   tier-1 exchanges blocked mean %.1f" % (
            a, sum(ih) / len(ih), sum(bl) / len(bl)))
        OUT.append({"metric": "tier1RangingBlocked_mean", "arm": a, "estimate": "%.6g" % (sum(bl) / len(bl)),
                    "lo": "", "hi": "", "n": len(bl)})

    say("\n=== P4 paired differences, on - off")
    for a in DYNAMIC:
        dc = [float(data[("on", s)]["rows"][a]["cep50M"]) - float(data[("off", s)]["rows"][a]["cep50M"])
              for s in paired]
        da = [data[("on", s)]["afix"][a] - data[("off", s)]["afix"][a] for s in paired]
        de = [int(data[("on", s)]["rows"][a]["escalations"]) - int(data[("off", s)]["rows"][a]["escalations"])
              for s in paired]
        rj = [math.log(float(data[("on", s)]["rows"][a]["totalJ"]) / float(data[("off", s)]["rows"][a]["totalJ"]))
              for s in paired]
        mc, lc, hc = boot(dc, rng)
        ma, la, ha = boot(da, rng)
        me, le, he = boot(de, rng)
        gj, lj, hj = boot(rj, rng, math.exp)
        duty = max(float(data[("on", s)]["rows"][a]["duty_868"]) for s in paired)
        say("  %-20s cep50 %s [%s, %s] m   A_fix %s [%s, %s]   esc %s   radioJ x%s   max duty_868 %.5f%s" % (
            a, fm(mc), fm(lc), fm(hc), fm(ma, "%+.5f"), fm(la, "%+.5f"), fm(ha, "%+.5f"),
            fm(me, "%+.1f"), fm(gj, "%.4f"), duty, "" if duty <= 0.025 else "  OVER CAP"))
        for name, v in (("cep50_diff", (mc, lc, hc)), ("afix_diff", (ma, la, ha)),
                        ("esc_diff", (me, le, he)), ("radioJ_ratio", (gj, lj, hj))):
            OUT.append({"metric": name, "arm": a, "estimate": fm(v[0], "%.6g"), "lo": fm(v[1], "%.6g"),
                        "hi": fm(v[2], "%.6g"), "n": len(paired)})
        if a == "ca_tbl_full":
            say("  P4 (ca_tbl_full cep50 CI includes 0): %s" % (
                "HOLDS" if lc is not None and lc <= 0 <= hc else "FAILS"))

    with open(os.path.join(outdir, "a3_analysis.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=["metric", "arm", "n", "estimate", "lo", "hi"])
        w.writeheader()
        w.writerows(OUT)
    with open(os.path.join(outdir, "a3_report.txt"), "w") as fh:
        fh.write("\n".join(REPORT) + "\n")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
