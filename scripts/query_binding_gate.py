#!/usr/bin/env python3
"""G32: each tier's identity-query driver sends on that tier's radio
(docs/SPEC_CONFORMANCE_PLAN.md NW-3).

Every tier's query driver was bound to the first anchor MAC in the run -- gateway 101's
868 MHz radio -- so a "2.4 GHz" query went out with 868 MHz's SF and bandwidth, was charged
to that radio's duty budget, and was heard by 868 MHz radios. --queryTierBinding=per-tier
binds driver k to the same gateway's tier-k radio. Given explicitly, either value prints
one line per driver: its node, the radio (channel) its MAC is registered on, and that MAC's
QUERY count and airtime.

Reads a directory written by scripts/query_binding_gate.sh. Every check runs and is
reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL   --queryTierBinding=first IS the default: every output file byte-identical, and
         the default prints no NW-3 line.
  FIRST  the defect, observed: under `first` all three drivers are on gateway 101's radio
         0, and that MAC's QUERY airtime decomposes EXACTLY as k frames at the 868 MHz
         time on air plus (sent - k) at the UWB frame time, for an integer k -- the
         "2.4 GHz" queries were billed at 868 MHz airtime -- and does NOT decompose with
         the 2.4 GHz time on air in place of either.
  PER    under per-tier driver k is on radio k of the same gateway, each sent > 0, and each
         MAC's airtime per QUERY is that tier's time on air, RECOMPUTED here from the
         Semtech formula (SF7/125 kHz at 868 MHz, SF7/800 kHz at 2.4 GHz, main()'s phy[]
         at defaults) and the IEEE 802.15.4z frame time, for the 16-byte QUERY (9-byte
         header + 6 + 1 manifest id).
  R      the flag without queries, and an unknown value, are refused (exit 1).
"""
import math
import os
import re
import sys

PAYLOAD = 9 + 6 + 1  # CA_TBL_MAC_HEADER_BYTES + CaTblPayloadBytes(QUERY, 1)


def lora_toa(sf, bw, pl, cr=1, pre=8, explicit=True, crc=True):
    ts = 2.0 ** sf / bw
    tp = (pre + 4.25) * ts
    de = 1 if ts > 0.016 else 0
    num = 8 * pl - 4 * sf + 28 + 16 * (1 if crc else 0) - 20 * (0 if explicit else 1)
    n = max(math.ceil(num / (4 * (sf - 2 * de))) * (cr + 4), 0)
    return tp + (8 + n) * ts


def uwb_toa(pl, pre=128, rate=6.81e6):
    return (pre + 8) * 1.0176e-6 + 19.0 / 850e3 + (8 * pl + 48) / rate


TOA = {0: lora_toa(7, 125e3, PAYLOAD), 1: lora_toa(7, 800e3, PAYLOAD), 2: uwb_toa(PAYLOAD)}
LINE = re.compile(r"NW-3 query driver tier (\d): node (\d+) radio (-?\d+), QUERY sent (\d+) "
                  r"airtime ([0-9.eE+-]+) s \((first|per-tier)\)")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def drivers(d, n):
    return [tuple(m.groups()) for m in LINE.finditer(log(d, n))]


def main():
    if len(sys.argv) != 2:
        print("usage: query_binding_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def c_runs():
        bad = [n for n in ("def", "first", "per") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        ff = sorted(f for f in os.listdir(os.path.join(d, "first")) if f != "run_id.txt")
        same = fd == ff and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "first", f), "rb").read() for f in fd)
        nl = len(drivers(d, "def")) + log(d, "def").count("NW-3")
        check("NULL", same and nl == 0 and fd,
              f"explicit first vs default: {len(fd)} files, {'identical' if same else 'DIFFER'}; "
              f"{nl} NW-3 lines under the default (need 0)")
    safe("NULL", c_null)

    def c_first():
        dv = drivers(d, "first")
        ok = len(dv) == 3 and {x[0] for x in dv} == {"0", "1", "2"} and \
            all(x[1] == "101" and x[2] == "0" and x[5] == "first" for x in dv)
        sent, air = int(dv[0][3]), float(dv[0][4])
        same_mac = all(int(x[3]) == sent and float(x[4]) == air for x in dv)

        def decomposes(ta, tb):
            return [k for k in range(sent + 1) if abs(k * ta + (sent - k) * tb - air) < 1e-7]
        k868 = decomposes(TOA[0], TOA[2])
        k2p4 = decomposes(TOA[1], TOA[2]) + decomposes(TOA[0], TOA[1])
        check("FIRST", ok and same_mac and sent > 0 and len(k868) == 1 and not k2p4,
              f"first: drivers on (node, radio) {[(x[1], x[2]) for x in dv]} (need 101/0 for all); "
              f"one MAC, {sent} QUERY frames, {air:.9f} s = {k868[0] if k868 else '?'} x 868 MHz ToA + "
              f"the rest x UWB frame time ({'exact' if len(k868) == 1 else 'NO DECOMPOSITION'}); "
              f"decompositions using the 2.4 GHz ToA: {len(k2p4)} (need 0)")
    safe("FIRST", c_first)

    def c_per():
        dv = drivers(d, "per")
        bad = []
        for t, node, radio, sent, air, mode in dv:
            per = float(air) / int(sent) if int(sent) else -1
            if radio != t or node != "101" or mode != "per-tier" or int(sent) <= 0 or \
                    abs(per - TOA[int(t)]) > 1e-9 * max(1.0, TOA[int(t)] * 1e3):
                bad.append((t, node, radio, sent, per))
        check("PER", len(dv) == 3 and not bad,
              f"per-tier: {len(dv)} drivers; per-QUERY airtime " +
              ", ".join(f"tier {x[0]} radio {x[2]} {float(x[4]) / max(1, int(x[3])):.9f} s "
                        f"(recomputed {TOA[int(x[0])]:.9f})" for x in dv) +
              (f"; WRONG: {bad}" if bad else ""))
    safe("PER", c_per)

    def c_ref():
        for n, text in [("r_noq", "--queryTierBinding needs --identityQueryPeriodS > 0"),
                        ("r_bad", "--queryTierBinding must be first or per-tier")]:
            got, has = rc(d, n), text in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G32 {tag:7s} {name:5s} {why}")
    print(f"  G32: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
