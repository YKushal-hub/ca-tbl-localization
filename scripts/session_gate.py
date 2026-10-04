#!/usr/bin/env python3
"""G54: the base station's session is fed by delivered RESPONSEs
(docs/SPEC_CONFORMANCE_PLAN.md TA-1a, --sessionOverMesh).

Transport sect 2: "a rescuer's handheld/base station issues a search", and "ON any RESPONSE
received for this session: result = extract_position_and_provenance(RESPONSE)". The session
was opened by the measured tag for itself and fed its own EKF estimate every cycle. Under
--sessionOverMesh the base station (requester 0, behind the gateway backhaul) opens one
session over what its queries ask, and is fed only by RESPONSEs delivered to a gateway, once
per answer, the RESULT built from the delivered entry.

Reads a directory written by scripts/session_gate.sh (mesh, legacy, r_nosess, r_stale).
Every check runs and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  FEED     every result row is an answer a GATEWAY received (responses_*, requester 101-499;
           and an events_* RESPONSE delivered to that gateway with the same query, answerer
           and manifest), once per (query, answerer, manifest); and every such answer is
           there.
  CONTENT  each RESULT is that delivered entry: entry node, PCS (its byte / 255), integrity
           verdict, fix tier and hop count.
  STATE    the rows chain (each stateBefore is the previous stateAfter, from SEARCHING); the
           session leaves SEARCHING on a result row -- a delivered answer, not a tick.
  TTFF     the TA-1 line's first-coarse time per target equals the first non-suspect result
           for it (1e-3), and is > 0 and finite for at least one target.
  CONTROL  the legacy feed writes no session_ file and prints no TA-1 line.
  R        the two refusals.
"""
import csv
import math
import os
import re
import sys

LINE = re.compile(r"TA-1 base-station session \((\w+)\): answers delivered (\d+), path (\S+); "
                  r"first coarse/precise \(s\):(.*)$", re.M)
OK_OUT = ("delivered", "capture")


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
        print("usage: session_gate.py <dir>", file=sys.stderr)
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

    M = os.path.join(d, "mesh")
    st = {}

    def load():
        if "s" not in st:
            st["s"] = rows(os.path.join(M, "session_ca_tbl_full_seed7.csv"))
            st["r"] = [r for r in rows(os.path.join(M, "responses_ca_tbl_full_seed7.csv"))
                       if 101 <= int(r["requesterId"]) <= 499 and r["entryNode"] != "0"]
            ev = set()
            for r in rows(os.path.join(M, "events_ca_tbl_full_seed7.csv")):
                if r["msgType"] != "RESPONSE" or r["outcome"] not in OK_OUT:
                    continue
                p = payload(r["payload"])
                if "rq" in p and 101 <= int(r["rxId"]) <= 499 and r["rxId"] == p.get("req"):
                    ev.add((p["rq"], p["ans"], p.get("rm", "0")))
            st["ev"] = ev
        return st

    def c_runs():
        bad = [n for n in ("mesh", "legacy") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_feed():
        s = load()
        results = [r for r in s["s"] if r["event"] == "result"]
        keys = [(r["queryId"], r["answererId"], r["manifest"]) for r in results]
        delivered = {(r["queryId"], r["answererId"], r["manifest"]) for r in s["r"]}
        not_delivered = [k for k in keys if k not in delivered or k not in s["ev"]]
        missing = [k for k in delivered if k not in set(keys)]
        dup = len(keys) - len(set(keys))
        check("FEED", results and not not_delivered and not missing and dup == 0,
              f"{len(results)} results; not a gateway delivery {len(not_delivered)}, "
              f"deliveries never fed {len(missing)}, duplicates {dup} (need 0, 0, 0)"
              f"{': ' + str((not_delivered + missing)[:2]) if not_delivered or missing else ''}")
    safe("FEED", c_feed)

    def c_content():
        s = load()
        by = {}
        for r in s["r"]:
            by.setdefault((r["queryId"], r["answererId"], r["manifest"]), r)
        bad = []
        for r in s["s"]:
            if r["event"] != "result":
                continue
            e = by.get((r["queryId"], r["answererId"], r["manifest"]))
            if e is None:
                continue
            want = (e["entryNode"], round(int(e["pcsQ"]) / 255.0, 4), e["integrity"],
                    e["tierSource"], e["fixHop"])
            got = (r["entryNode"], round(float(r["pcs"]), 4), r["suspect"], r["tierSource"],
                   r["hopCountAtFix"])
            if want != got:
                bad.append((r["t"], got, want))
        check("CONTENT", not bad, f"results not the delivered entry: {len(bad)} (need 0)"
                                  f"{': ' + str(bad[:2]) if bad else ''}")
    safe("CONTENT", c_content)

    def c_state():
        s = load()
        bad = []
        prev = "SEARCHING"
        left_on = None
        for r in s["s"]:
            if r["stateBefore"] != prev:
                bad.append((r["t"], "chain", prev, r["stateBefore"]))
            if left_on is None and r["stateBefore"] == "SEARCHING" and r["stateAfter"] != "SEARCHING":
                left_on = r["event"]
            prev = r["stateAfter"]
        check("STATE", not bad and left_on == "result",
              f"broken chain links {len(bad)} (need 0); left SEARCHING on a {left_on} row (need "
              f"result){': ' + str(bad[:2]) if bad else ''}")
    safe("STATE", c_state)

    def c_ttff():
        s = load()
        m = LINE.search(log(d, "mesh"))
        if not m:
            check("TTFF", False, "no TA-1 line")
            return
        firsts = {}
        for r in s["s"]:
            if r["event"] == "result" and r["suspect"] == "0" and r["manifest"] not in firsts:
                firsts[r["manifest"]] = float(r["t"])
        bad, ok_any = [], False
        for mm in re.finditer(r"m(\d+) (-?[0-9.]+)/(-?[0-9.]+)", m.group(4)):
            man, coarse = mm.group(1), float(mm.group(2))
            want = firsts.get(man)
            if want is None:
                if coarse >= 0.0:
                    bad.append((man, coarse, None))
                continue
            if abs(coarse - want) > 0.05:  # the line prints 0.1 s
                bad.append((man, coarse, want))
            ok_any = ok_any or (0.0 < coarse < math.inf)
        check("TTFF", not bad and ok_any,
              f"first-coarse times not the first delivered result: {len(bad)} (need 0); a "
              f"finite positive time-to-first-fix {'present' if ok_any else 'ABSENT'}"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("TTFF", c_ttff)

    def c_line():
        s = load()
        m = LINE.search(log(d, "mesh"))
        got = int(m.group(2)) if m else None
        want = sum(1 for r in s["s"] if r["event"] == "result")
        check("LINE", got == want, f"TA-1 line answers {got}; result rows {want}")
    safe("LINE", c_line)

    def c_control():
        fs = [f for f in os.listdir(os.path.join(d, "legacy")) if f.startswith("session_")]
        lines = log(d, "legacy").count("TA-1 base-station")
        check("CONTROL", not fs and lines == 0,
              f"legacy feed: session_ files {len(fs)}, TA-1 lines {lines} (need 0, 0)")
    safe("CONTROL", c_control)

    def c_ref():
        for n, need in (("r_nosess", "FATAL: --sessionOverMesh needs --sessionLayer"),
                        ("r_stale", "FATAL: --sessionStalenessS and --sessionTimeoutS")):
            got = rc(d, n)
            has = need in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G54 {tag:7s} {name:7s} {why}")
    print(f"  G54: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
