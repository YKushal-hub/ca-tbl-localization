#!/usr/bin/env python3
"""G24: the preset is conformant (docs/SPEC_CONFORMANCE_PLAN.md sect 3.2).

usage: spec_profile_gate.py <dir written by scripts/spec_profile_gate.sh>

Every spec mechanism must have a NON-ZERO population in the run registered for it (the
"firing plus input" rule, TECH_DEBT H.3/I.5). A zero is a FAIL, never a pass by absence.
Runs: main (the preset), end (the endgame: 750 m box, finders converging), gasp (the
last-gasp run); off_<flag> (the preset with that flag off); eq_preset / eq_flags (the preset
against its own flags written out); r_topo and r_n12 (the refusals).

Checks (each prints "G24 ok|FAIL|MISSING <LABEL> ..."):
  RUNS      every run exited 0 (the refusals r_* excepted). x_froff, the preset with routing
            turned off explicitly (CAMP-SPEC's single-hop shape), is one of them: the preset's
            routing weights are unused there, not refused.
  EQUIV     --specProfile gives byte-identical CSVs to its own settings given as flags.
  FILE      spec_profile.txt lists every setting; in end/ the explicit --boxHalf and
            --finderMode are recorded as kept, not overwritten.
  WEIGHTS   every NW-2 router in main/ and end/ ran with CAMP-SPEC's pick, alpha 0 beta 0
            gamma 1 slack 1 (owner decision 2026-10-03).
  NOHV      --hopVarianceTerms is not in the preset (owner decision 2026-10-03, TECH_DEBT
            N.11): spec_profile.txt does not set it, and no NW-1 hop-variance line is printed
            in main/ or end/.
  POP <m>   one line per mechanism: its population in its registered run is > 0.
  EFF <f>   one line per efficacy run: with flag <f> off, its mechanism's population is 0.
  R         the preset with --topology=1 exits 1 with the reason.
  R12       the preset with --fixRoutingSubjects=false exits 1 with the reason: --fixRouting
            with subjects and without it is refused (TECH_DEBT N.12, 2026-10-03), so the
            routed-subjects mechanism is shown off by refusal, not by an efficacy run.
"""
import csv
import glob
import os
import re
import sys

nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G24 ok      {label:<26}{msg}")
    else:
        nfail += 1
        print(f"  G24 FAIL    {label:<26}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G24 MISSING {label:<26}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def log(d, n):
    try:
        return open(os.path.join(d, n + ".log")).read()
    except OSError:
        return ""


def num(text, pat, groups=(1,)):
    """Sum of the given groups over every match of pat, or None when it never matches."""
    ms = list(re.finditer(pat, text))
    if not ms:
        return None
    return sum(float(m.group(g)) for m in ms for g in groups)


def routed_subjects(d, n):
    tot = 0
    files = glob.glob(os.path.join(d, n, "fix_routing_*_tier*.csv"))
    if not files:
        return None
    for f in files:
        for r in csv.DictReader(open(f)):
            if r["outcome"] == "delivered" and 1 <= int(r["originId"]) <= 5:
                tot += 1
    return tot


def uwb_fixes(d, n):
    tot = 0
    fs = glob.glob(os.path.join(d, n, "trace_*.csv")) + glob.glob(os.path.join(d, n, "subjtrace_*.csv"))
    if not fs:
        return None
    for f in fs:
        for r in csv.DictReader(open(f)):
            if r["hadFix"] == "1" and r["fixTier"] == "UWB":
                tot += 1
    return tot


def precise(text):
    return len(re.findall(r"path [A-Z_>]*LOCATED_PRECISE", text))


# mechanism -> (run, reader, flag whose efficacy run reads it (or None))
MECH = [
    ("tier-1 TDoA solves", "main", lambda d, n: num(log(d, n), r"solves OK (\d+)"), "tier1Tdoa"),
    ("NLOS score fed to PCS", "main", lambda d, n: num(log(d, n), r"fed to PCS (\d+)"), "pcsNlosClassifier"),
    ("integrity eligibility", "main", lambda d, n: num(log(d, n), r"DC-8 integrity eligibility from NLOS_score: (\d+) eligible"), "integrityNlosFromScore"),
    ("suspect downweighted", "main", lambda d, n: num(log(d, n), r"DC-7 downweighted suspect fixes: (\d+) of"), None),
    ("IMU predict cycles", "main", lambda d, n: num(log(d, n), r"DC-1 IMU: [^\n]*cycles (\d+)"), None),
    ("IMU/corroboration refusals", "main", lambda d, n: num(log(d, n), r"INTEGRITY: escalations refused -- IMU inconsistency (\d+), weak corroboration (\d+)", (1, 2)), None),
    ("witness evaluations", "main", lambda d, n: num(log(d, n), r"DC-9 witness \(model\): evaluations (\d+)"), None),
    ("entry prediction", "main", lambda d, n: num(log(d, n), r"DC-13 entry prediction: substitutions (\d+)"), "entryPrediction"),
    ("Case D updates", "main", lambda d, n: num(log(d, n), r"DC-4 Case D: cycles \d+, updates applied (\d+)"), "caseD"),
    ("Case A fixes", "main", lambda d, n: num(log(d, n), r"A\(2 gateways\) (\d+)"), None),
    ("Case B fixes", "main", lambda d, n: num(log(d, n), r"B\(anchor\+peer\) (\d+)"), None),
    ("Case C cycles", "main", lambda d, n: num(log(d, n), r"C\(unlocalisable\) (\d+)"), None),
    ("Case C shapes at the base", "end", lambda d, n: num(log(d, n), r"shapes: pairs (\d+), 3\+ (\d+)", (1, 2)), "caseCRelative"),
    ("triggered updates", "main", lambda d, n: num(log(d, n), r"MC-2 triggered updates: large correction (\d+)"), "triggeredUpdates"),
    ("last gasp", "gasp", lambda d, n: num(log(d, n), r"last gasp (\d+)"), None),
    ("mobility from range", "main", lambda d, n: num(log(d, n), r"MOBILITY: stationary \d+, approaching (\d+), receding (\d+)", (1, 2)), "mobilityFromRange"),
    ("digest pushes", "main", lambda d, n: num(log(d, n), r"ANTI-ENTROPY: digest pushes (\d+)"), None),
    ("CI table fuses", "main", lambda d, n: num(log(d, n), r"TABLE CI: (\d+) covariance-intersection fuses"), "tableFuseCI"),
    ("mesh pairs", "main", lambda d, n: num(log(d, n), r"mesh pairs (\d+)"), None),
    ("relayed ranges admitted", "main", lambda d, n: num(log(d, n), r"C5 relayed measurements: admitted (\d+)"), None),
    # Hop depth is read in the endgame: relaying needs subjects within 2.4 GHz reach of one
    # another. Moved there after the first G24 run read 0 at 5 km (2026-10-03).
    ("fixes at hop depth 1", "end", lambda d, n: num(log(d, n), r"fixes by depth of the fused set 0:\d+ 1:(\d+)"), None),
    ("fixes at hop depth 2", "end", lambda d, n: num(log(d, n), r"fixes by depth of the fused set 0:\d+ 1:\d+ 2:(\d+)"), "subjectRelayChain"),
    ("subjects fused through the chain", "end", lambda d, n: num(log(d, n), r"L\.1\.2 subject chain: [^\n]*, fused (\d+)"), None),
    ("subjects' fixes routed", "main", routed_subjects, None),  # off: refused, R12
    ("v2 answers heard", "main", lambda d, n: num(log(d, n), r"answers heard by requesters (\d+)"), None),
    ("query-triggered escalations", "main", lambda d, n: num(log(d, n), r"NW-4 query-triggered escalation[^\n]*escalations (\d+)"), "queryTriggeredEscalation"),
    ("reverse-path hops", "main", lambda d, n: num(log(d, n), r"unicast hops (\d+)"), None),
    ("arbitration grants", "end", lambda d, n: num(log(d, n), r"NW-5 arbitration \(\S+\): grants (\d+)"), None),
    ("arbitration pre-emptions", "end", lambda d, n: num(log(d, n), r"NW-5 arbitration[^\n]*pre-emptions (\d+)"), None),
    ("queue gossip items", "end", lambda d, n: num(log(d, n), r"queue items sent (\d+)"), None),
    ("grant tie-break verdicts", "end", lambda d, n: num(log(d, n), r"tie-break WIN (\d+) DEFER (\d+)", (1, 2)), None),
    ("grants confirmed (tag 0)", "end", lambda d, n: num(log(d, n), r"NW-7 grant-gated UWB \(\S+\), tag 0: requests \d+, confirmed (\d+)"), None),
    ("handshakes settled by a grant", "end", lambda d, n: num(log(d, n), r"requests settled by a grant (\d+)"), None),
    ("UWB-delivered fixes", "end", uwb_fixes, None),
    ("base-station answers", "main", lambda d, n: num(log(d, n), r"TA-1 base-station session \(\S+\): answers delivered (\d+)"), None),
    ("finder-session answers", "main", lambda d, n: num(log(d, n), r"TA-1b finder session \(\S+\): finder \d+ target m\d+, answers (\d+)"), None),
    ("requester escalations accepted", "end", lambda d, n: num(log(d, n), r"TA-2 requester escalation[^\n]*at the targets accepted (\d+)"), "requesterEscalation"),
    ("finder requests granted", "end", lambda d, n: num(log(d, n), r"TA-1b finder requester escalation \(\S+\): [^\n]*granted (\d+)"), "finderRequesterEscalation"),
    ("subjects' finder ranges on reports", "end", lambda d, n: num(log(d, n), r"N\.21 finder pseudo-anchors \(subjects, \S+\): [^\n]*reported positions used (\d+)"), "subjectFinderPseudoAnchor"),
    ("tag 0's finder ranges dropped", "end", lambda d, n: num(log(d, n), r"N\.21 strict pseudo-anchors \(\S+\): tag 0 finder ranges on reports \d+, dropped for want of a position (\d+)"), "finderPseudoAnchorStrict"),
    ("sessions reaching PRECISE", "end", lambda d, n: precise(log(d, n)), None),
    ("edge uplink records", "main", lambda d, n: num(log(d, n), r"SY-1 edge uplink: 2.4 GHz frames \d+ records (\d+)"), "edgeUplinkRanges"),
    ("subjects' uplink records", "main", lambda d, n: num(log(d, n), r"subject \d+ 2\.4 GHz frames \d+ records (\d+)"), "subjectEdgeUplink"),
]


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    runs = [os.path.basename(p)[:-3] for p in glob.glob(os.path.join(d, "*.rc")) if not os.path.basename(p).startswith("r_")]
    bad = [n for n in runs if rc(d, n) != 0]
    report(not bad and "main" in runs, "RUNS", f"{len(runs)} runs exited 0" if not bad else f"non-zero exit: {bad}")

    a, b = os.path.join(d, "eq_preset"), os.path.join(d, "eq_flags")
    fa = sorted(os.path.basename(p) for p in glob.glob(os.path.join(a, "*.csv")))
    fb = sorted(os.path.basename(p) for p in glob.glob(os.path.join(b, "*.csv")))
    diff = [f for f in fa if not os.path.exists(os.path.join(b, f)) or open(os.path.join(a, f), "rb").read() != open(os.path.join(b, f), "rb").read()]
    report(fa and fa == fb and not diff, "EQUIV", f"--specProfile vs its own settings as flags: {len(fa)} CSVs, differing {len(diff)} (need 0)"
           + (f": {diff[:3]}" if diff else ""))

    sp = os.path.join(d, "main", "spec_profile.txt")
    se = os.path.join(d, "end", "spec_profile.txt")
    if not os.path.exists(sp) or not os.path.exists(se):
        missing("FILE", "no spec_profile.txt")
    else:
        rows = [l for l in open(sp) if l.strip() and not l.startswith("#")]
        end = open(se).read()
        kept = all(re.search(rf"^{f}: explicit", end, re.M) for f in ("boxHalf", "finderMode"))
        endlog = log(d, "end")
        ran750 = "--boxHalf=750" in open(os.path.join(d, "end", "run_id.txt")).read() if os.path.exists(os.path.join(d, "end", "run_id.txt")) else False
        report(len(rows) >= 60 and kept and ran750, "FILE",
               f"main lists {len(rows)} settings; end keeps the explicit --boxHalf/--finderMode {kept}; end ran at 750 m {ran750}")

    wl = [m.groups() for n in ("main", "end") for m in re.finditer(r"NW-2 weights alpha (\S+) beta (\S+) gamma (\S+) slack (\S+);", log(d, n))]
    badw = [w for w in wl if tuple(float(x) for x in w) != (0.0, 0.0, 1.0, 1.0)]
    report(len(wl) >= 6 and not badw, "WEIGHTS",
           f"NW-2 routers in main and end {len(wl)} (need 6), not at (0,0,1,1) {len(badw)} (need 0)"
           + (f": {badw[:2]}" if badw else ""))

    hv = [n for n in ("main", "end") if os.path.exists(os.path.join(d, n, "spec_profile.txt"))
          and re.search(r"^hopVarianceTerms", open(os.path.join(d, n, "spec_profile.txt")).read(), re.M)]
    hvl = [n for n in ("main", "end") if "NW-1 hop variance terms" in log(d, n)]
    report(not hv and not hvl, "NOHV",
           f"--hopVarianceTerms set by the preset in {hv} and its NW-1 line in {hvl} (need none)")

    zero = []
    for name, run, reader, flag in MECH:
        v = reader(d, run)
        if v is None:
            missing(f"POP {name}", f"no evidence line in {run}")
            zero.append(name)
            continue
        report(v > 0, f"POP {name}", f"{run}: {int(v)}" + ("" if v > 0 else " (need > 0)"))
        if v <= 0:
            zero.append(name)
        if flag:
            off = f"off_{flag}"
            vo = reader(d, off)
            if rc(d, off) is None:
                missing(f"EFF {flag}", f"no {off} run")
            else:
                vo = 0 if vo is None else vo
                report(vo == 0, f"EFF {flag}", f"with --{flag}=false: {name} {int(vo)} (need 0)")
    print(f"  G24 zero-population mechanisms ({len(zero)}): {zero}")

    t = log(d, "r_topo")
    report(rc(d, "r_topo") == 1 and "--specProfile with --topology=1" in t, "R", f"r_topo: exit {rc(d, 'r_topo')} (need 1)")
    t = log(d, "r_n12")
    report(rc(d, "r_n12") == 1 and "--measuredSubjects > 1 needs --fixRoutingSubjects" in t, "R12",
           f"r_n12 (the preset with --fixRoutingSubjects=false): exit {rc(d, 'r_n12')} (need 1)")
    print(f"  G24: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
