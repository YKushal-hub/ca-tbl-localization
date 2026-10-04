#!/usr/bin/env python3
"""G67: subjects run the battery fault and its last gasp (--subjectBatteryDeplete; TECH_DEBT
N.3, register row 55).

usage: subject_battery_gate.py <dir written by scripts/subject_battery_gate.sh>

Runs read: on/ and off/ (G34's regime, five subjects, a 3 J budget, triggered updates on for
everyone), r_nobat (refusal: without --batteryDepleteJ).

What a firing looks like on disk. A subject whose energy crosses the budget gets one FAULT
line at that cycle; its trace shows the crossing there; one last_gasp row in
subjtriggered_*, and one reason=2 frame heard; after that it fixes nothing, and no frame it
originated is heard except the gasp. (Listen and sleep are still billed after the death,
for tag 0 as for subjects: TECH_DEBT N.20. So energy is not asserted to stop.)

Checks (each prints "G67 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     on and off exited 0.
  DEAD     every FAULT (subject) line's time is the cycle the subject's trace crosses the
           budget: the first row with cumJ >= J, or the next (the trace row also carries
           that cycle's listen and sleep); at least one subject died.
  SILENT   after its death, a dead subject's trace has no fix, and no frame it originated is
           first heard more than 2 s after it, except the last gasp.
  GASP     each dead subject has exactly one last_gasp row, at its death, and a reason=2 frame
           of its own heard within 2 s.
  COUNTS   the MC-2 subjects line's "last gasp" equals the last_gasp rows.
  CONTROL  off/: no FAULT (subject) line, and the start-up WARNING names --batteryDepleteJ.
  R        the refusal exits 1 with the reason.
"""
import csv
import glob
import os
import re
import sys

J = 3.0
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G67 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G67 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G67 MISSING {label:<8}{msg}")


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


def field(p, k):
    m = re.search(r"(?:^|;)" + k + r"=(\d+)", p)
    return int(m.group(1)) if m else None


FAULT = re.compile(r"FAULT sect 49 \(subject (\d+)\): battery exhausted at t=(\S+) s")


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "on and off exited 0" if not bad else f"non-zero exit: {bad}")
    lg = read(os.path.join(d, "on.log")) or ""
    dead = {int(m.group(1)): float(m.group(2)) for m in FAULT.finditer(lg)}
    tr = {}
    for f in glob.glob(os.path.join(d, "on", "subjtrace_ca_tbl_full_seed7_subj*.csv")):
        tr[int(re.search(r"_subj(\d+)\.csv$", f).group(1))] = rows(f)
    db = []
    for sid, td in dead.items():
        r = tr.get(sid) or []
        first = next((k for k, x in enumerate(r) if float(x["cumJ"]) >= J), None)
        i = next((k for k, x in enumerate(r) if abs(float(x["t"]) - td) < 1e-3), None)
        if first is None or i is None or i not in (first, first + 1):
            db.append((sid, td, first, i))
    # a subject that crossed the budget must have died
    for sid, r in tr.items():
        if any(float(x["cumJ"]) >= J for x in r[:-1]) and sid not in dead:
            db.append((sid, "crossed the budget, no FAULT line"))
    report(dead and not db, "DEAD", f"subjects dead {sorted(dead)}; disagreeing with their traces {len(db)} (need 0)" + (f": {db[:3]}" if db else ""))

    ev = rows(os.path.join(d, "on", "events_ca_tbl_full_seed7.csv")) or []
    first_heard = {}
    for e in ev:
        pkt = field(e["payload"], "pkt")
        if pkt is None:
            continue
        k = (e["tier"], pkt)
        t = float(e["t"])
        if k not in first_heard or t < first_heard[k][0]:
            first_heard[k] = (t, pkt >> 20, field(e["payload"], "reason"), e["msgType"])
    sb = []
    gasps_heard = {}
    for (tier, pkt), (t, node, reason, mt) in first_heard.items():
        if node in dead and t > dead[node] + 2.0:
            sb.append((node, t, mt, reason))
        if node in dead and reason == 2 and 0.0 <= t - dead[node] <= 2.0:
            gasps_heard[node] = gasps_heard.get(node, 0) + 1
    for sid, td in dead.items():
        fx = [x["t"] for x in (tr.get(sid) or []) if float(x["t"]) > td + 1e-6 and x["hadFix"] == "1"]
        if fx:
            sb.append((sid, "fix after death", fx[:2]))
    report(not sb, "SILENT", f"after death: frames originated and fixes {len(sb)} (need 0)" + (f": {sb[:3]}" if sb else ""))

    tg = rows(os.path.join(d, "on", "subjtriggered_ca_tbl_full_seed7.csv")) or []
    gp = {}
    for r in tg:
        if r["kind"] == "last_gasp":
            gp.setdefault(int(r["subjectId"]), []).append(float(r["t"]))
    gb = [(sid, gp.get(sid), gasps_heard.get(sid, 0)) for sid, td in dead.items()
          if len(gp.get(sid, [])) != 1 or abs(gp[sid][0] - td) > 1e-3 or gasps_heard.get(sid, 0) < 1]
    gb += [(sid, "gasp without a death") for sid in gp if sid not in dead]
    report(dead and not gb, "GASP", f"dead subjects with exactly one last gasp at death and its frame heard: "
           f"{len(dead) - len([g for g in gb if g[0] in dead])} of {len(dead)}; bad {len(gb)} (need 0)" + (f": {gb[:3]}" if gb else ""))

    m = re.search(r"MC-2 triggered updates \(subjects, \w+\): [^\n]*last gasp (\d+)", lg)
    n = sum(len(v) for v in gp.values())
    report(m is not None and int(m.group(1)) == n, "COUNTS", f"MC-2 subjects line last gasp {m.group(1) if m else None}; rows {n}")
    offlog = read(os.path.join(d, "off.log")) or ""
    w = "--batteryDepleteJ" in "".join(l for l in offlog.split("\n") if "WARNING" in l)
    report(not FAULT.search(offlog) and w, "CONTROL", f"off/: FAULT (subject) lines {len(FAULT.findall(offlog))}, warning names --batteryDepleteJ {w} (need 0, True)")
    t = read(os.path.join(d, "r_nobat.log")) or ""
    report(rc(d, "r_nobat") == 1 and "--subjectBatteryDeplete needs" in t, "R", f"without --batteryDepleteJ: exit {rc(d, 'r_nobat')} (need 1)")
    print(f"  G67: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
