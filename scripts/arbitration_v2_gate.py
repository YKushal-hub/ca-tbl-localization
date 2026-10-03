#!/usr/bin/env python3
"""G51: query-triggered escalation and UWB arbitration as specified
(docs/SPEC_CONFORMANCE_PLAN.md NW-4 + NW-5, --queryTriggeredEscalation --arbitrationV2).

Network sect 4: an URGENT QUERY matching a target "can itself trigger UWB tier escalation
for that specific target", and the request says so (is_query_triggered). Network sect 5:
U(r) = l1*degradation + l2*(1/battery_remaining) + l3*staleness + l4*is_query_triggered,
at anchors, granted to the argmax "among requests within current UWB range of this
anchor", and "denied requests remain queued, re-scored next cycle (aging ... prevents
starvation)". The C4 arbiter scored the battery SPENT, a constant T_query, forgot a denial,
let only gateways arbitrate, and took hearing a 2.4 GHz request as UWB reach.

Reads a directory written by scripts/arbitration_v2_gate.sh (v2: both flags, URGENT v2
queries for five subjects; c4: the control; r_qte, r_urg, r_arb). Every check runs and is
reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9), from arb_*.csv unless stated:
  SCORE    every scored row's u is l.T recomputed from its logged terms, with T_batt =
           min(1/max(rem, 0.05), 20) x 0.05 and T_query the request's bit (1e-5).
  ARGMAX   at every grant from the queue, the granted request scored at least every
           candidate scored at that instant by that arbiter.
  RANGE    every grant, pre-emption and queueing had this arbiter's evidence of UWB reach
           at most 10 s old; every out-of-range refusal had none.
  QUEUE    queued requests are granted later from the queue -- at least one -- and at
           least one such grant reached its subject and committed UWB (grants_*: queued,
           then confirmed by the same anchor).
  FINDERS  a finder (id >= 501) granted a slot.
  QUERY    URGENT queries were latched, the hook escalated, and requests carried
           is_query_triggered (events_*: ESC_REQUEST with qt=1; and arb rows with qt=1).
  COUNTS   the NW-5 line equals the log: grants, finder grants, pre-emptions, grants from
           the queue, new queue entries, refusals out of range, drops from the queue.
  CONTROL  the C4 run writes no arb_ file and prints no NW-4/NW-5 line.
  R        the three refusals.
"""
import csv
import os
import re
import sys

LINE5 = re.compile(r"NW-5 arbitration \((\w+)\): grants (\d+) \(by finders (\d+)\), pre-emptions "
                   r"(\d+), from the queue (\d+), queued (\d+), out of range (\d+), dropped from "
                   r"the queue (\d+)")
LINE4 = re.compile(r"NW-4 query-triggered escalation \((\w+)\): URGENT queries latched (\d+); "
                   r"hook armed (\d+), fired (\d+), escalations (\d+), suppressed (\d+), ignored "
                   r"(\d+)")
EPS = 0.05


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
        print("usage: arbitration_v2_gate.py <dir>", file=sys.stderr)
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

    V = os.path.join(d, "v2")
    st = {}

    def arb():
        if "a" not in st:
            st["a"] = rows(os.path.join(V, "arb_ca_tbl_full_seed7.csv"))
        return st["a"]

    def c_runs():
        bad = [n for n in ("v2", "c4") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_score():
        bad, n = [], 0
        for r in arb():
            if r["event"] in ("out_of_range", "expire"):
                continue
            n += 1
            rem = float(r["rem"])
            tb = min(1.0 / max(rem, EPS), 1.0 / EPS) * EPS
            u = float(r["tDeg"]) + tb + float(r["tStale"]) + float(r["qt"])
            if abs(tb - float(r["tBatt"])) > 1e-5 or abs(u - float(r["u"])) > 1e-5:
                bad.append((r["t"], r["arbiter"], r["requester"], r["event"], r["u"], round(u, 6)))
        check("SCORE", n > 0 and not bad,
              f"{n} scored rows; u or T_batt not as recomputed: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("SCORE", c_score)

    def c_argmax():
        groups = {}
        for r in arb():
            if r["event"] in ("score", "aged_grant"):
                groups.setdefault((r["t"], r["arbiter"]), []).append(r)
        bad, n = [], 0
        for key, g in groups.items():
            win = [r for r in g if r["event"] == "aged_grant"]
            if not win:
                continue
            n += 1
            top = max(float(r["u"]) for r in g if r["event"] == "score")
            if float(win[0]["u"]) < top - 1e-9:
                bad.append((key, win[0]["requester"], win[0]["u"], top))
        check("ARGMAX", n > 0 and not bad,
              f"{n} grants from the queue; not the argmax of their cycle: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("ARGMAX", c_argmax)

    def c_range():
        bad, n = [], 0
        for r in arb():
            age = float(r["evidenceAgeS"])
            if r["event"] in ("grant", "preempt", "queue"):
                n += 1
                if not (0.0 <= age <= 10.0 + 1e-9):
                    bad.append((r["t"], r["arbiter"], r["requester"], r["event"], age))
            elif r["event"] == "out_of_range" and 0.0 <= age <= 10.0:
                bad.append((r["t"], r["arbiter"], r["requester"], "refused with evidence", age))
        oor = sum(r["event"] == "out_of_range" for r in arb())
        check("RANGE", n > 0 and not bad,
              f"{n} grants/pre-emptions/queueings, {oor} out-of-range refusals; evidence not as "
              f"required: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("RANGE", c_range)

    def c_queue():
        aged = [r for r in arb() if r["event"] == "aged_grant"]
        gl = rows(os.path.join(V, "grants_ca_tbl_full_seed7.csv"))
        committed = 0
        by = {}
        for g in gl:
            by.setdefault(g["subjectId"], []).append(g)
        for sid, ev in by.items():
            ev.sort(key=lambda g: float(g["t"]))
            for a, b in zip(ev, ev[1:]):
                if a["event"] == "queued" and b["event"] == "confirm" and a["anchorId"] == b["anchorId"]:
                    committed += 1
        check("QUEUE", len(aged) > 0 and committed > 0,
              f"grants from the queue {len(aged)} (need > 0); queued requests later confirmed by "
              f"the same anchor at the subject {committed} (need > 0)")
    safe("QUEUE", c_queue)

    def c_finders():
        fg = [r for r in arb() if r["event"] in ("grant", "preempt", "aged_grant")
              and int(r["arbiter"]) >= 501]
        check("FINDERS", len(fg) > 0, f"slots granted by a finder: {len(fg)} (need > 0)")
    safe("FINDERS", c_finders)

    def c_query():
        m = LINE4.search(log(d, "v2"))
        got = tuple(int(x) for x in m.groups()[1:]) if m else None
        ev = rows(os.path.join(V, "events_ca_tbl_full_seed7.csv"))
        qt_frames = sum(1 for r in ev if r["msgType"] == "ESC_REQUEST" and ";qt=1" in r["payload"])
        qt_arb = sum(1 for r in arb() if r["qt"] == "1")
        ok = got is not None and got[0] > 0 and got[3] > 0 and qt_frames > 0 and qt_arb > 0
        check("QUERY", ok,
              f"NW-4 line (latched, armed, fired, escalations, suppressed, ignored) {got}; "
              f"ESC_REQUEST frames with qt=1 {qt_frames}, arbiter rows with qt=1 {qt_arb} "
              f"(need latched, escalations and both > 0)")
    safe("QUERY", c_query)

    def c_counts():
        a = arb()
        grants = [r for r in a if r["event"] in ("grant", "preempt", "aged_grant")]
        queued_new, inq = 0, set()
        for r in a:
            key = (r["arbiter"], r["requester"])
            if r["event"] == "queue":
                if key not in inq:
                    queued_new += 1
                    inq.add(key)
            elif r["event"] in ("grant", "preempt", "aged_grant", "expire"):
                inq.discard(key)
        want = (len(grants), sum(int(r["arbiter"]) >= 501 for r in grants),
                sum(r["event"] == "preempt" for r in a), sum(r["event"] == "aged_grant" for r in a),
                queued_new, sum(r["event"] == "out_of_range" for r in a),
                sum(r["event"] == "expire" for r in a))
        m = LINE5.search(log(d, "v2"))
        got = tuple(int(x) for x in m.groups()[1:]) if m else None
        check("COUNTS", got == want,
              f"NW-5 line {got}; recounted {want} (grants, by finders, pre-emptions, from the "
              f"queue, queued, out of range, dropped)")
    safe("COUNTS", c_counts)

    def c_control():
        fs = [f for f in os.listdir(os.path.join(d, "c4")) if f.startswith("arb_")]
        lines = log(d, "c4").count("NW-5") + log(d, "c4").count("NW-4")
        ev = rows(os.path.join(d, "c4", "events_ca_tbl_full_seed7.csv"))
        qt = sum(1 for r in ev if ";qt=1" in r["payload"])
        check("CONTROL", not fs and lines == 0 and qt == 0,
              f"the C4 run: arb_ files {len(fs)}, NW-4/5 lines {lines}, qt frames {qt} (need 0)")
    safe("CONTROL", c_control)

    def c_ref():
        for n, need in (("r_qte", "FATAL: --queryTriggeredEscalation needs --queryV2"),
                        ("r_urg", "FATAL: --identityQueryUrgent needs --queryV2"),
                        ("r_arb", "FATAL: --arbitrationV2 needs --uwbArbitration=true")):
            got = rc(d, n)
            has = need in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G51 {tag:7s} {name:7s} {why}")
    print(f"  G51: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
