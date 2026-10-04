#!/usr/bin/env python3
"""G81: a requester's escalation request travels as a frame (register row 52, TECH_DEBT N.10,
--requestOverMesh).

usage: reqcarry_gate.py <dir written by scripts/reqcarry_gate.sh>

Runs read: on/ (the flag), on_hd/ (the flag with --halfDuplex=2, where the arbiter's ACK is
not destroyed by its own GRANT -- N.6 -- and the base station's route is delivered at seed 7),
off/ (the control), r_noreq (refusal: without --requesterEscalation).

What a firing looks like on disk. reqcarry_*: one row per request handed to a carrier --
requester 0 (the base station) with a gateway carrier, or a finder carrying its own named
request -- or "unheard" with carrier 0. events_*: the carried ESC_REQUEST, its payload
";for=<subject>", delivered at the arbiter. arb_*: the arbiter's obo_submit for that subject
at the instant of that delivery, and never at the instant of the carriage. The defect this
replaces is an obo_submit at a finder arbiter with no frame under it.

Checks (each prints "G81 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on, on_hd and off exited 0.
  ROWS     over on and on_hd, a frame carried by a gateway for the base station and one
           carried by a finder for itself, each delivered at its arbiter.
  Per flag-on run:
  ROUTE    a base-station row: arbiter a finder (501+), carrier a gateway (101..500) other than
           the arbiter; a finder's row: carrier the finder itself. Carried rows have
           heardAgeS >= 0; unheard rows carrier 0 and heardAgeS -1.
  LINK     every row is a request: a reqesc_ row (base) or a named finderreq_ row (finder)
           at that t, for that subject and arbiter, with the same outcome; and every reqesc_
           row naming a finder arbiter, and every named finderreq_ row, is carried or unheard,
           with its carriage row.
  ACT      every obo_submit has exactly one cause: a backhaul request to a gateway arbiter
           (reqesc_, same t), a finder's own-queue request (finderreq_, same t), or a carried
           frame delivered at that arbiter for that subject at that t (events_). Every such
           delivery has its obo_submit. No carried request is scored at its carriage instant.
  COUNTS   the N.10 line: carried and unheard equal the file; frames sent equal the carried
           rows; heard equals the deliveries; granted + queued + refused equals heard.
  CONTROL  off/: no reqcarry_ file, no N.10 line, no ";for=" frame.
  R        the refusal exits 1 with the reason.
"""
import csv
import glob
import os
import re
import sys

LINE = re.compile(r"N\.10 request over mesh \(\w+\): carried (\d+), unheard (\d+); frames sent (\d+), "
                  r"heard at arbiters (\d+): granted (\d+), queued (\d+), refused (\d+)")
FOR = re.compile(r";for=(\d+)")
ON = ("on", "on_hd")
TOL = 0.006  # events_ prints t with six significant digits
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G81 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G81 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G81 MISSING {label:<8}{msg}")


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


def is_gw(i):
    return 101 <= int(i) <= 500


def is_finder(i):
    return int(i) >= 501


def obo_deliveries(path):
    """(t, arbiter, subject, carrier) of every on-behalf ESC_REQUEST delivered (or captured)."""
    out = []
    try:
        with open(path) as fh:
            rd = csv.reader(fh)
            next(rd, None)
            for r in rd:
                if len(r) < 8 or r[4] != "ESC_REQUEST" or r[5] not in ("delivered", "capture"):
                    continue
                m = FOR.search(",".join(r[7:]))
                if m:
                    out.append((float(r[0]), r[3], m.group(1), r[2]))
    except OSError:
        return None
    return out


def check_run(d, n):
    """ROUTE, LINK, ACT and COUNTS on one flag-on run; returns (base delivered, finder delivered)."""
    on = os.path.join(d, n)
    cr = rows(os.path.join(on, "reqcarry_ca_tbl_full_seed7.csv"))
    rq = rows(os.path.join(on, "reqesc_ca_tbl_full_seed7.csv"))
    fq = rows(os.path.join(on, "finderreq_ca_tbl_full_seed7.csv"))
    arb = rows(os.path.join(on, "arb_ca_tbl_full_seed7.csv"))
    dv = obo_deliveries(os.path.join(on, "events_ca_tbl_full_seed7.csv"))
    if None in (cr, rq, fq, arb, dv):
        missing("ROWS", f"{n}/ lacks one of reqcarry_, reqesc_, finderreq_, arb_, events_")
        return 0, 0
    carried = [r for r in cr if r["outcome"] == "carried"]

    rb = []
    for r in cr:
        c, a, q = r["carrier"], r["arbiter"], r["requester"]
        if r["outcome"] == "carried":
            if float(r["heardAgeS"]) < 0.0 or c == a:
                rb.append((r["t"], q, "carried without hearing", c, a))
            elif q == "0" and not (is_finder(a) and is_gw(c)):
                rb.append((r["t"], q, "base row not gateway->finder", c, a))
            elif q != "0" and c != q:
                rb.append((r["t"], q, "finder row not from its own radio", c, a))
        elif r["outcome"] == "unheard":
            if c != "0" or float(r["heardAgeS"]) != -1.0:
                rb.append((r["t"], q, "unheard with a carrier", c))
        else:
            rb.append((r["t"], q, "outcome", r["outcome"]))
    report(not rb, "ROUTE", f"{n}: {len(cr)} rows; carrier or arbiter wrong for the route: {len(rb)} (need 0)"
           + (f": {rb[:2]}" if rb else ""))

    lb = []
    base = {(r["t"], r["target"], r["arbiter"]): r["outcome"] for r in rq}
    fnd = {(r["t"], r["finderId"], r["target"], r["arbiter"]): r["outcome"] for r in fq if r["route"] == "named"}
    for r in cr:
        if r["requester"] == "0":
            o = base.get((r["t"], r["subject"], r["arbiter"]))
        else:
            o = fnd.get((r["t"], r["requester"], r["subject"], r["arbiter"]))
        if o != r["outcome"]:
            lb.append((r["t"], r["requester"], r["subject"], "request says", o))
    ck = {(r["t"], r["requester"], r["subject"], r["arbiter"]) for r in cr}
    for r in rq:
        if r["outcome"] in ("carried", "unheard") and (r["t"], "0", r["target"], r["arbiter"]) not in ck:
            lb.append((r["t"], "0", r["target"], "carried or unheard, no carriage row"))
    for r in fq:
        if r["outcome"] in ("carried", "unheard") and (r["t"], r["finderId"], r["target"], r["arbiter"]) not in ck:
            lb.append((r["t"], r["finderId"], r["target"], "carried or unheard, no carriage row"))
    for r in rq:
        if r["arbiter"] != "0" and is_finder(r["arbiter"]) and r["outcome"] not in ("carried", "unheard"):
            lb.append((r["t"], "0", r["target"], "finder arbiter reached without a frame", r["outcome"]))
    for r in fq:
        if r["route"] == "named" and r["outcome"] not in ("carried", "unheard"):
            lb.append((r["t"], r["finderId"], r["target"], "named request without a frame", r["outcome"]))
    report(not lb, "LINK", f"{n}: carriages without their request, or requests to a finder scored without a "
           f"frame: {len(lb)} (need 0)" + (f": {lb[:2]}" if lb else ""))

    sub = [(float(a["t"]), a["t"], a["arbiter"], a["requester"]) for a in arb if a["event"] == "obo_submit"]
    backhaul = {(r["t"], r["arbiter"], r["target"]) for r in rq if r["outcome"] in ("granted", "queued", "refused")}
    own = {(r["t"], r["arbiter"], r["target"]) for r in fq if r["route"] == "own"}
    used = [False] * len(dv)
    ab = []
    for tf, ts, a, s in sub:
        causes = int((ts, a, s) in backhaul) + int((ts, a, s) in own)
        hit = [k for k, (td, ad, sd, _) in enumerate(dv) if ad == a and sd == s and abs(td - tf) <= TOL]
        if hit:
            used[hit[0]] = True
            causes += 1
        if causes != 1:
            ab.append((ts, a, s, "causes", causes))
    for k, u in enumerate(used):
        if not u:
            ab.append((dv[k][0], dv[k][1], dv[k][2], "delivered, never scored"))
    inst = {(r["t"], r["arbiter"], r["subject"]) for r in carried}
    for tf, ts, a, s in sub:
        if (ts, a, s) in inst:
            ab.append((ts, a, s, "scored at its carriage instant"))
    report(not ab, "ACT", f"{n}: {len(sub)} obo_submit, {len(dv)} carried frames delivered; without exactly "
           f"one cause, unscored, or instant: {len(ab)} (need 0)" + (f": {ab[:2]}" if ab else ""))

    m = LINE.search(read(os.path.join(d, n + ".log")) or "")
    got = tuple(int(x) for x in m.groups()) if m else None
    nu = sum(r["outcome"] == "unheard" for r in cr)
    ok = (got is not None and got[0] == len(carried) and got[1] == nu and got[2] == len(carried)
          and got[3] == len(dv) and got[4] + got[5] + got[6] == got[3])
    report(ok, "COUNTS", f"{n}: N.10 line {got}; carried {len(carried)}, unheard {nu}, deliveries {len(dv)}")

    return sum(is_gw(x[3]) for x in dv), sum(is_finder(x[3]) for x in dv)


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ON + ("off",) if rc(d, n) != 0]
    report(not bad, "RUNS", "on, on_hd and off exited 0" if not bad else f"non-zero exit: {bad}")
    nb = nf = 0
    for n in ON:
        b, f = check_run(d, n)
        nb += b
        nf += f
    report(nb > 0 and nf > 0, "ROWS", f"carried requests delivered at their arbiters over {ON}: by a gateway for "
           f"the base station {nb}, by a finder for itself {nf} (need > 0 each)")

    of = glob.glob(os.path.join(d, "off", "reqcarry_*"))
    ol = "N.10 request over mesh" in (read(os.path.join(d, "off.log")) or "")
    od = obo_deliveries(os.path.join(d, "off", "events_ca_tbl_full_seed7.csv"))
    report(not of and not ol and od == [], "CONTROL",
           f"off/: reqcarry_ files {len(of)}, N.10 line {ol}, on-behalf frames {None if od is None else len(od)} "
           f"(need 0, False, 0)")

    t = read(os.path.join(d, "r_noreq.log")) or ""
    report(rc(d, "r_noreq") == 1 and "--requestOverMesh needs" in t, "R",
           f"without --requesterEscalation: exit {rc(d, 'r_noreq')} (need 1)")
    print(f"  G81: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
