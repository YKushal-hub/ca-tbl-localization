#!/usr/bin/env python3
"""G49: a UWB escalation commits only on a confirmed grant (docs/SPEC_CONFORMANCE_PLAN.md
NW-7, --grantGatedUwb).

MAC sect 8 makes the UWB request an ACK'd exchange so that a node "must know definitively
whether the grant was received"; Transport sect 4 moves the session to LOCATED_PRECISE "ON
UWB grant confirmed". The tier changed inside Step() and nothing at the node read ESC_GRANT,
so a node could run on UWB with no anchor's grant. Under --grantGatedUwb the escalation into
UWB is a pending request; the node stays on 2.4 GHz, frozen (no further escalation, no
healthy demotion; coverage loss still demotes and cancels), until a GRANT(granted) answering
its own request arrives, and moves to UWB at the next cycle. grants_<arm>_seed<N>.csv logs
every request, confirm, deny, timeout, late grant and coverage cancellation per subject.

Reads a directory written by scripts/grant_gated_gate.sh (on: static_868, static_uwb and
ca_tbl_full; subj: five measured subjects; off: the control; r_nofix). Every check runs and
is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  GRANTED  every entry into UWB at cycle T, of every subject, has a "confirm" for that
           subject in the slice (T - dt, T] -- and that confirm is a GRANT(granted) frame
           delivered to the subject by the anchor it names, at that instant, in events_*.
           Every confirm is followed by such an entry. Recomputed from the trace, the grant
           log and the event log, not from the policy.
  REQUESTS the request rows are exactly the trace's pending cycles: escalated on 2.4 GHz
           with nextTier still 2.4 GHz.
  FROZEN   between a request and its resolution no cycle demotes unless it is blind (no fix,
           or the fix came from the 868 floor), and none escalates.
  COUNTS   the NW-7 lines equal the grant log per event; arm_summary's escalations equal the
           trace's 868 -> 2.4 GHz moves plus the confirms.
  STATIC   both static arms request nothing and log nothing.
  CONTROL  without the flag, some UWB entries have no grant in their slice at all (the
           check above would fire on the default), and nothing NW-7 is written.
  SUBJECTS a measured subject other than tag 0 is confirmed onto UWB.
  DIGEST   each grants_ file is in run_id.txt with its sha256.
  R        --grantGatedUwb without --handshakeFixes is refused.
"""
import csv
import hashlib
import os
import re
import sys

LINE = re.compile(r"NW-7 grant-gated UWB \((\w+)\), (tag 0|subjects 2\.\.\d+): requests (\d+), "
                  r"confirmed (\d+), denied (\d+), timed out (\d+), late grants (\d+), aborted "
                  r"(\d+), cancelled by coverage loss (\d+), cycles waiting (\d+)")
EVENTS = ("request", "confirm", "deny", "timeout", "late", "coverage")
OK_OUT = ("delivered", "capture")
DT = 2.5


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(p):
    with open(p) as f:
        return list(csv.DictReader(f))


def seed_of(d, n):
    m = re.search(r"--seed=(\d+)\S*(?:.*--seed=(\d+))?", open(os.path.join(d, n, "run_id.txt")).read())
    return m.group(2) or m.group(1)


def traces(d, n, arm, seed):
    """subjectId -> trace rows (tag 0's trace_, subjects' subjtrace_)."""
    out = {"1": rows(os.path.join(d, n, f"trace_{arm}_seed{seed}.csv"))}
    for f in os.listdir(os.path.join(d, n)):
        m = re.match(rf"subjtrace_{arm}_seed{seed}_subj(\d+)\.csv$", f)
        if m:
            out[m.group(1)] = rows(os.path.join(d, n, f))
    return out


def entries(tr):
    out, prev = [], None
    for r in tr:
        if r["activeTier"] == "UWB" and prev is not None and prev["activeTier"] != "UWB":
            out.append(float(r["t"]))
        prev = r
    return out


def granted_events(d, n, arm, seed):
    out = []
    for r in rows(os.path.join(d, n, f"events_{arm}_seed{seed}.csv")):
        if r["msgType"] == "ESC_GRANT" and r["outcome"] in OK_OUT and "granted=1" in r["payload"]:
            out.append((float(r["t"]), r["txId"], r["rxId"], r["t"]))
    return out


def main():
    if len(sys.argv) != 2:
        print("usage: grant_gated_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    GATED = (("on", "ca_tbl_full"), ("subj", "ca_tbl_full"))

    def c_runs():
        bad = [n for n in ("on", "subj", "off") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_granted():
        bad, n_ent, n_conf = [], 0, 0
        for run, arm in GATED:
            seed = seed_of(d, run)
            tr = traces(d, run, arm, seed)
            gl = rows(os.path.join(d, run, f"grants_{arm}_seed{seed}.csv"))
            ev = granted_events(d, run, arm, seed)
            for sid, t in tr.items():
                conf = [(float(g["t"]), g["anchorId"]) for g in gl
                        if g["subjectId"] == sid and g["event"] == "confirm"]
                ent = entries(t)
                n_ent += len(ent)
                n_conf += len(conf)
                for T in ent:
                    hit = [c for c in conf if T - DT < c[0] <= T + 1e-9]
                    if len(hit) != 1:
                        bad.append((run, sid, T, "confirms in slice", len(hit)))
                        continue
                    tc, anchor = hit[0]
                    # The event log prints time at the stream's default six significant
                    # digits, so the confirm is matched to a relative 1e-5 (a printed value
                    # sits within half a unit of its sixth digit).
                    if not any(abs(te - tc) <= 1e-5 * max(1.0, tc) and tx == anchor and rx == sid
                               for te, tx, rx, _ in ev):
                        bad.append((run, sid, T, "no delivered GRANT(granted) for the confirm"))
                for tc, _ in conf:
                    if not any(tc <= T + 1e-9 and T - DT < tc for T in ent):
                        bad.append((run, sid, tc, "confirm with no UWB entry after it"))
        check("GRANTED", n_ent > 0 and n_ent == n_conf and not bad,
              f"{n_ent} UWB entries, {n_conf} confirms; entries without exactly one confirm "
              f"matched to a delivered grant, or confirms without an entry: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("GRANTED", c_granted)

    def c_requests():
        bad, n = [], 0
        for run, arm in GATED:
            seed = seed_of(d, run)
            tr = traces(d, run, arm, seed)
            gl = rows(os.path.join(d, run, f"grants_{arm}_seed{seed}.csv"))
            for sid, t in tr.items():
                want = sorted(round(float(r["t"]), 4) for r in t
                              if r["escalated"] == "1" and r["activeTier"] == "2.4GHz"
                              and r["nextTier"] == "2.4GHz")
                got = sorted(round(float(g["t"]), 4) for g in gl
                             if g["subjectId"] == sid and g["event"] == "request")
                n += len(got)
                if want != got:
                    bad.append((run, sid, len(want), len(got)))
        check("REQUESTS", n > 0 and not bad,
              f"{n} requests logged; subjects whose request rows are not the trace's pending "
              f"cycles: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("REQUESTS", c_requests)

    def c_frozen():
        bad, windows, inside = [], 0, 0
        for run, arm in GATED:
            seed = seed_of(d, run)
            tr = traces(d, run, arm, seed)
            gl = rows(os.path.join(d, run, f"grants_{arm}_seed{seed}.csv"))
            for sid, t in tr.items():
                ev = sorted((float(g["t"]), g["event"]) for g in gl if g["subjectId"] == sid)
                for i, (tr0, e) in enumerate(ev):
                    if e != "request":
                        continue
                    end = next((te for te, ee in ev[i + 1:]
                                if ee in ("confirm", "deny", "timeout", "coverage")), 1e18)
                    windows += 1
                    for r in t:
                        tt = float(r["t"])
                        if not (tr0 < tt <= end):
                            continue
                        inside += 1
                        blind = r["hadFix"] == "0" or r["usedFloor"] == "1"
                        if (r["deescalated"] == "1" and not blind) or r["escalated"] == "1":
                            bad.append((run, sid, tt, r["escalated"], r["deescalated"]))
        check("FROZEN", windows > 0 and inside > 0 and not bad,
              f"{windows} pending windows, {inside} cycles inside them; a healthy demotion or "
              f"an escalation inside one: {len(bad)} (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("FROZEN", c_frozen)

    def c_counts():
        bad = []
        for run, arm in GATED:
            seed = seed_of(d, run)
            gl = rows(os.path.join(d, run, f"grants_{arm}_seed{seed}.csv"))
            lines = [m for m in LINE.finditer(log(d, run)) if m.group(1) == arm]
            for m in lines:
                tag0 = m.group(2) == "tag 0"
                sel = [g for g in gl if (g["subjectId"] == "1") == tag0]
                got = [sum(g["event"] == e for g in sel) for e in EVENTS]
                want = [int(m.group(k)) for k in (3, 4, 5, 6, 7, 9)]
                if got != want:
                    bad.append((run, m.group(2), want, got))
            tr = traces(d, run, arm, seed)["1"]
            up = sum(1 for r in tr if r["escalated"] == "1" and r["activeTier"] == "868MHz"
                     and r["nextTier"] == "2.4GHz")
            conf = sum(1 for g in gl if g["subjectId"] == "1" and g["event"] == "confirm")
            esc = int(next(r for r in rows(os.path.join(d, run, "arm_summary.csv"))
                           if r["arm"] == arm)["escalations"])
            if esc != up + conf:
                bad.append((run, "escalations", esc, up, conf))
            if not lines:
                bad.append((run, "no NW-7 line"))
        check("COUNTS", not bad,
              f"NW-7 lines vs the grant log, and escalations vs 868->2.4 moves + confirms: "
              f"{len(bad)} mismatches (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("COUNTS", c_counts)

    def c_static():
        bad = []
        for arm in ("static_868", "static_uwb"):
            gl = rows(os.path.join(d, "on", f"grants_{arm}_seed1.csv"))
            ln = [m for m in LINE.finditer(log(d, "on")) if m.group(1) == arm]
            if gl or len(ln) != 1 or any(int(ln[0].group(k)) for k in range(3, 11)):
                bad.append((arm, len(gl), len(ln)))
        check("STATIC", not bad, f"static arms that requested, logged or printed counts: {bad or 'none'}")
    safe("STATIC", c_static)

    def c_control():
        tr = rows(os.path.join(d, "off", "trace_ca_tbl_full_seed1.csv"))
        ev = granted_events(d, "off", "ca_tbl_full", "1")
        ent = entries(tr)
        ungranted = sum(1 for T in ent
                        if not any(T - DT < te <= T and rx == "1" for te, _, rx, _ in ev))
        files = [f for f in os.listdir(os.path.join(d, "off")) if f.startswith("grants_")]
        lines = log(d, "off").count("NW-7")
        check("CONTROL", ungranted > 0 and not files and lines == 0,
              f"without the flag: {ungranted} of {len(ent)} UWB entries had no grant in their slice "
              f"(need > 0); grants_ files {len(files)}, NW-7 lines {lines} (need 0, 0)")
    safe("CONTROL", c_control)

    def c_subjects():
        seed = seed_of(d, "subj")
        gl = rows(os.path.join(d, "subj", f"grants_ca_tbl_full_seed{seed}.csv"))
        who = sorted({g["subjectId"] for g in gl if g["event"] == "confirm" and g["subjectId"] != "1"})
        check("SUBJECTS", len(who) > 0, f"subjects other than tag 0 confirmed onto UWB: {who} (need some)")
    safe("SUBJECTS", c_subjects)

    def c_digest():
        bad, n = [], 0
        for run in ("on", "subj"):
            dg = open(os.path.join(d, run, "run_id.txt")).read()
            for f in os.listdir(os.path.join(d, run)):
                if not f.startswith("grants_"):
                    continue
                n += 1
                h = hashlib.sha256(open(os.path.join(d, run, f), "rb").read()).hexdigest()
                if f"{h}  {f}" not in dg:
                    bad.append((run, f))
        check("DIGEST", n == 4 and not bad, f"{n} grants_ files (need 4); not digested as written: "
                                            f"{bad or 'none'}")
    safe("DIGEST", c_digest)

    def c_ref():
        got = rc(d, "r_nofix")
        has = "FATAL: --grantGatedUwb needs --handshakeFixes=true" in log(d, "r_nofix")
        check("R", got == 1 and has, f"r_nofix: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G49 {tag:7s} {name:8s} {why}")
    print(f"  G49: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
