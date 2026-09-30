#!/usr/bin/env python3
"""L.1.1 / G18, G18b -- position fixes are ROUTED, and the two metrics are seen to disagree.

WHY THIS EXISTS. CaTblRoutingTable, its ETX baseline, the confidence metric and
Divergences() had a passing unit suite and ZERO scenario call sites: every check passed
and nothing was ever routed. A unit suite proves the selection rule; it cannot prove a run
calls it. This gate reads what a RUN wrote and asks whether routing happened there.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9). With --fixRouting the scenario writes
fix_routing_<arm>_seed<N>.csv, one row per hop decision:

    t,originId,hop,senderId,etxNext,etxHops,etxCost,confNext,confHops,confCost,
    metric,chosenNext,chosenClass,diverged,outcome

The defect this guards against has the form of ABSENCE -- the router unwired again, every
relay in reach of a gateway so no route is ever relayed, the two metrics silently
identical -- so it is checked by COUNTING rows of a file a fresh run must produce (rule
12), never by scanning a file that might simply be missing.

WHAT IT CHECKS
  1. The file exists, has the header above and at least one data row.
  2. Every row is internally consistent: the next hop taken is the pick of the metric the
     row says was active; `diverged` is exactly (etxNext != confNext); a `delivered` hop
     ends at a gateway and a `forwarded` hop at a relay; gateway ids are 101-499 and relay
     ids 2-100 (the id plan in CLAUDE.md: field nodes 1, 2+; gateways 101+; finders 501+,
     which are never routers); `noroute` rows name no next hop.
  3. Every multi-hop path is continuous: within one (t, originId) the hops run 0, 1, 2...,
     hop 0 is sent by the originator, and hop k>0 is sent by the relay the previous row
     FORWARDED to. A relayed hop with no forward behind it is a route from nowhere.
  4. The L.1.1 done-when: a hop whose next hop is NOT the destination (a relay, not a
     gateway) actually carried a fix; a fix that crossed a relay was delivered; and at
     least one hop where ETX and the confidence metric chose differently.
  5. --require-confidence (G18b): every row was routed by the confidence metric, and at
     least one DIVERGED hop was carried (forwarded or delivered) -- position-fix traffic
     going where the confidence metric sent it and ETX would not have.
  6. --stdout LOG (rule 3, recount from raw): the run's own printed counters -- the
     router's and the sum of CaTblRoutingTable::Selections()/Divergences() over every
     table -- must equal the counts recomputed here from the CSV. A CSV that drifted from
     the in-memory counters, or counters that drifted from the table, fails.
  7. --require-round (needs --stdout): the relays' ranging round ran (C5_INSTANT,
     --relaySlots). It has been on by default in every relayed run since 2026-09-30, and
     G18 and G18b pass this so that they test that default rather than co-occur with it:
     the log must carry exactly one `C5_INSTANT relay round:` line whose slots add up to
     a round shorter than the cycle and which moved anchor polls (1 <= bursts <= polls).
     With --relaySlots=false the run prints no round and fails here -- the state every
     relayed run was in before, including G18's own until that date.

usage: fix_routing_gate.py [--require-confidence] [--require-round] [--stdout LOG]
                           <fix_routing csv>
exit 0 PASS, 1 FAIL, 2 usage.
"""
import csv
import re
import sys

# The round's own line, as RelayRound::Report prints it (the same pattern G17b reads).
ROUND_RE = re.compile(
    r"C5_INSTANT relay round: slot ([0-9.]+) ms x (\d+) = ([0-9.]+) ms before the instant, "
    r"of a ([0-9.]+) ms cycle; anchor polls moved (\d+) in (\d+) bursts; relay-to-relay "
    r"moved (\d+), at most (\d+) of (\d+) slots in a cycle")

COLS =["t", "originId", "hop", "senderId", "etxNext", "etxHops", "etxCost", "confNext",
        "confHops", "confCost", "metric", "chosenNext", "chosenClass", "diverged", "outcome"]
OUTCOMES = {"delivered", "forwarded", "lost", "loop", "noslot", "duty", "noroute"}


def is_gateway(i):
    return 101 <= i <= 499


def is_relay(i):
    return 2 <= i <= 100


def main(argv):
    args = argv[1:]
    need_conf = False
    need_round = False
    stdout_log = None
    while args and args[0].startswith("--"):
        a = args.pop(0)
        if a == "--require-confidence":
            need_conf = True
        elif a == "--require-round":
            need_round = True
        elif a == "--stdout" and args:
            stdout_log = args.pop(0)
        else:
            print(__doc__)
            return 2
    if len(args) != 1 or (need_round and stdout_log is None):
        print(__doc__)
        return 2
    path = args[0]
    try:
        with open(path, newline="") as fh:
            rd = csv.reader(fh)
            header = next(rd, None)
            rows = [r for r in rd if r]
    except OSError as e:
        print(f"G18 FAIL: cannot read {path}: {e} -- the flag-on run wrote no routing log")
        return 1
    if header != COLS:
        print(f"G18 FAIL: header {header} is not {COLS}")
        return 1
    print(f"G18 rows: {len(rows)} in {path}")
    if not rows:
        print("G18 FAIL: zero hop rows -- no relay ever tried to route a fix")
        return 1

    bad = []
    metrics = set()
    sel = div = relay_carried = relayed_delivered = div_carried = 0
    prev = None  # previous row of the same (t, origin) walk
    for n, r in enumerate(rows, start=2):
        if len(r) != len(COLS):
            bad.append(f"line {n}: {len(r)} fields")
            prev = None
            continue
        d = dict(zip(COLS, r))
        try:
            hop = int(d["hop"])
            origin = int(d["originId"])
            sender = int(d["senderId"])
            etx_n = int(d["etxNext"])
            conf_n = int(d["confNext"])
            chosen = int(d["chosenNext"])
            dv = int(d["diverged"])
        except ValueError:
            bad.append(f"line {n}: non-integer id/hop/diverged field")
            prev = None
            continue
        out = d["outcome"]
        metrics.add(d["metric"])
        if out not in OUTCOMES:
            bad.append(f"line {n}: unknown outcome {out!r}")
        if d["metric"] not in ("etx", "confidence"):
            bad.append(f"line {n}: unknown metric {d['metric']!r}")

        # 3. path continuity
        key = (d["t"], origin)
        if hop == 0:
            if sender != origin:
                bad.append(f"line {n}: hop 0 sent by {sender}, not the originator {origin}")
        else:
            if (prev is None or prev[0] != key or prev[1] != hop - 1 or
                    prev[2] != "forwarded" or prev[3] != sender):
                bad.append(f"line {n}: hop {hop} by {sender} has no forward behind it")
        prev = (key, hop, out, chosen)

        # 2. row consistency
        if out == "noroute":
            if chosen != 0 or d["chosenClass"] != "none" or dv != 0:
                bad.append(f"line {n}: noroute row names a next hop")
            continue
        sel += 1
        pick = etx_n if d["metric"] == "etx" else conf_n
        if chosen != pick:
            bad.append(f"line {n}: took {chosen} but the active metric "
                       f"({d['metric']}) picked {pick}")
        if dv != int(etx_n != conf_n):
            bad.append(f"line {n}: diverged={dv} but ETX {etx_n} vs confidence {conf_n}")
        cls = d["chosenClass"]
        if cls == "gateway":
            if not is_gateway(chosen):
                bad.append(f"line {n}: class gateway but id {chosen}")
        elif cls == "relay":
            if not is_relay(chosen):
                bad.append(f"line {n}: class relay but id {chosen}")
        else:
            bad.append(f"line {n}: chosenClass {cls!r}")
        if out == "delivered" and cls != "gateway":
            bad.append(f"line {n}: delivered to a non-gateway {chosen}")
        if out == "forwarded" and cls != "relay":
            bad.append(f"line {n}: forwarded to a non-relay {chosen}")
        div += dv
        if cls == "relay" and out == "forwarded":
            relay_carried += 1
        if out == "delivered" and hop >= 1:
            relayed_delivered += 1
        if dv and out in ("forwarded", "delivered"):
            div_carried += 1

    for b in bad[:12]:
        print(f"G18 FAIL: {b}")
    if bad:
        print(f"G18 FAIL: {len(bad)} inconsistent row(s)")
        return 1
    if len(metrics) != 1:
        print(f"G18 FAIL: rows name {sorted(metrics)} -- one run has one active metric")
        return 1
    metric = metrics.pop()
    print(f"G18 selections {sel}, divergences {div}, relay next hops carried "
          f"{relay_carried}, relayed fixes delivered {relayed_delivered}, diverged hops "
          f"carried {div_carried}, metric {metric}")

    # 4. the done-when
    ok = True
    if relay_carried == 0:
        print("G18 FAIL: no fix was ever handed to a relay -- every next hop was a gateway")
        ok = False
    if relayed_delivered == 0:
        print("G18 FAIL: no fix that crossed a relay reached a gateway")
        ok = False
    if div == 0:
        print("G18 FAIL: Divergences() is zero -- ETX and the confidence metric never "
              "disagreed about a routed frame")
        ok = False
    # 5. G18b
    if need_conf:
        if metric != "confidence":
            print(f"G18b FAIL: routed by {metric}, not by the confidence metric")
            ok = False
        elif div_carried == 0:
            print("G18b FAIL: no diverged hop was carried -- the confidence metric never "
                  "sent a fix where ETX would not have")
            ok = False
    # 6. recount against the run's own counters
    if stdout_log is not None:
        try:
            text = open(stdout_log, encoding="utf-8", errors="replace").read()
        except OSError as e:
            print(f"G18 FAIL: cannot read {stdout_log}: {e}")
            return 1
        m = re.findall(r"selections (\d+) \(tables (\d+)\), divergences (\d+) \(tables (\d+)\)",
                       text)
        if len(m) != 1:
            print(f"G18 FAIL: expected one router summary in {stdout_log}, found {len(m)} "
                  "(run a single arm)")
            return 1
        s_r, s_t, d_r, d_t = (int(x) for x in m[0])
        if not (s_r == s_t == sel and d_r == d_t == div):
            print(f"G18 FAIL: printed selections {s_r}/{s_t} divergences {d_r}/{d_t} "
                  f"(router/tables) but the CSV holds {sel} and {div}")
            ok = False
        else:
            print(f"G18 recount agrees: selections {sel}, divergences {div} "
                  "(router = tables = CSV)")
        # 7. the relays' round, when the default is what is under test
        if need_round:
            rounds = ROUND_RE.findall(text)
            if len(rounds) != 1:
                print(f"G18 FAIL: {len(rounds)} C5_INSTANT round line(s) in {stdout_log}, "
                      "expected one -- was --relaySlots off, or more than one arm?")
                ok = False
            else:
                slot, k, rnd, cyc, polls, bursts = rounds[0][:6]
                slot, rnd, cyc = float(slot), float(rnd), float(cyc)
                k, polls, bursts = int(k), int(polls), int(bursts)
                if k < 1 or abs(slot * k - rnd) > 0.1 or not rnd < cyc:
                    print(f"G18 FAIL: round {slot} ms x {k} = {rnd} ms against a {cyc} ms "
                          "cycle -- it must add up and end inside the cycle")
                    ok = False
                elif polls == 0 or bursts == 0 or bursts > polls:
                    print(f"G18 FAIL: the round moved {polls} anchor polls in {bursts} "
                          "bursts -- nothing moved means the round was inert")
                    ok = False
                else:
                    print(f"G18 round: {k} x {slot} ms = {rnd} ms of a {cyc} ms cycle, "
                          f"anchor polls moved {polls} in {bursts} bursts")
    if not ok:
        return 1
    print("G18b PASS" if need_conf else "G18 PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
