#!/usr/bin/env python3
"""G91: the S2-B accuracy flags (docs/TECH_DEBT.md N.35-N.38).

  --dualBaseRelayTier  under --loraDualBase, tag 0's relayed (C5) ranges are formed on the
                       awake 2.4 GHz radio whatever the active tier (N.35).
  --relayCovScale k    hybrid arms scale a relay's position covariance by k before admission (N.36).
  --hybridNoLone868    hybrid arms step a one-range tier-1 cycle as empty, so Case D never
                       applies a lone 868 MHz RSSI range (N.37).
  --caseDInfoRatio k   under --caseD a single range whose sigma exceeds k times the prior's
                       along its line of sight is refused; the cycle stays a Case D cycle (N.38).

Reads a directory written by scripts/acc_b_gate.sh (endgame: base, off, relay, scale; 5 km:
a_base, a_lone, a_info). Every check runs and is reported; exit 1 if any failed, 2 if an input is
missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  RC        a run exited non-zero.
  NULL      the three flags given at their off values are the flags absent: every output
            file byte-identical except run_id.txt (its invocation line differs).
  RELAY     with --dualBaseRelayTier, tag-leg rows of relay_chain_ on 2.4 GHz (tier 1) at a
            cycle whose trace activeTier is 868MHz exist; flags absent there are none. A
            fire is a count of 0 under the flag, or any under the base run.
  SCALE     with --relayCovScale=4 the first tag-leg relay_chain_ row carries exactly 4x the
            carriedVarM2 of the relay run's first row, same t and relay (the stream has not
            diverged before the first relayed leg). A fire is any other ratio.
  LONE      5 km, flags absent: some Case D row (cased_) falls on a cycle whose trace fixTier
            is 868MHz. Under --hybridNoLone868 none does, while one-range 868MHz cycles
            (trace nRangesEkf 1) still occur, so the zero is not an absence of the case
            (rule 4).
  INFO      5 km, --caseDInfoRatio=3: some Case D row is refused although its NIS passed
            6.63 (the new refusal is reached; rule 4), none is applied with NIS above 6.63,
            and the N.38 stdout count equals those rows. Flags absent there is no such row.
"""
import re
import csv
import filecmp
import os
import sys

ARM = "ca_tbl_full"
SEED = 1


def rows(d, n, f):
    with open(os.path.join(d, n, f)) as h:
        return list(csv.DictReader(h))


def trace(d, n):
    return {r["t"]: r for r in rows(d, n, f"trace_{ARM}_seed{SEED}.csv")}


def tag_legs(d, n):
    return [r for r in rows(d, n, f"relay_chain_{ARM}_seed{SEED}.csv") if r["leg"] == "tag"]


def main():
    if len(sys.argv) != 2:
        print("usage: acc_b_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    names = ["base", "off", "relay", "scale", "a_base", "a_lone", "a_info"]
    for n in names:
        if not os.path.isfile(os.path.join(d, n + ".rc")) or not os.path.isdir(os.path.join(d, n)):
            print(f"  G91 MISSING input {n}")
            return 2
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    bad = [n for n in names if open(os.path.join(d, n + ".rc")).read().strip() != "0"]
    check("RC", not bad, f"non-zero exit: {bad}")

    fb = sorted(os.listdir(os.path.join(d, "base")))
    fo = sorted(os.listdir(os.path.join(d, "off")))
    diff = [f for f in fb if f != "run_id.txt" and (f not in fo or not filecmp.cmp(
        os.path.join(d, "base", f), os.path.join(d, "off", f), shallow=False))]
    diff += [f for f in fo if f not in fb]
    check("NULL", fb and not diff, f"{len(fb)} files, differing: {diff[:5]}")

    def cross(n):
        tr = trace(d, n)
        return sum(1 for r in tag_legs(d, n) if r["tier"] == "1" and tr[r["t"]]["activeTier"] == "868MHz")

    cb, cr = cross("base"), cross("relay")
    check("RELAY", cb == 0 and cr > 0, f"2.4 GHz tag legs on 868MHz cycles: base {cb}, relay {cr}")

    lr, ls = tag_legs(d, "relay"), tag_legs(d, "scale")
    ok = bool(lr and ls) and lr[0]["t"] == ls[0]["t"] and lr[0]["relayId"] == ls[0]["relayId"]
    ratio = float(ls[0]["carriedVarM2"]) / float(lr[0]["carriedVarM2"]) if ok and float(lr[0]["carriedVarM2"]) > 0 else -1.0
    check("SCALE", ok and abs(ratio - 4.0) < 1e-6, f"first tag leg carriedVarM2 ratio {ratio:.9f}")

    def lone(n):
        tr = trace(d, n)
        cd = sum(1 for r in rows(d, n, f"cased_{ARM}_seed{SEED}.csv") if tr[r["t"]]["fixTier"] == "868MHz")
        one = sum(1 for r in tr.values() if r["fixTier"] == "868MHz" and r["nRangesEkf"] == "1")
        return cd, one

    (bc, bo), (lc, lo) = lone("a_base"), lone("a_lone")
    check("LONE", bc > 0 and lc == 0 and lo > 0,
          f"Case D rows on 868MHz cycles: base {bc}, flag {lc}; one-range 868MHz cycles under the flag {lo}")

    def info(n):
        cd = rows(d, n, f"cased_{ARM}_seed{SEED}.csv")
        ref = sum(1 for r in cd if r["applied"] == "0" and 0.0 <= float(r["nis"]) <= 6.63)
        over = sum(1 for r in cd if r["applied"] == "1" and float(r["nis"]) > 6.63)
        return ref, over

    (ib, _), (ii, io) = info("a_base"), info("a_info")
    with open(os.path.join(d, "a_info.log")) as h:
        m = re.search(r"N\.38 Case D refused as less informative than the prior \(sigma ratio > 3\): (\d+)", h.read())
    line = int(m.group(1)) if m else -1
    check("INFO", ib == 0 and ii > 0 and io == 0 and line == ii,
          f"refused with NIS <= 6.63: base {ib}, flag {ii} (stdout {line}); applied with NIS > 6.63 {io}")

    nbad = 0
    for name, ok, why in res:
        print(f"  G91 {'ok  ' if ok else 'FAIL'} {name:8s} {why}")
        nbad += 0 if ok else 1
    print(f"  G91 {len(res) - nbad}/{len(res)} checks pass")
    return 1 if nbad else 0


if __name__ == "__main__":
    sys.exit(main())
