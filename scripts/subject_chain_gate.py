#!/usr/bin/env python3
"""G62: measured subjects in the relay chain (TECH_DEBT N.3, N.15; --subjectRelayChain).

usage: subject_chain_gate.py <dir written by scripts/subject_chain_gate.sh>

Runs read (G17's chain regime with every field node a measured subject, hop provenance on):
  on/    static_2p4 with --subjectRelayChain: relay_chain_*.csv, subjtrace_*, the stdout.
  full/  ca_tbl_full with the flag: a hybrid arm, so subjects escalate and keep the floor.
         Its subjects make no chain exchange (on 2.4 GHz the floor solves; on 868 MHz A1
         admits nothing): it shows the tier-1 skip and the hybrid bookkeeping.
  off/   static_2p4 without it (the control).
  r_nochain, r_nosubj   refusals: without --relayChain; without measured subjects.

What a firing looks like on disk. relay_chain_<arm>_seed7.csv gains rows whose consumer is
a subject (ids 2..4) on leg `relay` -- its exchange with a fix-holder -- and `fuse` rows
for subjects whose step fused one, carrying the depth taken. Tag 0's `tag` rows through a
subject then carry that subject's depth + 1. The defects this guards against are the
chain's own (G17): depth from nowhere, A2 disagreeing with its row, sigma_eff outside its
bounds; and the subject's own: a holder that had not stepped (tag 0, or a higher id), an
exchange on a tier the subject was not on, or a holder asleep on it.

Checks (each prints "G62 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on, full, off exited 0.
  ROWS     on/ and full/: subject `relay` rows > 0 and subject `fuse` rows > 0 in at least
           one of them.
  A2       every decision row's A2 label agrees with its numbers, and every ADMIT's
           sigma_eff^2 - sigma_raw^2 lies in [0, carriedVarM2] (G17's rules 2 and 3).
  PROV     every depth-d >= 2 row (decision or fuse) through relay R has an earlier fuse
           row for R at depth d-1, in file order (G17's rule 4).
  DEPTH2   tag 0 admitted a range at hopDepth 2 through a subject, and the NW-1 line shows
           fixes at depth 2, in on/ or full/.
  HOLDER   every subject exchange's holder is a lower-id subject (holders step first; tag 0
           steps last and is never one), and none is on 868 MHz (A1 admits nothing there, so
           the exchange is not made).
  SIX      every subject exchange is on the tier the subject was on that cycle (subjtrace
           activeTier), and its holder was awake on it (its active tier, or the 868 MHz floor
           while off 868), in on/ and full/.
  TIER1    full/, a hybrid arm: subjects short on 868 MHz exist and made no exchange (the
           branch that skips tier 1, where A1 admits nothing). In every hybrid regime probed
           the subjects on 2.4 GHz held a fix through the floor, so the exchanges are shown
           on static_2p4 (on/).
  COUNTS   the L.1.2 subject-chain line: ranged = subject `relay` rows, fused = subject
           `fuse` rows, fused by depth = their hopDepth, in on/ and full/.
  CONTROL  off/: no subject `relay` or `fuse` rows, every tag row at depth 1, no subject-
           chain line.
  R        both refusals exit 1 with the reason.
"""
import csv
import glob
import os
import re
import sys

SUBJ = {2, 3, 4}
TIERN = {"868MHz": 0, "2.4GHz": 1, "UWB": 2}
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G62 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G62 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G62 MISSING {label:<8}{msg}")


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


def chain_rows(d, n):
    fs = glob.glob(os.path.join(d, n, "relay_chain_*_seed7.csv"))
    if len(fs) != 1:
        return None
    return list(csv.DictReader(open(fs[0])))


def a2_bad(rows):
    bad = []
    for i, r in enumerate(rows):
        if r["leg"] == "fuse" or r["verdict"] == "COMPOSE_REFUSED" or r["verdict"].startswith("REJECT_TIER"):
            if r["leg"] != "fuse" and r["a2"] != "not_reached":
                bad.append((i, "a2 reached past A1 or a refused composition"))
            continue
        h, mx = int(r["hopDepth"]), int(r["maxRelayDepth"])
        b, cv = float(r["budgetM2"]), float(r["carriedVarM2"])
        want = "ceiling" if not (1 <= h <= mx) else ("budget" if b > 0 and cv > b else "pass")
        if r["a2"] != want:
            bad.append((i, "a2", r["a2"], want))
        if r["verdict"] == "ADMIT":
            if want != "pass":
                bad.append((i, "admit past A2"))
            se, sr = float(r["sigmaEffM"]), float(r["sigmaRawM"])
            proj = se * se - sr * sr
            if proj < -1e-6 or proj > cv * (1 + 1e-6) + 1e-6:
                bad.append((i, "sigma", proj, cv))
        if r["verdict"].startswith("REJECT_DEPTH") and want == "pass":
            bad.append((i, "depth refusal with A2 pass"))
    return bad


def prov_bad(rows):
    fused = set()
    bad = []
    for i, r in enumerate(rows):
        h = int(r["hopDepth"])
        if r["leg"] == "fuse":
            # a fuse at depth d >= 2 leaned on relayId at d-1
            if h >= 2 and (int(r["relayId"]), h - 1) not in fused:
                bad.append((i, "fuse", r["consumerId"], r["relayId"], h))
            fused.add((int(r["consumerId"]), h))
            continue
        if h >= 2 and r["verdict"] != "COMPOSE_REFUSED" and (int(r["relayId"]), h - 1) not in fused:
            bad.append((i, r["leg"], r["consumerId"], r["relayId"], h))
    return bad


def subjtrace(d, n):
    m = {}
    for f in glob.glob(os.path.join(d, n, "subjtrace_*_subj*.csv")):
        sid = int(re.search(r"_subj(\d+)\.csv$", f).group(1))
        for r in csv.DictReader(open(f)):
            m[(round(float(r["t"]), 3), sid)] = TIERN.get(r["activeTier"], -1)
    return m


LINE = re.compile(r"L\.1\.2 subject chain: subject-cycles held on direct ranges (\d+), short (\d+) "
                  r"\(on 868 MHz, where A1 admits nothing, (\d+); no exchange possible (\d+)\), exchanges (\d+) "
                  r"\(ranged (\d+)\), admitted (\d+), fused (\d+) \(at depth 1/2/3\+ (\d+)/(\d+)/(\d+)\)")
DEPTHLINE = re.compile(r"NW-1 hop provenance \(\S+\): fixes by depth of the fused set 0:(\d+) 1:(\d+) 2:(\d+)")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "full", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on, full, off exited 0" if not bad else f"non-zero exit: {bad}")

    runs = {n: chain_rows(d, n) for n in ("on", "full", "off")}
    if runs["on"] is None or runs["full"] is None:
        missing("ROWS", "no relay_chain_*.csv in on/ or full/")
        return 1
    sr = {n: [r for r in runs[n] if r["leg"] == "relay" and int(r["consumerId"]) in SUBJ] for n in ("on", "full")}
    sf = {n: [r for r in runs[n] if r["leg"] == "fuse" and int(r["consumerId"]) in SUBJ] for n in ("on", "full")}
    report(sum(len(v) for v in sr.values()) > 0 and sum(len(v) for v in sf.values()) > 0, "ROWS",
           f"subject exchanges on/full {len(sr['on'])}/{len(sr['full'])}, subject fuses {len(sf['on'])}/{len(sf['full'])} (need > 0)")

    ab = [(n,) + b for n in ("on", "full") for b in a2_bad(runs[n])]
    report(not ab, "A2", f"decision rows checked {sum(1 for n in ('on', 'full') for r in runs[n] if r['leg'] != 'fuse')}; "
           f"A2 or sigma disagreeing {len(ab)} (need 0)" + (f": {ab[:3]}" if ab else ""))
    pb = [(n,) + b for n in ("on", "full") for b in prov_bad(runs[n])]
    report(not pb, "PROV", f"depth >= 2 rows without an earlier fuse at depth - 1: {len(pb)} (need 0)" + (f": {pb[:3]}" if pb else ""))

    t2 = {n: [r for r in runs[n] if r["leg"] == "tag" and int(r["hopDepth"]) == 2 and r["verdict"] == "ADMIT"
              and int(r["relayId"]) in SUBJ] for n in ("on", "full")}
    dl = {}
    for n in ("on", "full"):
        m = DEPTHLINE.search(read(os.path.join(d, n + ".log")) or "")
        dl[n] = int(m.group(3)) if m else None
    report(any(t2[n] for n in t2) and any((dl[n] or 0) > 0 for n in dl), "DEPTH2",
           f"tag 0's depth-2 admits through a subject on/full {len(t2['on'])}/{len(t2['full'])}; "
           f"fixes at depth 2 on/full {dl['on']}/{dl['full']} (need > 0 somewhere)")

    hb = [(n, r["t"], r["consumerId"], r["relayId"]) for n in ("on", "full") for r in sr[n]
          if not (int(r["relayId"]) in SUBJ and int(r["relayId"]) < int(r["consumerId"]))]
    hb += [(n, r["t"], r["consumerId"], "on 868 MHz") for n in ("on", "full") for r in sr[n] if r["tier"] == "0"]
    report(not hb, "HOLDER", f"subject exchanges whose holder is not a lower-id subject: {len(hb)} (need 0)" + (f": {hb[:3]}" if hb else ""))

    sb = []
    nx = 0
    for n in ("on", "full"):
        st = subjtrace(d, n)
        for r in sr[n]:
            nx += 1
            key = round(float(r["t"]), 3)
            c, h, tier = int(r["consumerId"]), int(r["relayId"]), int(r["tier"])
            ct, ht = st.get((key, c)), st.get((key, h))
            if ct is None or ht is None:
                sb.append((n, r["t"], c, h, "no trace row"))
                continue
            if tier != ct:
                sb.append((n, r["t"], c, h, "subject not on the tier", tier, ct))
            if not (ht == tier or (tier == 0 and ht != 0)):
                sb.append((n, r["t"], c, h, "holder asleep on the tier", tier, ht))
    report(not sb and nx > 0, "SIX",
           f"{nx} subject exchanges, each on the tier the subject was on with the holder awake on it; bad {len(sb)} (need 0)"
           + (f": {sb[:3]}" if sb else ""))
    m = LINE.search(read(os.path.join(d, "full.log")) or "")
    t1 = int(m.group(3)) if m else -1
    report(t1 > 0, "TIER1", f"full/ (a hybrid arm): short subject-cycles on 868 MHz that made no exchange {t1} (need > 0; "
           f"A1 admits no relayed range on tier 1, so none is attempted)")

    cb = []
    for n in ("on", "full"):
        m = LINE.search(read(os.path.join(d, n + ".log")) or "")
        if not m:
            cb.append((n, "no line"))
            continue
        g = [int(x) for x in m.groups()]
        byd = [sum(1 for r in sf[n] if min(int(r["hopDepth"]), 3) == k) for k in (1, 2, 3)]
        if g[5] != len(sr[n]) or g[7] != len(sf[n]) or g[8:11] != byd:
            cb.append((n, "ranged", g[5], len(sr[n]), "fused", g[7], len(sf[n]), g[8:11], byd))
    report(not cb, "COUNTS", "the subject-chain line recounted from the rows in on/ and full/" if not cb else f"disagree: {cb}")

    if runs["off"] is None:
        missing("CONTROL", "no relay_chain_*.csv in off/")
    else:
        o = runs["off"]
        osr = [r for r in o if r["leg"] in ("relay", "fuse") and int(r["consumerId"]) in SUBJ]
        deep = [r for r in o if r["leg"] == "tag" and int(r["hopDepth"]) != 1]
        line = "L.1.2 subject chain" in (read(os.path.join(d, "off.log")) or "")
        report(not osr and not deep and not line, "CONTROL",
               f"off/: subject rows {len(osr)}, tag rows not at depth 1 {len(deep)}, subject-chain line {line} (need 0, 0, False)")

    rb = []
    for n in ("r_nochain", "r_nosubj"):
        if rc(d, n) != 1 or "--subjectRelayChain needs" not in (read(os.path.join(d, n + ".log")) or ""):
            rb.append((n, rc(d, n)))
    report(not rb, "R", "both refusals exit 1 with the reason" if not rb else f"refusals not as expected: {rb}")

    print(f"  G62: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
