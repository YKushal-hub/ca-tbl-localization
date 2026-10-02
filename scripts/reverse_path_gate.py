#!/usr/bin/env python3
"""G48: the reverse path really exists (docs/SPEC_CONFORMANCE_PLAN.md NW-8,
--reversePathHop).

Transport sect 1: "unicast home along the cached reverse path; fall back to flood". The
MAC was never told who transmitted a frame, so it cached the QUERY's ORIGINATOR, and the
RESPONSE's hit and miss branches sent the same one-hop frame to the same node: a path of
one hop, or none. Under --reversePathHop every node caches the PREVIOUS HOP (the channel's
transmitter of the first copy it accepted), the RESPONSE is unicast to it, each node on
the way forwards it to its own entry, the 300 s window is honoured, and a miss floods.

Reads a directory written by scripts/reverse_path_gate.sh (hop, legacy: G32's query regime
under --meshLinks=all with the event log; r_norp, r_noq). Every check runs and is reported;
exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9), from events_*.csv:
  PATH    every unicast RESPONSE hop X -> Y for query Q goes to the node Y from which X
          first received Q, on that tier -- recomputed from the QUERY rows, not taken from
          the MAC. A hop to anyone else, or from a node that never heard Q, is a firing.
  COUNTS  distinct answers -- (query, answerer) -- received unicast by a node that did not
          ask equal the NW-8 line's "forwarded", and received by their requester equal
          "delivered"; the further distinct frames of the same answers (retransmissions
          after a lost ACK, under fresh packet ids) equal "duplicates absorbed"; distinct
          answers each node sent unicast <= "unicast hops".
  MULTI   the path is more than one hop somewhere: a requester received an answer from a
          node that was itself forwarding it; forwarded > 0 and delivered > 0.
  LEGACY  --reversePath alone carries no route fields and prints no NW-8 line.
  R       --reversePathHop without --reversePath, or without queries, is refused.
"""
import csv
import os
import re
import sys

LINE = re.compile(r"NW-8 reverse path: unicast hops (\d+), forwarded (\d+), floods (\d+), "
                  r"expired entries (\d+), delivered to the requester (\d+), duplicates "
                  r"absorbed (\d+)")
OK_OUT = ("delivered", "capture")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def payload(p):
    out = {}
    for kv in p.split(";"):
        if "=" in kv:
            k, v = kv.split("=", 1)
            out[k] = v
    return out


def events(d, n):
    with open(os.path.join(d, n, "events_ca_tbl_full_seed7.csv")) as f:
        rd = csv.DictReader(f)
        out = []
        for r in rd:
            r["p"] = payload(r["payload"])
            out.append(r)
        return out


def main():
    if len(sys.argv) != 2:
        print("usage: reverse_path_gate.py <dir>", file=sys.stderr)
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

    st = {}

    def load():
        if "ev" not in st:
            ev = events(d, "hop")
            first_q = {}
            for r in ev:
                if r["msgType"] == "QUERY" and r["outcome"] in OK_OUT:
                    key = (r["tier"], r["rxId"], r["p"]["pkt"])
                    if key not in first_q:
                        first_q[key] = r["txId"]
            st["ev"] = ev
            st["first_q"] = first_q
            m = LINE.search(log(d, "hop"))
            st["line"] = tuple(int(x) for x in m.groups()) if m else None
        return st

    def c_runs():
        bad = [n for n in ("hop", "legacy") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_path():
        s = load()
        n, bad = 0, []
        for r in s["ev"]:
            if r["msgType"] != "RESPONSE" or r["outcome"] not in OK_OUT or "rq" not in r["p"]:
                continue
            if r["p"]["flood"] != "0":
                continue
            n += 1
            prev = s["first_q"].get((r["tier"], r["txId"], r["p"]["rq"]))
            if prev != r["rxId"]:
                bad.append((r["t"], r["tier"], r["txId"], r["rxId"], r["p"]["rq"], prev))
        check("PATH", n > 0 and not bad,
              f"{n} unicast RESPONSE hops received; not to the sender's previous hop for "
              f"that query: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("PATH", c_path)

    def c_counts():
        s = load()
        fwd, dlv, uni, fwd_f, dlv_f = set(), set(), set(), set(), set()
        for r in s["ev"]:
            if r["msgType"] != "RESPONSE" or "rq" not in r["p"]:
                continue
            p = r["p"]
            if p["flood"] == "0":
                uni.add((r["tier"], r["txId"], p["rq"], p["ans"]))
            if r["outcome"] not in OK_OUT:
                continue
            ans = (r["tier"], r["rxId"], p["rq"], p["ans"])
            frame = (r["tier"], r["rxId"], p["pkt"])
            if r["rxId"] == p["req"]:
                dlv.add(ans)
                dlv_f.add(frame)
            elif p["flood"] == "0":
                fwd.add(ans)
                fwd_f.add(frame)
        dup = (len(fwd_f) - len(fwd)) + (len(dlv_f) - len(dlv))
        ln = s["line"]
        ok = ln is not None and ln[1] == len(fwd) and ln[4] == len(dlv) and ln[5] == dup \
            and len(uni) <= ln[0]
        check("COUNTS", ok,
              f"NW-8 line {ln}; recounted forwarded {len(fwd)}, delivered {len(dlv)}, "
              f"duplicates {dup}, answers sent unicast {len(uni)} (need equal, equal, "
              f"equal, <= hops)")
    safe("COUNTS", c_counts)

    def c_multi():
        s = load()
        got = set()  # (tier, node, rq) that received a unicast copy of an answer
        for r in s["ev"]:
            if r["msgType"] == "RESPONSE" and r["outcome"] in OK_OUT and "rq" in r["p"] \
                    and r["p"]["flood"] == "0" and r["rxId"] != r["p"]["req"]:
                got.add((r["tier"], r["rxId"], r["p"]["rq"]))
        multi = sum(1 for r in s["ev"]
                    if r["msgType"] == "RESPONSE" and r["outcome"] in OK_OUT and "rq" in r["p"]
                    and r["rxId"] == r["p"]["req"]
                    and (r["tier"], r["txId"], r["p"]["rq"]) in got)
        ln = s["line"] or (0, 0, 0, 0, 0, 0)
        check("MULTI", multi > 0 and ln[1] > 0 and ln[4] > 0,
              f"answers that reached the requester from a forwarder {multi} (need > 0); "
              f"forwarded {ln[1]}, delivered {ln[4]} (need > 0)")
    safe("MULTI", c_multi)

    def c_legacy():
        ev = events(d, "legacy")
        rq = sum(1 for r in ev if "rq" in r["p"])
        resp = sum(1 for r in ev if r["msgType"] == "RESPONSE")
        lines = log(d, "legacy").count("NW-8")
        check("LEGACY", rq == 0 and lines == 0 and resp > 0,
              f"--reversePath alone: frames with route fields {rq}, NW-8 lines {lines} (need 0, "
              f"0); RESPONSE rows {resp} (need > 0)")
    safe("LEGACY", c_legacy)

    def c_ref():
        for n in ("r_norp", "r_noq"):
            got = rc(d, n)
            has = "FATAL: --reversePathHop needs --reversePath=true" in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G48 {tag:7s} {name:6s} {why}")
    print(f"  G48: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
