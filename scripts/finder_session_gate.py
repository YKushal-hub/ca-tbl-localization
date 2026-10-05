#!/usr/bin/env python3
"""G56: every finder runs its own search (docs/SPEC_CONFORMANCE_PLAN.md TA-1b,
--finderSessions).

Transport sect 2: "a rescuer's handheld ... issues a search". Only the base station searched
(TA-1a). Under --finderSessions finder k opens a session for subject (k mod N) + 1, queries
for it from its own tier-1 radio, and is fed only by the answers that reach it as the
requester -- over the reverse path to a moving node. A finder within 2 m of its target
cancels its session (Transport sect 2's explicit cancel; the 2 m is OURS).

Reads a directory written by scripts/finder_session_gate.sh (fs, nofs, r_nobase). Every check
runs and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9), from session_* rows with requesterId >= 501:
  FEED     every finder result is an answer that reached that finder as its requester
           (responses_ requester = the finder; events_ RESPONSE delivered to it with req =
           it), once per (query, answerer, manifest) per finder; every such answer is fed.
  TARGET   finder k's results are all for subject (k mod 5) + 1, and so is its TA-1b line.
  COARSE   some finder session left SEARCHING on a result row.
  CANCEL   in both runs with finder sessions, every cancel was within the run's cancel
           distance (2 m, or 30 m in fsc, G47's converging regime) and closed the session;
           nothing moved it
           afterwards; the TA-1b line's cancel time is the cancel row's (or -1 with none).
           Non-vacuous: fsc cancels at least one session.
  LINE     each finder's TA-1b answer count equals its result rows.
  CONTROL  without the flag no finder row and no TA-1b line; the base station's rows exist
           in both runs.
  R        --finderSessions without --sessionOverMesh is refused.
"""
import csv
import os
import re
import sys

LINE = re.compile(r"TA-1b finder session \((\w+)\): finder (\d+) target m(\d+), answers (\d+), "
                  r"path (\S+), first coarse/precise \(s\) (-?[0-9.]+)/(-?[0-9.]+), cancelled "
                  r"(-?[0-9.]+)")
OK_OUT = ("delivered", "capture")
N = 5


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
        print("usage: finder_session_gate.py <dir>", file=sys.stderr)
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

    F = os.path.join(d, "fs")
    st = {}

    def load():
        if "s" not in st:
            allrows = rows(os.path.join(F, "session_ca_tbl_full_seed7.csv"))
            st["s"] = [r for r in allrows if int(r["requesterId"]) >= 501]
            st["base"] = [r for r in allrows if r["requesterId"] == "0"]
            st["r"] = [r for r in rows(os.path.join(F, "responses_ca_tbl_full_seed7.csv"))
                       if int(r["requesterId"]) >= 501 and r["entryNode"] != "0"]
            ev = set()
            for r in rows(os.path.join(F, "events_ca_tbl_full_seed7.csv")):
                if r["msgType"] != "RESPONSE" or r["outcome"] not in OK_OUT:
                    continue
                p = payload(r["payload"])
                if "rq" in p and int(r["rxId"]) >= 501 and r["rxId"] == p.get("req"):
                    ev.add((r["rxId"], p["rq"], p["ans"], p.get("rm", "0")))
            st["ev"] = ev
            st["lines"] = {m.group(2): m for m in LINE.finditer(log(d, "fs"))}
        return st

    def c_runs():
        bad = [n for n in ("fs", "nofs", "fsc") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_feed():
        s = load()
        res_rows = [r for r in s["s"] if r["event"] == "result"]
        keys = [(r["requesterId"], r["queryId"], r["answererId"], r["manifest"]) for r in res_rows]
        delivered = {(r["requesterId"], r["queryId"], r["answererId"], r["manifest"]) for r in s["r"]}
        bad = [k for k in keys if k not in delivered or k not in s["ev"]]
        missing = [k for k in delivered if k not in set(keys)]
        dup = len(keys) - len(set(keys))
        check("FEED", res_rows and not bad and not missing and dup == 0,
              f"{len(res_rows)} finder results; not a delivery to that finder {len(bad)}, "
              f"deliveries never fed {len(missing)}, duplicates {dup} (need 0, 0, 0)"
              f"{': ' + str((bad + missing)[:2]) if bad or missing else ''}")
    safe("FEED", c_feed)

    def c_target():
        s = load()
        bad = []
        for r in s["s"]:
            if r["event"] == "result":
                k = int(r["requesterId"]) - 501
                if r["manifest"] != str(k % N + 1):
                    bad.append((r["requesterId"], r["manifest"]))
        for fid, m in s["lines"].items():
            if m.group(3) != str((int(fid) - 501) % N + 1):
                bad.append((fid, "line target", m.group(3)))
        check("TARGET", s["lines"] and not bad,
              f"finder lines {len(s['lines'])}; results or lines not for (k mod 5) + 1: {len(bad)} "
              f"(need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("TARGET", c_target)

    def c_coarse():
        s = load()
        left = [r for r in s["s"] if r["event"] == "result" and r["stateBefore"] == "SEARCHING"
                and r["stateAfter"] != "SEARCHING"]
        check("COARSE", len(left) > 0,
              f"finder sessions that left SEARCHING on a delivered answer: "
              f"{sorted({r['requesterId'] for r in left})} (need some)")
    safe("COARSE", c_coarse)

    def c_cancel():
        bad, ns = [], {}
        for run, lim in (("fs", 2.0), ("fsc", 30.0)):
            srows = [r for r in rows(os.path.join(d, run, "session_ca_tbl_full_seed7.csv"))
                     if int(r["requesterId"]) >= 501]
            lines = {m.group(2): m for m in LINE.finditer(log(d, run))}
            cancelled = {}
            for r in srows:
                fid = r["requesterId"]
                if r["event"] == "cancel":
                    if not (float(r["distM"]) < lim and r["stateAfter"] == "CLOSED"):
                        bad.append((run, r["t"], fid, r["distM"], r["stateAfter"]))
                    cancelled[fid] = float(r["t"])
                elif fid in cancelled and r["stateAfter"] != r["stateBefore"]:
                    bad.append((run, r["t"], fid, "moved after cancel"))
            for fid, m in lines.items():
                want = cancelled.get(fid, -1.0)
                if abs(float(m.group(8)) - want) > 0.05:
                    bad.append((run, fid, "line cancel", m.group(8), want))
            ns[run] = len(cancelled)
        check("CANCEL", not bad and ns.get("fsc", 0) > 0,
              f"cancels at 2 m {ns.get('fs')}, at 30 m {ns.get('fsc')} (need > 0 at 30 m); not "
              f"within the distance and closing, a move after one, or a line that disagrees: "
              f"{len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("CANCEL", c_cancel)

    def c_line():
        s = load()
        bad = []
        for fid, m in s["lines"].items():
            want = sum(1 for r in s["s"] if r["requesterId"] == fid and r["event"] == "result")
            if int(m.group(4)) != want:
                bad.append((fid, m.group(4), want))
        check("LINE", s["lines"] and not bad, f"TA-1b answer counts not the result rows: "
                                              f"{bad or 'none'}")
    safe("LINE", c_line)

    def c_control():
        s = load()
        nr = rows(os.path.join(d, "nofs", "session_ca_tbl_full_seed7.csv"))
        finder_rows = sum(1 for r in nr if int(r["requesterId"]) >= 501)
        lines = log(d, "nofs").count("TA-1b")
        base_nofs = sum(1 for r in nr if r["requesterId"] == "0")
        check("CONTROL", finder_rows == 0 and lines == 0 and base_nofs > 0 and s["base"],
              f"without the flag: finder rows {finder_rows}, TA-1b lines {lines} (need 0, 0); "
              f"base-station rows with/without {len(s['base'])}/{base_nofs} (need > 0)")
    safe("CONTROL", c_control)

    def c_ref():
        got = rc(d, "r_nobase")
        has = "FATAL: --finderSessions needs --sessionOverMesh" in log(d, "r_nobase")
        check("R", got == 1 and has, f"r_nobase: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G56 {tag:7s} {name:7s} {why}")
    print(f"  G56: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
