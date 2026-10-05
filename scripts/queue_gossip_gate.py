#!/usr/bin/env python3
"""G52: anchors gossip their queue as well as their grants (docs/SPEC_CONFORMANCE_PLAN.md
NW-6, --gossipQueue).

Network sect 6: "Anchors periodically gossip their current Q (request queue) and active
grants", and on an overlap the lower id defers. GRANT_GOSSIP carried grants only, so two
anchors could hold the same request in their queues (NW-5) and each grant it from there.
Under --gossipQueue the gossip carries the queue, and the tie-break extends to it: a
lower-id arbiter gives up (with a final denial) or refuses to queue a request a higher-id
anchor queues or has granted. Finders gossip too, over NW-0's mesh links.

Reads a directory written by scripts/queue_gossip_gate.sh (q, nq, r_noarb). Every check runs
and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  GOSSIP  gossip frames carry queue items (events_*: GRANT_GOSSIP with queue=k), some of them
          from finders; the NW-6 line's items heard are the event log's, recounted per
          distinct frame received by an arbiter, and its items sent are at least those on
          the air (a composed frame can fail CAD).
  YIELD   every queued request an arbiter gave up (arb_*: yield) had a higher-id anchor
          that held it within the gossip's reach of that moment (at t, t - 5 s or t - 10 s:
          the yield acts on gossip up to a period old) -- queued (a queue row there, not
          since granted, dropped or yielded) or granted within the 10 s slot hold. Some
          yields happened.
  AFTER   an arbiter that yielded a request never grants it from its queue afterwards
          unless it queued it again.
  COUNTS  the NW-6 line's yields equal the yield rows.
  CONTROL grants-only gossip carries no queue item, prints no NW-6 line, and yields nothing.
  R       --gossipQueue without --arbitrationV2 is refused.
"""
import csv
import os
import re
import sys

LINE = re.compile(r"NW-6 queue gossip \((\w+)\): queue items sent (\d+), heard (\d+), "
                  r"double-queued (\d+), queued requests yielded to a higher id (\d+) "
                  r"\((\d+) finders gossiping\)")
OK_OUT = ("delivered", "capture")
HOLD = 10.0


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(p):
    with open(p) as f:
        return list(csv.DictReader(f))


def payload(p):
    return dict(kv.split("=", 1) for kv in p.split(";") if "=" in kv)


def main():
    if len(sys.argv) != 2:
        print("usage: queue_gossip_gate.py <dir>", file=sys.stderr)
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

    Q = os.path.join(d, "q")

    def c_runs():
        bad = [n for n in ("q", "nq") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_gossip():
        ev = rows(os.path.join(Q, "events_ca_tbl_full_seed1.csv"))
        sent, heard, finders = {}, {}, set()
        for r in ev:
            if r["msgType"] != "GRANT_GOSSIP":
                continue
            p = payload(r["payload"])
            k = int(p.get("queue", "0"))
            if k == 0:
                continue
            sent[(r["tier"], r["txId"], p["pkt"])] = k
            if int(r["txId"]) >= 501:
                finders.add(r["txId"])
            # Only an arbiter (a gateway or a finder) stores a heard queue.
            if r["outcome"] in OK_OUT and int(r["rxId"]) >= 101:
                heard[(r["tier"], r["rxId"], p["pkt"])] = k
        m = LINE.search(log(d, "q"))
        got = (int(m.group(2)), int(m.group(3))) if m else None
        want = (sum(sent.values()), sum(heard.values()))
        # Sent counts frames composed, which can exceed frames that reached the air.
        ok = got is not None and want[0] > 0 and want[1] > 0 and finders and \
            got[1] == want[1] and got[0] >= want[0]
        check("GOSSIP", ok,
              f"queue items on gossip frames on the air {want[0]}, received {want[1]}; NW-6 line "
              f"(sent, heard) {got} (need heard equal, sent >= on the air, both > 0); finders "
              f"gossiping a queue {sorted(finders)} (need some)")
    safe("GOSSIP", c_gossip)

    st = {}

    def arb():
        if "a" not in st:
            st["a"] = rows(os.path.join(Q, "arb_ca_tbl_full_seed1.csv"))
        return st["a"]

    def held_by(a_rows, anchor, req, t):
        """Did `anchor` hold `req` (queued, or granted within HOLD) at time t?"""
        state, gt = None, None
        for r in a_rows:
            if r["arbiter"] != anchor or r["requester"] != req or float(r["t"]) > t + 1e-9:
                continue
            ev = r["event"]
            if ev == "queue":
                state = "queued"
            elif ev in ("grant", "preempt", "aged_grant"):
                state, gt = "granted", float(r["t"])
            elif ev in ("expire", "yield"):
                state = None
        if state == "queued":
            return True
        return state == "granted" and t - gt < HOLD

    def c_yield():
        a = arb()
        anchors = sorted({r["arbiter"] for r in a}, key=int)
        bad, n = [], 0
        for r in a:
            if r["event"] != "yield":
                continue
            n += 1
            t = float(r["t"])
            # The yielding arbiter acts on gossip up to a period (5 s) old, so the higher
            # anchor must have held the request then or since.
            higher = [b for b in anchors if int(b) > int(r["arbiter"])
                      and any(held_by(a, b, r["requester"], t - lag) for lag in (0.0, 5.0, 10.0))]
            if not higher:
                bad.append((r["t"], r["arbiter"], r["requester"]))
        check("YIELD", n > 0 and not bad,
              f"{n} yields; with no higher-id anchor holding the request: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("YIELD", c_yield)

    def c_after():
        a = arb()
        bad = []
        yielded = set()
        for r in a:
            key = (r["arbiter"], r["requester"])
            if r["event"] == "yield":
                yielded.add(key)
            elif r["event"] == "queue":
                yielded.discard(key)
            elif r["event"] == "aged_grant" and key in yielded:
                bad.append((r["t"], key))
        check("AFTER", not bad, f"grants from the queue of a request yielded and not re-queued: "
                                f"{len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("AFTER", c_after)

    def c_counts():
        m = LINE.search(log(d, "q"))
        got = int(m.group(5)) if m else None
        want = sum(r["event"] == "yield" for r in arb())
        check("COUNTS", got == want, f"NW-6 line yields {got}; yield rows {want}")
    safe("COUNTS", c_counts)

    def c_control():
        ev = rows(os.path.join(d, "nq", "events_ca_tbl_full_seed1.csv"))
        qf = sum(1 for r in ev if r["msgType"] == "GRANT_GOSSIP" and ";queue=" in r["payload"])
        lines = log(d, "nq").count("NW-6")
        y = sum(1 for r in rows(os.path.join(d, "nq", "arb_ca_tbl_full_seed1.csv"))
                if r["event"] == "yield")
        gossips = sum(1 for r in ev if r["msgType"] == "GRANT_GOSSIP")
        check("CONTROL", qf == 0 and lines == 0 and y == 0 and gossips > 0,
              f"grants-only gossip: frames with a queue {qf}, NW-6 lines {lines}, yields {y} (need "
              f"0, 0, 0); gossip frames {gossips} (need > 0)")
    safe("CONTROL", c_control)

    def c_ref():
        got = rc(d, "r_noarb")
        has = "FATAL: --gossipQueue needs --anchorGrantGossipS > 0 and --arbitrationV2" in log(d, "r_noarb")
        check("R", got == 1 and has, f"r_noarb: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G52 {tag:7s} {name:7s} {why}")
    print(f"  G52: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
