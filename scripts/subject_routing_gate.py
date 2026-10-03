#!/usr/bin/env python3
"""G59: every measured subject routes its own position-fix traffic, on its own tier (NW-2).

usage: subject_routing_gate.py <dir written by scripts/subject_routing_gate.sh>

Runs read:
  on/   --fixRoutingSubjects: fix_routing_<arm>_seed7_tier{0,1,2}.csv, one per tier's router,
        rows carrying the confidence route's three sums; trace_* (tag 0's tier per cycle) and
        subjtrace_*_subj<id>.csv (each subject's); on.log, one router block per tier.
  w/    the same with --routeBeta=0 --routeSlack=2.
  off/  without the flag (the control): one router on --relayTier, and its WARNING.
  r_nosubj, r_w   refusals: the flag without subjects; a weight without --fixRouting.

Checks (each prints "G59 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     every run the checks read exited 0.
  SIX      sect 6, per routed frame: every frame put on the air (forwarded, delivered, lost)
           in tier k's file was sent by a node that ranged on tier k that cycle, read from
           tag 0's trace, the subject's own subjtrace, or --relayTier for a plain relay; and
           every forwarded frame's receiver was on tier k too. Every row's sender took part.
  OFFTIER  a hop whose chosen relay was on another tier that cycle is refused ("offtier"),
           never sent, and every refused one really was elsewhere.
  SUBJ     the measured tag and at least one subject 2..5 had a fix delivered; frames went on
           the air on two tiers or more; the measured tag forwarded or originated on a tier.
  COST     confCost = alpha*confNlos + beta*confVar + gamma*confJit with the run's weights, on
           every routed row of on/ and w/; and in w/ the variance term was really there to
           drop (confVar > 0 on rows whose cost ignores it).
  COUNTS   each router block's originated / delivered by hops / forwarded / lost / off-tier
           counts are recounted from its file; participants sum to nodes x cycles.
  CONTROL  without the flag: no tier files, no NW-2 line, and the WARNING is printed.
  R        both refusals exit 1 with the reason.
"""
import csv
import glob
import os
import re
import sys

TIERN = {"868MHz": 0, "2.4GHz": 1, "UWB": 2}
TX = ("forwarded", "delivered", "lost")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G59 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G59 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G59 MISSING {label:<8}{msg}")


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


def tiers(run):
    """(t, nodeId) -> tier, from tag 0's trace and every subjtrace."""
    m = {}
    tr = glob.glob(os.path.join(run, "trace_*_seed*.csv"))
    for f in tr:
        for r in csv.DictReader(open(f)):
            m[(r["t"], 1)] = TIERN.get(r["activeTier"], -1)
    for f in glob.glob(os.path.join(run, "subjtrace_*_seed*_subj*.csv")):
        for r in csv.DictReader(open(f)):
            m[(r["t"], int(r["subjectId"]))] = TIERN.get(r["activeTier"], -1)
    return m, bool(tr)


def blocks(log):
    """tier name -> dict of the router block's counts."""
    out = {}
    pat = re.compile(r"L\.1\.1 fix routing on (\S+), selector [^\n]*\n\s+fixes originated (\d+): delivered by "
                     r"hops 1/2/3\+ (\d+)/(\d+)/(\d+), forwarded (\d+), lost on air (\d+), no route at origin "
                     r"(\d+) / mid-path (\d+), loop (\d+), ttl (\d+), duty (\d+), no slot (\d+)\n[^\n]*\n\s+NW-2 "
                     r"weights alpha ([-\d.e]+) beta ([-\d.e]+) gamma ([-\d.e]+) slack (\d+); measured tag routes "
                     r"(\w+); participants (\d+) over (\d+) cycles; next hop off this tier (\d+)")
    for m in pat.finditer(log):
        g = m.groups()
        out[g[0]] = dict(orig=int(g[1]), d1=int(g[2]), d2=int(g[3]), d3=int(g[4]), fwd=int(g[5]), lost=int(g[6]),
                         nro=int(g[7]), nrm=int(g[8]), loop=int(g[9]), ttl=int(g[10]), duty=int(g[11]), noslot=int(g[12]),
                         a=float(g[13]), b=float(g[14]), c=float(g[15]), slack=int(g[16]), tag0=g[17],
                         part=int(g[18]), cycles=int(g[19]), off=int(g[20]))
    return out


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "w", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "every run the checks read exited 0" if not bad else f"non-zero exit: {bad}")

    on = os.path.join(d, "on")
    files = {k: os.path.join(on, f"fix_routing_ca_tbl_full_seed7_tier{k}.csv") for k in range(3)}
    if not all(os.path.exists(p) for p in files.values()):
        missing("SIX", "on/ lacks a tier file")
        return 1
    rows = {k: list(csv.DictReader(open(p))) for k, p in files.items()}
    tmap, have_trace = tiers(on)
    if not have_trace:
        missing("SIX", "no trace_* in on/ (tag 0's tier)")
    log = read(os.path.join(d, "on.log")) or ""
    blk = blocks(log)
    relay_tier = 1  # --relayTier default (2.4 GHz); the regime does not change it

    def tier_of(t, nid):
        if nid == 1 or 2 <= nid <= 5:
            return tmap.get((t, nid))
        return relay_tier

    six, sent_by_tier = [], {0: 0, 1: 0, 2: 0}
    for k, rs in rows.items():
        for r in rs:
            st = tier_of(r["t"], int(r["senderId"]))
            if st != k:
                six.append((k, r["t"], r["senderId"], st, r["outcome"]))
                continue
            if r["outcome"] in TX:
                sent_by_tier[k] += 1
            if r["outcome"] == "forwarded":
                rt = tier_of(r["t"], int(r["chosenNext"]))
                if rt != k:
                    six.append((k, r["t"], "receiver", r["chosenNext"], rt))
    report(not six and sum(sent_by_tier.values()) > 0, "SIX",
           f"frames on the air by tier {sent_by_tier}, each from a node on that tier that cycle, every forward heard "
           f"on it; breaches {len(six)} (need 0)" + (f": {six[:3]}" if six else ""))

    off, offbad = 0, []
    for k, rs in rows.items():
        for i, r in enumerate(rs):
            if r["outcome"] != "offtier":
                continue
            off += 1
            nt = tier_of(r["t"], int(r["chosenNext"]))
            if r["chosenClass"] != "relay" or nt == k:
                offbad.append((k, r["t"], r["chosenNext"], nt))
            nxt = rs[i + 1] if i + 1 < len(rs) else None
            if nxt and nxt["t"] == r["t"] and nxt["originId"] == r["originId"] and int(nxt["hop"]) > int(r["hop"]):
                offbad.append((k, r["t"], "continued after offtier"))
    report(not offbad and off > 0, "OFFTIER",
           f"{off} hops refused for a next hop on another tier, each really elsewhere and none continued; bad "
           f"{len(offbad)} (need 0)" + (f": {offbad[:3]}" if offbad else ""))

    deliv = {int(r["originId"]) for rs in rows.values() for r in rs if r["outcome"] == "delivered"}
    tiers_used = [k for k, v in sent_by_tier.items() if v > 0]
    tag0 = sum(1 for rs in rows.values() for r in rs if r["senderId"] == "1" and r["outcome"] in TX)
    ok = 1 in deliv and any(2 <= x <= 5 for x in deliv) and len(tiers_used) >= 2 and tag0 > 0
    report(ok, "SUBJ", f"fixes delivered from nodes {sorted(deliv)} (need 1 and one of 2..5); frames on tiers "
           f"{tiers_used} (need 2+); frames the measured tag put on the air {tag0} (need > 0)")

    cbad, ndrop, nchk = [], 0, 0
    for run in ("on", "w"):
        rl = read(os.path.join(d, run + ".log")) or ""
        b = blocks(rl)
        if not b:
            cbad.append((run, "no NW-2 line"))
            continue
        w = next(iter(b.values()))
        for k in range(3):
            p = os.path.join(d, run, f"fix_routing_ca_tbl_full_seed7_tier{k}.csv")
            if not os.path.exists(p):
                cbad.append((run, k, "no file"))
                continue
            for r in csv.DictReader(open(p)):
                if r["confNext"] == "0":
                    continue
                nchk += 1
                want = w["a"] * float(r["confNlos"]) + w["b"] * float(r["confVar"]) + w["c"] * float(r["confJit"])
                if abs(float(r["confCost"]) - want) > 1e-4 + 1e-6 * abs(want):
                    cbad.append((run, k, r["t"], r["confCost"], round(want, 6)))
                if run == "w" and float(r["confVar"]) > 0:
                    ndrop += 1
        if run == "w" and (w["b"] != 0.0 or w["slack"] != 2):
            cbad.append((run, "weights line", w["b"], w["slack"]))
    report(not cbad and nchk > 0 and ndrop > 0, "COST",
           f"{nchk} routed rows' confidence cost recomputed from the run's weights; in w/ beta 0 dropped a non-zero "
           f"variance on {ndrop} (need > 0); bad {len(cbad)} (need 0)" + (f": {cbad[:3]}" if cbad else ""))

    cnt = []
    names = {0: "868MHz", 1: "2.4GHz", 2: "UWB"}
    psum = 0
    for k, rs in rows.items():
        b = blk.get(names[k])
        if not b:
            cnt.append((k, "no block"))
            continue
        orig = sum(1 for r in rs if r["hop"] == "0")
        dl = [sum(1 for r in rs if r["outcome"] == "delivered" and min(int(r["hop"]) + 1, 3) == h) for h in (1, 2, 3)]
        got = (b["orig"], b["d1"], b["d2"], b["d3"], b["fwd"], b["lost"], b["off"], b["loop"])
        want = (orig, dl[0], dl[1], dl[2], sum(1 for r in rs if r["outcome"] == "forwarded"),
                sum(1 for r in rs if r["outcome"] == "lost"), sum(1 for r in rs if r["outcome"] == "offtier"),
                sum(1 for r in rs if r["outcome"] == "loop"))
        if got != want or b["tag0"] != "yes":
            cnt.append((names[k], got, want))
        psum += b["part"]
        cyc = b["cycles"]
    nodes = 6
    if blk and psum != nodes * cyc:
        cnt.append(("participants", psum, nodes * cyc))
    report(not cnt and len(blk) == 3, "COUNTS",
           f"three router blocks recounted from their files; participants {psum} = {nodes} nodes x "
           f"{cyc if blk else 0} cycles; bad {len(cnt)} (need 0)" + (f": {cnt[:2]}" if cnt else ""))

    offlog = read(os.path.join(d, "off.log"))
    if offlog is None:
        missing("CONTROL", "no off.log")
    else:
        tf = glob.glob(os.path.join(d, "off", "fix_routing_*_tier*.csv"))
        plain = glob.glob(os.path.join(d, "off", "fix_routing_*_seed7.csv"))
        nw2 = offlog.count("NW-2 weights")
        warn = "WARNING: --fixRouting with --measuredSubjects > 1" in offlog
        report(not tf and len(plain) == 1 and nw2 == 0 and warn, "CONTROL",
               f"without the flag: tier files {len(tf)}, one --relayTier file {len(plain) == 1}, NW-2 lines {nw2} (need 0), "
               f"the WARNING {warn}")

    rb = []
    for n, reason in (("r_nosubj", "--fixRoutingSubjects needs"), ("r_w", "they need --fixRouting")):
        txt = read(os.path.join(d, n + ".log")) or ""
        if rc(d, n) != 1 or reason not in txt:
            rb.append((n, rc(d, n)))
    report(not rb, "R", "both refusals exit 1 with the reason" if not rb else f"refusals not as expected: {rb}")

    print(f"  G59: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
