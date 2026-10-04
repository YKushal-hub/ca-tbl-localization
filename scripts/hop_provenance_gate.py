#!/usr/bin/env python3
"""G53: hop provenance reaches the decision and the RESULT (docs/SPEC_CONFORMANCE_PLAN.md
NW-1, --hopProvenance).

Physical sect 2.5 has trace(Sigma) "already folding in ... hop-decay"; the final plan models
confidence "as a function of the path a fix traveled -- hop count, ..."; Transport sect 3's
RESULT carries hop_count_at_fix. The measured tag's Step() was passed a literal 0 and the
RESULT said 0, though C5 knew each relayed range's depth. Under --hopProvenance the tag's and
every short relay's Step() get the deepest leg of the set they fuse, and a fix that fused it
carries it. hopWeight stays 0: hop enters through Sigma (C5's inflation), not as a PCS term.

Reads a directory written by scripts/hop_provenance_gate.sh (on, off: static_2p4 and
ca_tbl_full; r_norelay). Every check runs and is reported; exit 1 if any failed, 2 if an
input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  RECOUNT  each cycle's maxHop and relayed count are the relay_chain log's admitted tag legs
           at that instant (leg "tag", consumer 1, ADMIT), deepest and counted -- 0 on a
           tier-1-floor cycle -- and hop_count_at_fix is maxHop on a cycle that fused (a fix,
           not rejected, >= 3 ranges) and 0 otherwise.
  DEPTH    fused fixes carry at least two distinct non-zero depths (the chain reaches 2).
  LINE     the NW-1 line's fixes by depth equal the hops log.
  CONTROL  without the flag no hops_ file and no NW-1 line.
  R        --hopProvenance without --relayedRanging is refused.
"""
import csv
import os
import re
import sys

LINE = re.compile(r"NW-1 hop provenance \((\w+)\): fixes by depth of the fused set 0:(\d+) "
                  r"1:(\d+) 2:(\d+) 3\+:(\d+)")
ARMS = ("static_2p4", "ca_tbl_full")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(p):
    with open(p) as f:
        return list(csv.DictReader(f))


def main():
    if len(sys.argv) != 2:
        print("usage: hop_provenance_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError, StopIteration) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    ON = os.path.join(d, "on")

    def c_runs():
        bad = [n for n in ("on", "off") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_recount():
        bad, n, rel = [], 0, 0
        for arm in ARMS:
            legs = {}
            for r in rows(os.path.join(ON, f"relay_chain_{arm}_seed7.csv")):
                if r["leg"] == "tag" and r["consumerId"] == "1" and r["verdict"] == "ADMIT":
                    legs.setdefault(r["t"], []).append(int(r["hopDepth"]))
            for h in rows(os.path.join(ON, f"hops_{arm}_seed7.csv")):
                n += 1
                got = legs.get(h["t"], [])
                if h["usedFloor"] == "1":
                    want_max, want_n = 0, 0
                else:
                    want_max, want_n = (max(got) if got else 0), sum(1 for x in got if x > 0)
                rel += want_n
                fused = h["hadFix"] == "1" and h["rejected"] == "0" and int(h["nMeas"]) >= 3
                want_fix = want_max if fused else 0
                if (int(h["maxHop"]), int(h["nRelayed"]), int(h["hopCountAtFix"])) != \
                        (want_max, want_n, want_fix):
                    bad.append((arm, h["t"], h["maxHop"], h["nRelayed"], h["hopCountAtFix"],
                                want_max, want_n, want_fix))
        check("RECOUNT", n > 0 and rel > 0 and not bad,
              f"{n} cycles, {rel} relayed legs in the sets Step() was given; not as recounted from the chain "
              f"log: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("RECOUNT", c_recount)

    def c_depth():
        depths = set()
        for arm in ARMS:
            for h in rows(os.path.join(ON, f"hops_{arm}_seed7.csv")):
                if int(h["hopCountAtFix"]) > 0:
                    depths.add(int(h["hopCountAtFix"]))
        check("DEPTH", len(depths) >= 2,
              f"distinct non-zero hop_count_at_fix over fused fixes: {sorted(depths)} (need >= 2)")
    safe("DEPTH", c_depth)

    def c_line():
        bad = []
        lines = {m.group(1): tuple(int(x) for x in m.groups()[1:]) for m in LINE.finditer(log(d, "on"))}
        for arm in ARMS:
            hs = rows(os.path.join(ON, f"hops_{arm}_seed7.csv"))
            want = tuple(sum(1 for h in hs if min(3, int(h["hopCountAtFix"])) == k) for k in range(4))
            if lines.get(arm) != want:
                bad.append((arm, lines.get(arm), want))
        check("LINE", not bad, f"NW-1 lines not equal to the hops logs: {bad or 'none'}")
    safe("LINE", c_line)

    def c_control():
        fs = [f for f in os.listdir(os.path.join(d, "off")) if f.startswith("hops_")]
        lines = log(d, "off").count("NW-1")
        check("CONTROL", not fs and lines == 0,
              f"without the flag: hops_ files {len(fs)}, NW-1 lines {lines} (need 0, 0)")
    safe("CONTROL", c_control)

    def c_ref():
        got = rc(d, "r_norelay")
        has = "FATAL: --hopProvenance needs --relayedRanging" in log(d, "r_norelay")
        check("R", got == 1 and has, f"r_norelay: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G53 {tag:7s} {name:7s} {why}")
    print(f"  G53: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
