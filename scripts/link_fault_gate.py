#!/usr/bin/env python3
"""G37: the transmit-power-drop and antenna-loss fault classes
(docs/SPEC_CONFORMANCE_PLAN.md SY-5).

The docx's evaluation induces "statistically clean, physically wrong" faults: a cold-sagged
battery's RSSI skew, an antenna detuned by ice, a covered antenna. --injectTxPowerDropDb
lowers what the measured node sends; --injectAntennaLossDb lowers what it sends and what it
receives. Both act over the --injectBiasFraction window, seeded from --seed and the same for
every arm. Tier 1 inverts RSSI against the NOMINAL power, so its ranges read long by
10^(D / (10 n)); tiers 2 and 3 lose SNR and links, not bias. No random draw is taken.

Reads a directory written by scripts/link_fault_gate.sh (def, zero, drop, loss: ca_tbl_full,
seed 7, 900 s, window 0.3; r_nowin, r_neg). Every check runs and is reported; exit 1 if any
failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    both magnitudes given as 0 IS no fault: every file byte-identical, and no SY-5
          line under either.
  WINDOW  the SY-5 line's window is [900 (0.25 + 0.5 (seed % 7) / 7), + 0.3 x 900) =
          [225, 495) s, recomputed; and the fault acts in it and not before: every trace
          row with t < 225 s is byte-identical to the no-fault run's, and some row at or
          after 225 s differs. (After the window the runs need not reconverge: the shared
          stream is displaced, rule 8.)
  SKEW    the tier-1 skew factor is 10^((D + L) / (10 n)), recomputed from the line's own
          magnitudes and n; it was applied on > 0 links, never more than were attenuated;
          both runs attenuate ranging links (> 0).
  DIRECT  the drop is transmit-only: MAC links attenuated as the node RECEIVES are 0 under
          the drop and > 0 under the antenna loss, and both attenuate what it sends (> 0).
  R       a magnitude without the window, and a negative magnitude, are refused (exit 1).
"""
import csv
import os
import re
import sys

LINE = re.compile(r"SY-5 link fault: window \[([0-9.eE+-]+), ([0-9.eE+-]+)\) s, tx-power drop "
                  r"([0-9.eE+-]+) dB, antenna loss ([0-9.eE+-]+) dB, n ([0-9.eE+-]+), tier-1 skew x([0-9.eE+-]+); "
                  r"ranging links attenuated (\d+), tier-1 ranges skewed (\d+); MAC links attenuated "
                  r"tx (\d+) rx (\d+)")
SEED, DUR, FRAC = 7, 900.0, 0.3


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def trace(d, n):
    with open(os.path.join(d, n, "trace_ca_tbl_full_seed7.csv")) as f:
        return f.read().split("\n")


def main():
    if len(sys.argv) != 2:
        print("usage: link_fault_gate.py <dir>", file=sys.stderr)
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
        bad = [n for n in ("def", "zero", "drop", "loss") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fz = sorted(f for f in os.listdir(os.path.join(d, "zero")) if f != "run_id.txt")
        same = fd == fz and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "zero", f), "rb").read() for f in fd)
        lines = log(d, "def").count("SY-5") + log(d, "zero").count("SY-5")
        check("NULL", same and fd and lines == 0,
              f"both magnitudes 0 vs no fault: {len(fd)} files, {'identical' if same else 'DIFFER'}; "
              f"SY-5 lines {lines} (need 0)")
    safe("NULL", c_null)

    def c_window():
        start = DUR * (0.25 + 0.5 * (SEED % 7) / 7.0)
        end = start + FRAC * DUR
        bad = []
        base = trace(d, "def")
        hdr = base[0].split(",")
        ti = hdr.index("t")
        for n in ("drop", "loss"):
            m = LINE.search(log(d, n))
            if not m or abs(float(m.group(1)) - start) > 1e-6 or abs(float(m.group(2)) - end) > 1e-6:
                bad.append((n, "window", m.groups()[:2] if m else None, (start, end)))
            tr = trace(d, n)
            first = None
            for a, b in zip(base[1:], tr[1:]):
                if not a:
                    continue
                if a != b:
                    first = float(a.split(",")[ti])
                    break
            if first is None or first < start:
                bad.append((n, "first differing row", first))
        check("WINDOW", not bad,
              f"window [{start:.1f}, {end:.1f}) s recomputed; drop/loss lines and first differing "
              f"trace row vs no fault: {len(bad)} problems (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("WINDOW", c_window)

    def c_skew():
        bad, seen = [], []
        for n in ("drop", "loss"):
            m = LINE.search(log(d, n))
            rl, sk = int(m.group(7)), int(m.group(8))
            db = float(m.group(3)) + float(m.group(4))
            want = 10.0 ** (db / (10.0 * float(m.group(5))))
            seen.append((n, rl, sk, float(m.group(5)), float(m.group(6))))
            if rl <= 0 or sk <= 0 or sk > rl or abs(float(m.group(6)) - want) > 1e-6 * want:
                bad.append((n, rl, sk, float(m.group(6)), want))
        check("SKEW", not bad,
              f"(run, ranging links attenuated, tier-1 ranges skewed, n, factor): {seen}; need both "
              f"> 0, skewed <= attenuated, factor = 10^(dB/(10 n)){': BAD ' + str(bad) if bad else ''}")
    safe("SKEW", c_skew)

    def c_direct():
        md, ml = LINE.search(log(d, "drop")), LINE.search(log(d, "loss"))
        dtx, drx = int(md.group(9)), int(md.group(10))
        ltx, lrx = int(ml.group(9)), int(ml.group(10))
        check("DIRECT", dtx > 0 and drx == 0 and ltx > 0 and lrx > 0,
              f"MAC links attenuated: drop tx {dtx} rx {drx} (need > 0, 0); loss tx {ltx} rx {lrx} "
              f"(need > 0, > 0)")
    safe("DIRECT", c_direct)

    def c_ref():
        for n in ("r_nowin", "r_neg"):
            got = rc(d, n)
            has = "--injectTxPowerDropDb and --injectAntennaLossDb take a value >= 0" in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G37 {tag:7s} {name:6s} {why}")
    print(f"  G37: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
