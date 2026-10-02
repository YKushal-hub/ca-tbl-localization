#!/usr/bin/env python3
"""G35: the uplink of a tier-2/3 range to the gateway estimator is billed
(docs/SPEC_CONFORMANCE_PLAN.md SY-1, --edgeUplinkRanges; TECH_DEBT L.2.3).

With the estimator on the gateways (the default since 2026-09-29), a range measured by
two-way time of flight on 2.4 GHz or UWB is left at the node, and the frame that would
carry it to the gateways was never billed. The flag bills one frame per cycle with at least
one such range, on the tier that ranged: 9 header bytes plus 6 per answered anchor.

Reads a directory written by scripts/edge_uplink_gate.sh (def, off, on: all six arms, seed
7, 900 s; r_node). Every check runs and is reported; exit 1 if any failed, 2 if an input is
missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    --edgeUplinkRanges=false IS the default: every file byte-identical, and no SY-1
          line under the default.
  ONLY    only energy moves. arm_summary.csv: every column outside {totalJ,
          accuracyPerJoule} and the 2.4 GHz / UWB {j_, tx_, airtime_} is byte-identical
          on every row, so 868 MHz, receive, idle, sleep, CAD, accuracy and MAC columns do
          not move. Every trace column but cumJ is byte-identical on every row.
  TIERS   the bill only ADDS transmission on tiers 2 and 3: their tx_ and airtime_ never
          fall; and RULES.md sect 6 -- a row whose tier never transmitted in the default
          run (tx_ == 0) has that tier's columns identical under the flag.
  REACH   the uplink runs where ranges exist: static_2p4's 2.4 GHz tx_ rises, and the SY-1
          line reports frames > 0 for it. Rows whose arm never ranged on 2.4 GHz or UWB do
          not move at all. The UWB branch is NOT exercised: no regime known today returns a
          UWB range to the measured node (the DC-6 finding, TECH_DEBT L.2), so its frames
          are 0 here; it differs from the 2.4 GHz branch only in the airtime formula.
  POWER   the bill is a transmission at the radio's own power: per arm and tier, the extra
          tx_ joules divided by the extra airtime_ seconds equals the same constant on
          every row that moved (one TX power per radio), to 1e-6 relative; and the extra
          airtime_ equals the SY-1 line's airtime for that tier and arm, to 1e-6 s.
  R       the flag with the estimator on the node is refused (exit 1).
"""
import csv
import os
import re
import sys

MOV_EXACT = {"totalJ", "accuracyPerJoule"}
MOV_TIER = {"j_2p4", "j_uwb", "tx_2p4", "tx_uwb", "airtime_2p4", "airtime_uwb"}
LINE = re.compile(r"SY-1 edge uplink: 2\.4 GHz frames (\d+) records (\d+) airtime ([0-9.eE+-]+) s; "
                  r"UWB frames (\d+) records (\d+) airtime ([0-9.eE+-]+) s")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def summary(d, n):
    return list(csv.DictReader(open(os.path.join(d, n, "arm_summary.csv"))))


def arm_lines(d, n):
    """Per arm, the SY-1 line's numbers. Each arm prints its diagnostic lines BEFORE its
    row of the stdout table, and arms run in arm_summary.csv's row order, so the k-th
    SY-1 line belongs to the k-th row."""
    vals = [tuple(float(x) for x in m.groups()) for m in LINE.finditer(log(d, n))]
    arms = [r["arm"] for r in summary(d, n)]
    if len(vals) != len(arms):
        raise ValueError(f"{len(vals)} SY-1 lines for {len(arms)} arms")
    return dict(zip(arms, vals))


def main():
    if len(sys.argv) != 2:
        print("usage: edge_uplink_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def c_runs():
        bad = [n for n in ("def", "off", "on") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fo = sorted(f for f in os.listdir(os.path.join(d, "off")) if f != "run_id.txt")
        same = fd == fo and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "off", f), "rb").read() for f in fd)
        lines = log(d, "def").count("SY-1")
        check("NULL", same and fd and lines == 0,
              f"--edgeUplinkRanges=false vs default: {len(fd)} files, "
              f"{'identical' if same else 'DIFFER'}; SY-1 lines under the default {lines} (need 0)")
    safe("NULL", c_null)

    def c_only():
        a, b = summary(d, "def"), summary(d, "on")
        bad = []
        if [r["arm"] for r in a] != [r["arm"] for r in b] or not a:
            bad.append("row sets differ")
        for ra, rb in zip(a, b):
            for k in ra:
                if k in MOV_EXACT or k in MOV_TIER:
                    continue
                if ra[k] != rb[k]:
                    bad.append((ra["arm"], k))
        tbad = []
        traces = sorted(f for f in os.listdir(os.path.join(d, "def")) if f.startswith("trace_"))
        for f in traces:
            ta = list(csv.DictReader(open(os.path.join(d, "def", f))))
            tb = list(csv.DictReader(open(os.path.join(d, "on", f))))
            if len(ta) != len(tb):
                tbad.append((f, "rows"))
                continue
            for x, y in zip(ta, tb):
                for k in x:
                    if k != "cumJ" and x[k] != y[k]:
                        tbad.append((f, x["t"], k))
                        break
        check("ONLY", not bad and not tbad and traces,
              f"arm_summary: {len(a)} rows, non-energy cells moved {len(bad)} (need 0)"
              f"{': ' + str(bad[:3]) if bad else ''}; traces {len(traces)}, rows with a "
              f"non-cumJ cell moved {len(tbad)} (need 0){': ' + str(tbad[:3]) if tbad else ''}")
    safe("ONLY", c_only)

    def c_tiers():
        a, b = summary(d, "def"), summary(d, "on")
        bad = []
        for ra, rb in zip(a, b):
            for t in ("2p4", "uwb"):
                for k in ("tx_", "airtime_"):
                    if float(rb[k + t]) < float(ra[k + t]):
                        bad.append((ra["arm"], k + t, "fell"))
            # sect 6: a tier that never transmitted in the default run (tx_ == 0: it never
            # ranged, so it never had a range to send up) is not charged by the uplink.
            for t in ("2p4", "uwb"):
                if float(ra["tx_" + t]) == 0.0 and any(ra[k + t] != rb[k + t] for k in ("j_", "tx_", "airtime_")):
                    bad.append((ra["arm"], t, "charged a tier that never transmitted"))
        check("TIERS", not bad and a,
              f"{len(a)} rows; tier-2/3 tx_/airtime_ falling, or a silent tier charged: "
              f"{len(bad)} (need 0){': ' + str(bad[:3]) if bad else ''}")
    safe("TIERS", c_tiers)

    def c_reach():
        a, b = summary(d, "def"), summary(d, "on")
        moved = {ra["arm"]: ra["totalJ"] != rb["totalJ"] for ra, rb in zip(a, b)}
        s2 = next(i for i, r in enumerate(a) if r["arm"] == "static_2p4")
        rose = float(b[s2]["tx_2p4"]) > float(a[s2]["tx_2p4"])
        ln = arm_lines(d, "on")
        f2 = ln.get("static_2p4", (0,))[0]
        quiet = [ra["arm"] for ra, rb in zip(a, b)
                 if float(ra["tx_2p4"]) == 0.0 and float(ra["tx_uwb"]) == 0.0 and moved[ra["arm"]]]
        check("REACH", rose and f2 > 0 and not quiet,
              f"static_2p4 tx_2p4 {a[s2]['tx_2p4']} -> {b[s2]['tx_2p4']} (need a rise), SY-1 "
              f"frames {int(f2)} (need > 0); arms moved {sorted(k for k, v in moved.items() if v)}; "
              f"moved without tier-2/3 transmission {quiet} (need none)")
    safe("REACH", c_reach)

    def c_power():
        a, b = summary(d, "def"), summary(d, "on")
        ln = arm_lines(d, "on")
        ratios = {"2p4": [], "uwb": []}
        bad = []
        for ra, rb in zip(a, b):
            for t, col in (("2p4", 2), ("uwb", 5)):
                dj = float(rb["tx_" + t]) - float(ra["tx_" + t])
                ds = float(rb["airtime_" + t]) - float(ra["airtime_" + t])
                want = ln.get(ra["arm"], (0, 0, 0, 0, 0, 0))[col]
                if abs(ds - want) > 1e-6:
                    bad.append((ra["arm"], t, ds, want))
                if ds > 0:
                    ratios[t].append(dj / ds)
        spread = {t: (max(v) - min(v)) / max(v) if v else 0.0 for t, v in ratios.items()}
        ok = ratios["2p4"] and not bad and all(s <= 1e-6 for s in spread.values())
        check("POWER", ok,
              f"extra airtime vs the SY-1 line: {len(bad)} mismatches (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}; extra tx J per extra second: 2.4 GHz "
              f"{[round(x, 6) for x in ratios['2p4']]}, UWB {[round(x, 6) for x in ratios['uwb']]} "
              f"(one constant per radio)")
    safe("POWER", c_power)

    def c_ref():
        got = rc(d, "r_node")
        has = "--edgeUplinkRanges needs --gatewayEdgeServer=true" in log(d, "r_node")
        check("R", got == 1 and has, f"r_node: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G35 {tag:7s} {name:5s} {why}")
    print(f"  G35: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
