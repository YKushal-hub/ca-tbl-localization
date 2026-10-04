#!/usr/bin/env python3
"""G33: the escalation handshake as MAC sect 8 specifies it
(docs/SPEC_CONFORMANCE_PLAN.md MC-1, --handshakeFixes).

The spec: "Tier escalation request (UWB specifically): Yes [ACK'd] -- Must know
definitively whether the grant was received". By default five things stop that holding:
an ACK clears its receiver's OLDEST pending frame; an anchor ACKs a request BEFORE
arbitrating, so a denied request is settled and never retried; the pre-emption send
carries no terms; the other sends carry the previous cycle's terms; and 868 -> 2.4 GHz
escalations and de-escalations are signalled as well. --handshakeFixes fixes all five.

Reads a directory written by scripts/handshake_fixes_gate.sh: runs def, off, fix (NC9's
regime, seed 1, 600 s, handshake frames, arbitration, two peer tags) and r_noframes. The
measured tag is node 1; nodes 2 and 3 are peers whose PeerRequestDriver sends synthetic
requests (retired in MS-1), so the per-request checks below are on node 1's requests. Every
check runs and is reported; exit 1 if any failed, 2 if an input is missing.

The events file has one row per RECEPTION (t, tier, txId, rxId, msgType, outcome, ...,
payload), so a frame appears once per radio that heard it; frames are identified by pkt=.
An "original" request is node 1's first request in the first 0.5 s of a trace cycle (the
send follows the decision by CAD time, about 10 ms). Retransmissions follow at the 2 s ACK
timeout, at most twice. A frame is keyed by (tier, sender, receiver, pkt): a node's three
radios are three MACs whose packet-id counters all start at node_id << 20, so the same
pkt can name a GRANT on one tier and a DISCOVERY on another.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    --handshakeFixes=false IS the default: every output file byte-identical; the
          default prints no MC-1 line and no event payload carries target=, acks= or
          granted=.
  DEFECT  the default, observed: anchors (ids 101-199) send bare ACKs to the requester
          right after hearing its request (> 0), and node 1 sends original requests that
          some radio heard on de-escalation cycles, on escalations that are not into UWB,
          or on the cycle after one (the pre-emption send, which carries degr=0;batt=0)
          (> 0). Only heard frames are in the events file: a de-escalation request leaves
          on the radio being abandoned, usually with nobody in range, so this count is a
          floor.
  TARGET  under the flag every node-1 request carries target=2 (> 0 of them).
  ONLYUWB under the flag no node-1 original request sits on a cycle other than an
          escalation into UWB or the cycle after one (the pre-emption send), and every
          node-1 request falls within 7.5 s (dt + two 2 s ACK timeouts + 1 s of CAD)
          after an escalation into UWB.
  NOACK   under the flag every ACK names the frame it acknowledges (acks=P), and P is an
          ESC_GRANT or RESPONSE the ACK's receiver sent to the ACK's sender -- never a
          request, which is answered by the GRANT instead.
  GRANT   under the flag every GRANT says granted=0|1 and names (acks=P) a request its
          receiver sent and its sender heard, no later than the GRANT; both verdicts occur.
          Denials heard and retransmissions are both > 0 in the MC-1 line.
  TERMS   the request terms, RECOMPUTED from the trace: T_deg from the last five `pcs`
          values (the C4 slope form) and T_batt from `cumJ` against the 1000 J default cap.
          Under the flag a node-1 original carries THIS cycle's T_deg (the history
          including this cycle's pcs) to within one quantisation step, and a T_batt between
          the previous and this cycle's cumJ; the default carries the PREVIOUS cycle's
          T_deg. Cases where the two differ by more than one step must exist (> 0), or the
          check could not tell them apart.
  R       --handshakeFixes without --sendHandshakeFrames is refused (exit 1).
"""
import csv
import math
import os
import re
import sys

CAP_J = 1000.0  # --arbBatteryCapJ default
WINDOW_S = 2.5 + 2 * 2.0 + 1.0
ANCHORS = range(101, 200)
MC1 = re.compile(r"MC-1 handshake: ESC_REQUEST sent (\d+) \(retransmitted (\d+)\), bare ACKs to a "
                 r"request withheld (\d+); GRANT sent granted (\d+) denied (\d+); heard granted (\d+) "
                 r"denied (\d+), requests settled by a grant (\d+); ACKs matched by id (\d+), "
                 r"unmatched (\d+)")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def q(x):
    if not x > 0.0:
        return 0
    if x >= 1.0:
        return 255
    return int(x * 255.0 + 0.5)


def slope_term(h):
    n = len(h)
    sx = sy = sxx = sxy = 0.0
    for i, v in enumerate(h):
        sx += i
        sy += v
        sxx += i * i
        sxy += i * v
    den = n * sxx - sx * sx
    s = (n * sxy - sx * sy) / den if den != 0.0 else 0.0
    return 1.0 - math.exp(-max(0.0, -s))


def field(p, k):
    m = re.search(r"(?:^|;)" + k + r"=(\d+)", p)
    return int(m.group(1)) if m else None


def load(d, n):
    tr = list(csv.DictReader(open(os.path.join(d, n, "trace_ca_tbl_full_seed1.csv"))))
    ev = list(csv.DictReader(open(os.path.join(d, n, "events_ca_tbl_full_seed1.csv"))))
    for r in ev:
        r["pkt"] = field(r["payload"], "pkt")
        r["t"] = float(r["t"])
    return tr, ev


def requests(ev, node="1"):
    """First reception of each of node's ESC_REQUEST frames: [(t, payload)], by time."""
    seen = {}
    for r in ev:
        if r["msgType"] == "ESC_REQUEST" and r["txId"] == node and r["pkt"] not in seen:
            seen[r["pkt"]] = (r["t"], r["payload"])
    return sorted(seen.values())


def cycles(tr):
    t = [float(r["t"]) for r in tr]
    uwb = [i for i, r in enumerate(tr) if r["escalated"] == "1" and r["nextTier"] == "UWB"]
    return t, uwb


def row_of(t, x):
    """The trace row whose first 0.5 s holds x, or None."""
    for i, ti in enumerate(t):
        if ti <= x < ti + 0.5:
            return i
    return None


def terms(tr):
    """Per row k: (fresh T_deg, stale T_deg), quantised -- the C4 update replayed."""
    hist, prev, out = [], 0.0, []
    for r in tr:
        h = (hist + [float(r["pcs"])])[-5:]
        fresh = slope_term(h) if len(h) >= 3 else prev
        out.append((q(fresh), q(prev)))
        hist, prev = h, fresh
    return out


def main():
    if len(sys.argv) != 2:
        print("usage: handshake_fixes_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def c_runs():
        bad = [n for n in ("def", "off", "fix") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fo = sorted(f for f in os.listdir(os.path.join(d, "off")) if f != "run_id.txt")
        same = fd == fo and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "off", f), "rb").read() for f in fd)
        lines = log(d, "def").count("MC-1")
        ev = open(os.path.join(d, "def", "events_ca_tbl_full_seed1.csv")).read()
        marks = sum(ev.count(k) for k in (";target=", ";acks=", ";granted="))
        check("NULL", same and fd and lines == 0 and marks == 0,
              f"--handshakeFixes=false vs default: {len(fd)} files, "
              f"{'identical' if same else 'DIFFER'}; MC-1 lines under the default {lines} "
              f"(need 0); payload marks {marks} (need 0)")
    safe("NULL", c_null)

    def c_defect():
        tr, ev = load(d, "def")
        t, uwb = cycles(tr)
        heard = {}
        for r in ev:
            if r["msgType"] == "ESC_REQUEST" and int(r["rxId"]) in ANCHORS:
                heard.setdefault((r["rxId"], r["txId"]), []).append(r["t"])
        bare = sum(1 for r in ev if r["msgType"] == "ACK" and int(r["txId"]) in ANCHORS and
                   any(0.0 <= r["t"] - x < 0.5 for x in heard.get((r["txId"], r["rxId"]), [])))
        down = {i for i, r in enumerate(tr) if r["deescalated"] == "1"}
        lateral = {i for i, r in enumerate(tr) if r["escalated"] == "1" and r["nextTier"] != "UWB"}
        other = down | lateral | {i + 1 for i in lateral}
        stray = [(x, field(p, "degr"), field(p, "batt")) for x, p in requests(ev)
                 if row_of(t, x) in other]
        check("DEFECT", bare > 0 and len(stray) > 0,
              f"default: {bare} anchor ACK receptions answering a request (need > 0); "
              f"{len(stray)} node-1 originals on de-escalation or non-UWB escalation cycles "
              f"or the cycle after one (need > 0), (t, degr, batt) = {stray[:3]}")
    safe("DEFECT", c_defect)

    def c_target():
        _, ev = load(d, "fix")
        rq = requests(ev)
        bad = [x for x, p in rq if field(p, "target") != 2]
        check("TARGET", rq and not bad,
              f"fix: {len(rq)} node-1 requests, {len(bad)} without target=2 (need 0)")
    safe("TARGET", c_target)

    def c_onlyuwb():
        tr, ev = load(d, "fix")
        t, uwb = cycles(tr)
        ok_rows = set(uwb) | {i + 1 for i in uwb}
        rq = requests(ev)
        stray = [x for x, _ in rq if row_of(t, x) is not None and row_of(t, x) not in ok_rows]
        far = [x for x, _ in rq if not any(0.0 <= x - t[i] <= WINDOW_S for i in uwb)]
        check("ONLYUWB", rq and uwb and not stray and not far,
              f"fix: {len(uwb)} escalations into UWB, {len(rq)} node-1 requests; originals "
              f"off those cycles {len(stray)} (need 0); requests more than {WINDOW_S} s after "
              f"one {len(far)} (need 0)")
    safe("ONLYUWB", c_onlyuwb)

    def c_noack():
        _, ev = load(d, "fix")
        sent = {}
        for r in ev:
            sent[(r["tier"], r["txId"], r["rxId"], r["pkt"])] = r["msgType"]
        acks = [r for r in ev if r["msgType"] == "ACK"]
        bad = []
        for r in acks:
            p = field(r["payload"], "acks")
            what = sent.get((r["tier"], r["rxId"], r["txId"], p))
            if p is None or what not in ("ESC_GRANT", "RESPONSE"):
                bad.append((r["t"], r["tier"], r["txId"], r["rxId"], p, what))
        check("NOACK", acks and not bad,
              f"fix: {len(acks)} ACK receptions, {len(bad)} not naming a GRANT/RESPONSE their "
              f"receiver sent their sender (need 0){': ' + str(bad[:3]) if bad else ''}")
    safe("NOACK", c_noack)

    def c_grant():
        _, ev = load(d, "fix")
        req = {}
        for r in ev:
            if r["msgType"] == "ESC_REQUEST":
                req.setdefault((r["tier"], r["txId"], r["rxId"], r["pkt"]), r["t"])
        gr = [r for r in ev if r["msgType"] == "ESC_GRANT"]
        bad, verdicts = [], set()
        for r in gr:
            p, g = field(r["payload"], "acks"), field(r["payload"], "granted")
            heard = req.get((r["tier"], r["rxId"], r["txId"], p))
            if g not in (0, 1) or heard is None or heard > r["t"]:
                bad.append((r["t"], r["txId"], r["rxId"], p, g))
            verdicts.add(g)
        m = MC1.search(log(d, "fix"))
        retx, dh = (int(m.group(2)), int(m.group(7))) if m else (0, 0)
        check("GRANT", gr and not bad and verdicts == {0, 1} and m and retx > 0 and dh > 0,
              f"fix: {len(gr)} GRANT receptions, {len(bad)} not naming a request their "
              f"receiver sent their sender earlier (need 0); verdicts {sorted(verdicts)} "
              f"(need [0, 1]); MC-1 line: denials heard {dh}, retransmitted {retx} (need > 0)")
    safe("GRANT", c_grant)

    def c_terms():
        out = []
        disc = 0
        for n, want in (("fix", 0), ("def", 1)):
            tr, ev = load(d, n)
            t, uwb = cycles(tr)
            tm = terms(tr)
            cj = [float(r["cumJ"]) for r in tr]
            good = bad = 0
            for x, p in requests(ev):
                k = row_of(t, x)
                if k is None or k not in uwb:
                    continue
                dg, bt = field(p, "degr"), field(p, "batt")
                fresh, stale = tm[k]
                if n == "fix":
                    disc += abs(fresh - stale) > 1
                ok = abs(dg - tm[k][want]) <= 1
                if n == "fix":
                    ok = ok and q(cj[k - 1] / CAP_J) - 1 <= bt <= q(cj[k] / CAP_J) + 1
                good += ok
                bad += not ok
            out.append((n, good, bad))
        ok = all(g > 0 and b == 0 for _, g, b in out) and disc > 0
        check("TERMS", ok,
              f"originals on escalation-into-UWB cycles: fix {out[0][1]} carry this cycle's "
              f"T_deg and T_batt, {out[0][2]} do not; default {out[1][1]} carry the previous "
              f"cycle's T_deg, {out[1][2]} do not (need 0 and 0); cases telling the two "
              f"apart {disc} (need > 0)")
    safe("TERMS", c_terms)

    def c_ref():
        got = rc(d, "r_noframes")
        has = "--handshakeFixes needs --sendHandshakeFrames=true" in log(d, "r_noframes")
        check("R", got == 1 and has,
              f"r_noframes: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G33 {tag:7s} {name:7s} {why}")
    print(f"  G33: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
