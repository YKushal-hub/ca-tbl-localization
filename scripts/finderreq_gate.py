#!/usr/bin/env python3
"""G64: a finder's session drives escalation (docs/SPEC_CONFORMANCE_PLAN.md TA-1b,
--finderRequesterEscalation; TECH_DEBT N.10 "only the base station asks").

usage: finderreq_gate.py <dir written by scripts/finderreq_gate.sh>

Runs read: on/ (the flag), off/ (the control), r_nofs (refusal: without --finderSessions).

What a firing looks like on disk (finderreq_* unless stated): one row per request a finder
made, with its route -- `own` (the finder is itself the arbiter and holds UWB-reach evidence
of the target), `named` (the arbiter the target's entry names, as TA-2's), or `unroutable`.

Checks (each prints "G64 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  ROWS     requests > 0, some sent (own or named), and some granted.
  RULE     every request follows a result in that finder's own session at that instant
           (session_*: requesterId = the finder, clean, not on UWB) with PCS below the
           threshold; at most one per finder and target per 10 s.
  ROUTE    an own request names the finder itself, which held evidence of the target (arb_*
           at that instant: evidenceAgeS >= 0); a named one names another arbiter; an
           unroutable one names none.
  ARB      each sent request is scored at its arbiter at that instant (arb_*: obo_submit,
           then the outcome logged), its query-triggered bit as the request's.
  COUNTS   the TA-1b line equals the file.
  CONTROL  off/: no finderreq_ file and no TA-1b requester line.
  R        the refusal exits 1 with the reason.
"""
import csv
import glob
import os
import re
import sys

LINE = re.compile(r"TA-1b finder requester escalation \(\w+\): requests own-arbiter (\d+), named (\d+), "
                  r"unroutable (\d+); granted (\d+), queued (\d+), refused (\d+)")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G64 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G64 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G64 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def read(p):
    try:
        return open(p).read()
    except OSError:
        return None


def rows(p):
    try:
        return list(csv.DictReader(open(p)))
    except OSError:
        return None


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    rq = rows(os.path.join(d, "on", "finderreq_ca_tbl_full_seed7.csv"))
    if rq is None:
        missing("ROWS", "no finderreq_ca_tbl_full_seed7.csv in on/")
        return 1
    sent = [r for r in rq if r["route"] != "unroutable"]
    gr = [r for r in rq if r["outcome"] == "granted"]
    report(rq and sent and gr, "ROWS", f"requests {len(rq)}, sent {len(sent)} (own {sum(r['route'] == 'own' for r in rq)}, "
           f"named {sum(r['route'] == 'named' for r in rq)}), granted {len(gr)} (need > 0 each)")

    ss = rows(os.path.join(d, "on", "session_ca_tbl_full_seed7.csv")) or []
    by = {}
    for s in ss:
        if s["event"] == "result":
            by.setdefault((s["t"], s["requesterId"]), []).append(s)
    rb, last = [], {}
    for r in rq:
        trig = [s for s in by.get((r["t"], r["finderId"]), []) if s["entryNode"] == r["target"]
                and s["suspect"] == "0" and s["uwbActive"] == "0"]
        if not trig or not float(r["pcs"]) < float(r["threshold"]):
            rb.append((r["t"], r["finderId"], r["target"], "no clean low-PCS result in its session"))
        k = (r["finderId"], r["target"])
        t = float(r["t"])
        if k in last and t - last[k] < 10.0 - 1e-9:
            rb.append((r["t"], r["finderId"], r["target"], "within 10 s of the last"))
        last[k] = t
    report(not rb, "RULE", f"{len(rq)} requests; without a clean low-PCS result in the finder's session, or too soon: "
           f"{len(rb)} (need 0)" + (f": {rb[:2]}" if rb else ""))

    arb = rows(os.path.join(d, "on", "arb_ca_tbl_full_seed7.csv")) or []
    at = {}
    for a in arb:
        at.setdefault((a["t"], a["arbiter"], a["requester"]), []).append(a)
    ob = []
    for r in rq:
        if r["route"] == "unroutable":
            if r["arbiter"] != "0" or r["outcome"] != "unroutable":
                ob.append((r["t"], r["finderId"], "unroutable yet", r["arbiter"], r["outcome"]))
            continue
        if r["route"] == "own":
            evs = [e for e in at.get((r["t"], r["arbiter"], r["target"]), [])
                   if e["event"] in ("grant", "preempt", "queue", "out_of_range", "yield")]
            if r["arbiter"] != r["finderId"] or not evs or float(evs[0]["evidenceAgeS"]) < 0.0:
                ob.append((r["t"], r["finderId"], "own without evidence", r["arbiter"]))
        elif r["route"] == "named" and r["arbiter"] in ("0", r["finderId"]):
            ob.append((r["t"], r["finderId"], "named itself or none", r["arbiter"]))
    report(not ob and sent, "ROUTE", f"{len(sent)} sent; own without the finder's evidence, named to itself, "
           f"or unroutable with an arbiter: {len(ob)} (need 0)" + (f": {ob[:2]}" if ob else ""))

    want = {"granted": ("grant", "preempt"), "queued": ("queue",), "refused": ("out_of_range", "yield")}
    ab = []
    for r in sent:
        evs = at.get((r["t"], r["arbiter"], r["target"]), [])
        names = [e["event"] for e in evs]
        sub = [e for e in evs if e["event"] == "obo_submit"]
        if not sub or not any(n in want[r["outcome"]] for n in names) or not any(e["qt"] == r["qt"] for e in sub):
            ab.append((r["t"], r["finderId"], r["outcome"], names))
    report(not ab, "ARB", f"sent requests not scored at their arbiter as logged: {len(ab)} (need 0)" + (f": {ab[:2]}" if ab else ""))

    m = LINE.search(read(os.path.join(d, "on.log")) or "")
    got = tuple(int(x) for x in m.groups()) if m else None
    wantc = (sum(r["route"] == "own" for r in rq), sum(r["route"] == "named" for r in rq),
             sum(r["route"] == "unroutable" for r in rq), sum(r["outcome"] == "granted" for r in rq),
             sum(r["outcome"] == "queued" for r in rq), sum(r["outcome"] == "refused" for r in rq))
    report(got == wantc, "COUNTS", f"TA-1b line {got}; recounted {wantc}")

    of = glob.glob(os.path.join(d, "off", "finderreq_*"))
    ol = "TA-1b finder requester escalation" in (read(os.path.join(d, "off.log")) or "")
    report(not of and not ol, "CONTROL", f"off/: finderreq_ files {len(of)}, TA-1b requester line {ol} (need 0, False)")

    t = read(os.path.join(d, "r_nofs.log")) or ""
    report(rc(d, "r_nofs") == 1 and "--finderRequesterEscalation needs" in t, "R",
           f"without --finderSessions: exit {rc(d, 'r_nofs')} (need 1)")
    print(f"  G64: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
