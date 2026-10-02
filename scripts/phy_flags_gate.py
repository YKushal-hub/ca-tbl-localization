#!/usr/bin/env python3
"""G36: tier-1 and tier-2 spreading factor and bandwidth from the CLI
(docs/SPEC_CONFORMANCE_PLAN.md SY-3; Physical 1.1, "configurable spreading factor (SF7-SF12)
and bandwidth (125/250/500 kHz)").

--sf868/--bw868/--sf2p4/--bw2p4 replace the hard-coded phy[] values when given. A bandwidth
other than the one an engine's sensitivity table was written for (868 MHz: 125 kHz; 2.4 GHz:
800 kHz) shifts that table by 10 log10(BW / BW0) dB. Given, the flags print one SY-3 PHY
line (main) and one SY-3 engine sensitivity line per arm, read back from the configs the
engines are built from.

Reads a directory written by scripts/phy_flags_gate.sh (def, expl, sf9, bw: static_868,
static_2p4, ca_tbl_full, seed 7, 600 s; r_sf, r_bw). Every check runs and is reported; exit
1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    the four flags given at their defaults ARE the default: every output file
          byte-identical (the stdout gains the SY-3 lines, nothing else); the default prints
          no SY-3 line.
  LINE    each run's SY-3 PHY line states the SF and BW it was given, the offset
          10 log10(BW/BW0) recomputed here, and the DISCOVERY time on air (9 + 5 bytes)
          recomputed from the Semtech formula, to 1e-9 s.
  ENGINE  every arm's engine sensitivity is the datasheet table entry for its SF plus that
          offset (868: -123..-137 dBm at SF7..12; 2.4 GHz: -100..-121 at SF5..12), to 1e-6 dB
          -- the shift reached the engines, not just the printout.
  AIR     the flags reached the energy model. static_868 bills one broadcast per cycle, so
          its airtime is cycles x ToA(SF, BW, B) exactly (1e-6 s), with the frame size B
          solved from the default run; static_2p4 polls each addressed anchor, so its
          airtime is a whole number of ToA(SF, BW, B) frames.
  R       an SF outside SF7-SF12 and a bandwidth outside the table are refused (exit 1).
"""
import csv
import math
import os
import re
import sys

PHY = re.compile(r"SY-3 PHY: 868 MHz SF(\d+) BW ([0-9.e+]+) Hz sensitivity offset ([0-9.eE+-]+) dB "
                 r"DISCOVERY time on air ([0-9.eE+-]+) s; 2\.4 GHz SF(\d+) BW ([0-9.e+]+) Hz "
                 r"sensitivity offset ([0-9.eE+-]+) dB DISCOVERY time on air ([0-9.eE+-]+) s")
ENG = re.compile(r"SY-3 engine sensitivity: 868 MHz SF(\d+) ([0-9.eE+-]+) dBm; 2\.4 GHz SF(\d+) "
                 r"([0-9.eE+-]+) dBm")
T868 = {7: -123.0, 8: -126.0, 9: -129.0, 10: -132.0, 11: -134.5, 12: -137.0}
T2P4 = {sf: -100.0 - 3.0 * (sf - 5) for sf in range(5, 13)}
RUNS = {"expl": (7, 125e3, 7, 800e3), "sf9": (9, 125e3, 7, 800e3), "bw": (7, 250e3, 9, 812500.0)}
DISCOVERY = 9 + 5


def lora_toa(sf, bw, pl, cr=1, pre=8):
    ts = 2.0 ** sf / bw
    de = 1 if ts > 0.016 else 0
    num = 8 * pl - 4 * sf + 28 + 16
    n = max(math.ceil(num / (4 * (sf - 2 * de))) * (cr + 4), 0)
    return (pre + 4.25) * ts + (8 + n) * ts


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def summary(d, n):
    return {r["arm"]: r for r in csv.DictReader(open(os.path.join(d, n, "arm_summary.csv")))}


def main():
    if len(sys.argv) != 2:
        print("usage: phy_flags_gate.py <dir>", file=sys.stderr)
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
        bad = [n for n in ("def", "expl", "sf9", "bw") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fe = sorted(f for f in os.listdir(os.path.join(d, "expl")) if f != "run_id.txt")
        same = fd == fe and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "expl", f), "rb").read() for f in fd)
        lines = log(d, "def").count("SY-3")
        check("NULL", same and fd and lines == 0 and PHY.search(log(d, "expl")),
              f"the four flags at their defaults vs the default: {len(fd)} files, "
              f"{'identical' if same else 'DIFFER'}; SY-3 lines under the default {lines} (need 0)")
    safe("NULL", c_null)

    def c_line():
        bad = []
        for n, (s1, b1, s2, b2) in RUNS.items():
            m = PHY.search(log(d, n))
            g = m.groups()
            want = (s1, b1, 10 * math.log10(b1 / 125e3), lora_toa(s1, b1, DISCOVERY),
                    s2, b2, 10 * math.log10(b2 / 800e3), lora_toa(s2, b2, DISCOVERY))
            got = (int(g[0]), float(g[1]), float(g[2]), float(g[3]),
                   int(g[4]), float(g[5]), float(g[6]), float(g[7]))
            if got[0] != want[0] or got[4] != want[4] or any(
                    abs(x - y) > 1e-6 for x, y in [(got[1], want[1]), (got[2], want[2]), (got[5], want[5]),
                                                   (got[6], want[6])]) or abs(got[3] - want[3]) > 1e-9 or \
                    abs(got[7] - want[7]) > 1e-9:
                bad.append((n, got, want))
        check("LINE", not bad,
              f"SY-3 PHY lines of expl/sf9/bw vs recomputed SF, BW, offset and time on air: "
              f"{len(bad)} wrong (need 0){': ' + str(bad[:1]) if bad else ''}")
    safe("LINE", c_line)

    def c_engine():
        bad, seen = [], 0
        for n, (s1, b1, s2, b2) in RUNS.items():
            ls = ENG.findall(log(d, n))
            if len(ls) != 3:
                bad.append((n, f"{len(ls)} engine lines for 3 arms"))
            for sf1, v1, sf2, v2 in ls:
                seen += 1
                w1 = T868[s1] + 10 * math.log10(b1 / 125e3)
                w2 = T2P4[s2] + 10 * math.log10(b2 / 800e3)
                if int(sf1) != s1 or int(sf2) != s2 or abs(float(v1) - w1) > 1e-6 or abs(float(v2) - w2) > 1e-6:
                    bad.append((n, sf1, v1, w1, sf2, v2, w2))
        check("ENGINE", not bad and seen == 9,
              f"{seen} engine sensitivity lines (need 9); off the table plus offset: {len(bad)} "
              f"(need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("ENGINE", c_engine)

    def c_air():
        # 868 MHz is one broadcast per cycle, so static_868's airtime is cycles x ToA(B);
        # the frame size B is solved from the default run. 2.4 GHz polls each addressed
        # anchor, a varying number per cycle, so static_2p4's airtime must be a WHOLE
        # number of frames of that size at the run's SF/BW.
        base = summary(d, "def")
        cyc0, air0 = int(base["static_868"]["cycles_868"]), float(base["static_868"]["airtime_868"])
        sizes = [b for b in range(1, 256) if abs(lora_toa(7, 125e3, b) * cyc0 - air0) < 1e-6]
        out, bad = [], []
        if not sizes:
            bad.append(("static_868", "no frame size explains the default airtime"))
        for n, (s1, b1, s2, b2) in [("def", (7, 125e3, 7, 800e3))] + list(RUNS.items()):
            row = summary(d, n)
            c1, a1 = int(row["static_868"]["cycles_868"]), float(row["static_868"]["airtime_868"])
            if not any(abs(lora_toa(s1, b1, b) * c1 - a1) < 1e-6 for b in sizes):
                bad.append(("static_868", n, a1, c1))
            a2 = float(row["static_2p4"]["airtime_2p4"])
            frames = [a2 / lora_toa(s2, b2, b) for b in sizes]
            if not any(f > 0 and abs(f - round(f)) < 1e-6 * max(1.0, f) for f in frames):
                bad.append(("static_2p4", n, a2, [round(f, 6) for f in frames[:2]]))
            out.append((n, round(a1 / max(c1, 1), 6), round(frames[0], 3) if frames else None))
        check("AIR", not bad and sizes,
              f"ranging frame {sizes[:3] if sizes else '?'} B; per run (868 airtime per cycle, "
              f"2.4 GHz frames): {out}; inconsistent with the run's SF/BW: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("AIR", c_air)

    def c_ref():
        for n, text in (("r_sf", "--sf868=13 is outside SF7-SF12"), ("r_bw", "--bw868=300000 must be")):
            got, has = rc(d, n), text in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G36 {tag:7s} {name:6s} {why}")
    print(f"  G36: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
