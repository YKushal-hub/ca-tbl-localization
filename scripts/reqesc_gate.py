#!/usr/bin/env python3
"""G57: the requester drives escalation (docs/SPEC_CONFORMANCE_PLAN.md TA-2,
--requesterEscalation).

Transport sect 4: on a RESULT with PCS below the threshold and integrity CLEAN, the
requester submits an escalation request for that target; with URGENT priority and a non-UWB
tier it is query-triggered; the anchor scores it against every pending request. Only a
node's own PCS decision ever asked. Under --requesterEscalation the base station asks, at
the arbiter the target's entry names as the one it has UWB reach to (never "the nearest
gateway"), and a grant goes to the target, which takes it 2.4 GHz -> UWB.

Reads a directory written by scripts/reqesc_gate.sh (req, noreq, r_noarb). Every check runs
and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9), from reqesc_* unless stated:
  RULE     every request follows a base-station result for that target at that instant
           (session_*: requester 0, clean, not on UWB) whose PCS is below the threshold;
           at most one per target per 10 s.
  ROUTE    a sent request goes to the arbiter the target's entry named as the one it last had
           UWB-reach evidence with, and that arbiter held evidence of the target; an
           unroutable request's entry named none.
  ARB      each sent request is scored at its arbiter at that instant (arb_*: obo_submit
           then grant, queue or out_of_range for that target), its query-triggered bit as
           the request's.
  COMMIT   some requester-obtained grant (events_*: ESC_GRANT obo=1 delivered to the
           target) was accepted there (grants_*: obo_accept from that anchor), and the
           target ran on UWB at its next cycle.
  COUNTS   the TA-2 line equals the logs.
  CONTROL  without the flag no reqesc_ file, no obo grant, no TA-2 line.
  R        --requesterEscalation without --arbitrationV2 is refused.
"""
import csv
import os
import re
import sys

LINE = re.compile(r"TA-2 requester escalation \((\w+)\): granted (\d+), queued (\d+), refused "
                  r"(\d+), unroutable (\d+); at the targets accepted (\d+), refused (\d+)")
OK_OUT = ("delivered", "capture")
DT = 2.5


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
        print("usage: reqesc_gate.py <dir>", file=sys.stderr)
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

    Q = os.path.join(d, "req")

    def req():
        return rows(os.path.join(Q, "reqesc_ca_tbl_full_seed7.csv"))

    def c_runs():
        bad = [n for n in ("req", "noreq") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_rule():
        rq = req()
        sess = [r for r in rows(os.path.join(Q, "session_ca_tbl_full_seed7.csv"))
                if r["requesterId"] == "0" and r["event"] == "result"]
        by_t = {}
        for r in sess:
            by_t.setdefault(r["t"], []).append(r)
        bad, last = [], {}
        for r in rq:
            trig = [s for s in by_t.get(r["t"], []) if s["entryNode"] == r["target"]
                    and s["suspect"] == "0" and s["uwbActive"] == "0"]
            if not trig or not float(r["pcs"]) < float(r["threshold"]):
                bad.append((r["t"], r["target"], "no clean low-PCS result behind it"))
            t = float(r["t"])
            if r["target"] in last and t - last[r["target"]] < 10.0 - 1e-9:
                bad.append((r["t"], r["target"], "within 10 s of the last"))
            last[r["target"]] = t
        check("RULE", rq and not bad, f"{len(rq)} requests; not on a clean low-PCS result or too "
                                      f"soon: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("RULE", c_rule)

    def c_route():
        arb = rows(os.path.join(Q, "arb_ca_tbl_full_seed7.csv"))
        at = {}
        for r in arb:
            if r["event"] in ("grant", "preempt", "queue", "out_of_range", "yield"):
                at.setdefault((r["t"], r["arbiter"], r["requester"]), []).append(r)
        bad, n_sent = [], 0
        for r in req():
            sent = r["outcome"] != "unroutable"
            if not sent:
                if r["arbiter"] != "0":
                    bad.append((r["t"], r["target"], "unroutable though the entry named", r["arbiter"]))
                continue
            n_sent += 1
            if r["arbiter"] == "0":
                bad.append((r["t"], r["target"], "sent with no arbiter named"))
                continue
            evs = at.get((r["t"], r["arbiter"], r["target"]), [])
            # The arbiter the entry named must have held reach evidence for the target.
            if not evs or float(evs[0]["evidenceAgeS"]) < 0.0:
                bad.append((r["t"], r["target"], r["arbiter"], "arbiter had no evidence of it"))
        check("ROUTE", n_sent > 0 and not bad,
              f"{n_sent} sent, each to the arbiter the target's entry named; to one without "
              f"evidence of the target, or misrouted: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("ROUTE", c_route)

    def c_arb():
        arb = rows(os.path.join(Q, "arb_ca_tbl_full_seed7.csv"))
        at = {}
        for r in arb:
            at.setdefault((r["t"], r["arbiter"], r["requester"]), []).append(r)
        want = {"granted": ("grant", "preempt"), "queued": ("queue",), "refused": ("out_of_range", "yield")}
        bad = []
        for r in req():
            if r["outcome"] == "unroutable":
                continue
            evs = at.get((r["t"], r["arbiter"], r["target"]), [])
            names = [e["event"] for e in evs]
            sub = [e for e in evs if e["event"] == "obo_submit"]
            if not sub or not any(n in want[r["outcome"]] for n in names) or sub[0]["qt"] != r["qt"]:
                bad.append((r["t"], r["target"], r["outcome"], names))
        check("ARB", not bad, f"requests not scored at their arbiter as logged: {len(bad)} (need 0)"
                              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("ARB", c_arb)

    def c_commit():
        ev = rows(os.path.join(Q, "events_ca_tbl_full_seed7.csv"))
        obo = [(float(r["t"]), r["txId"], r["rxId"]) for r in ev if r["msgType"] == "ESC_GRANT"
               and r["outcome"] in OK_OUT and ";obo=1" in r["payload"]]
        gl = rows(os.path.join(Q, "grants_ca_tbl_full_seed7.csv"))
        acc = [g for g in gl if g["event"] == "obo_accept"]
        traces = {"1": {round(float(r["t"]) / DT): r["activeTier"]
                        for r in rows(os.path.join(Q, "trace_ca_tbl_full_seed7.csv"))}}
        for k in range(2, 6):
            p = os.path.join(Q, f"subjtrace_ca_tbl_full_seed7_subj{k}.csv")
            if os.path.exists(p):
                traces[str(k)] = {round(float(r["t"]) / DT): r["activeTier"] for r in rows(p)}
        good, bad = 0, []
        for g in acc:
            tg = float(g["t"])
            if not any(abs(te - tg) < 1e-5 * max(1.0, tg) and tx == g["anchorId"] and rx == g["subjectId"]
                       for te, tx, rx in obo):
                bad.append((g["t"], g["subjectId"], "no delivered obo grant"))
                continue
            nxt = traces.get(g["subjectId"], {}).get(int(tg // DT) + 1)
            if nxt == "UWB":
                good += 1
            else:
                bad.append((g["t"], g["subjectId"], "next cycle on", nxt))
        check("COMMIT", good > 0 and not bad,
              f"obo grants delivered {len(obo)}; accepted {len(acc)}, then on UWB {good} (need > 0); "
              f"inconsistent {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("COMMIT", c_commit)

    def c_counts():
        m = LINE.search(log(d, "req"))
        got = tuple(int(x) for x in m.groups()[1:]) if m else None
        rq = req()
        gl = rows(os.path.join(Q, "grants_ca_tbl_full_seed7.csv"))
        want = (sum(r["outcome"] == "granted" for r in rq), sum(r["outcome"] == "queued" for r in rq),
                sum(r["outcome"] == "refused" for r in rq), sum(r["outcome"] == "unroutable" for r in rq),
                sum(g["event"] == "obo_accept" for g in gl), sum(g["event"] == "obo_refuse" for g in gl))
        check("COUNTS", got == want, f"TA-2 line {got}; recounted {want}")
    safe("COUNTS", c_counts)

    def c_control():
        fs = [f for f in os.listdir(os.path.join(d, "noreq")) if f.startswith("reqesc_")]
        ev = rows(os.path.join(d, "noreq", "events_ca_tbl_full_seed7.csv"))
        obo = sum(1 for r in ev if ";obo=1" in r["payload"])
        lines = log(d, "noreq").count("TA-2")
        check("CONTROL", not fs and obo == 0 and lines == 0,
              f"without the flag: reqesc_ files {len(fs)}, obo grants {obo}, TA-2 lines {lines} (need 0)")
    safe("CONTROL", c_control)

    def c_ref():
        got = rc(d, "r_noarb")
        has = "FATAL: --requesterEscalation needs --sessionOverMesh, --arbitrationV2" in log(d, "r_noarb")
        check("R", got == 1 and has, f"r_noarb: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G57 {tag:7s} {name:7s} {why}")
    print(f"  G57: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
