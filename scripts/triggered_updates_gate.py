#!/usr/bin/env python3
"""G34: the spec's triggered update and last gasp (docs/SPEC_CONFORMANCE_PLAN.md MC-2,
--triggeredUpdates).

MAC sect 3 and 8: "Triggered-update / last-gasp -- large correction or dying battery", no
ACK, 2-3 jittered repeats. By default the only TRIGGERED_UPDATE is StartHandshake's
RECEDING push. The flag adds, for the measured node (id 1): a LARGE CORRECTION when the
posterior moves more than 3 sqrt(trace P) of the previous posterior from the
constant-velocity prediction, sent with the redundant class's repeats; and a LAST GASP,
one unrepeated frame queued as --batteryDepleteJ is crossed, before the radios fall
silent. Each is a row of triggered_ca_tbl_full_seed7.csv and carries reason=1 or 2 in the
event payload.

Reads a directory written by scripts/triggered_updates_gate.sh (def, off, on; seed 7,
900 s, a battery budget spent mid-run). Every check runs and is reported; exit 1 if any
failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL   --triggeredUpdates=false IS the default: every file byte-identical; no MC-2 line,
         no triggered_ file and no reason= mark under the default.
  LARGE  every large_correction row is self-consistent and agrees with the TRACE, an
         independent record: its estX/estY are the trace's at that t; its threshold is
         k sqrt(traceSigma) of the previous trace row (k = 1, --triggeredUpdateK's
         default; traceSigma = trace of the posterior covariance); moveM is |est - pred| recomputed from its own columns and
         exceeds the threshold; its tier is the trace's activeTier at that t (CLAUDE.md
         sect 6: it rides the radio that transmitted). At least one row.
  FRAMES every node-1 reason=1 frame heard in the events file follows a large-correction
         row by at most 2 s (CAD, then repeats jittered within the rebroadcast window),
         and some are heard; every reason=2 frame follows the last gasp the same way.
  GASP   exactly one last_gasp row, at the instant the stdout says the battery died; at
         most one distinct node-1 reason=2 frame; and it is the LAST frame node 1
         ORIGINATED that anyone heard -- the gasp got out, and nothing of its own after it.
         Originated, not transmitted: a node's packet ids are node_id << 20 plus a count,
         and a silenced radio still REBROADCASTS other nodes' floods (MaybeRebroadcast
         does not consult SetSilent; TECH_DEBT N.1), so frames with txId 1 continue after
         the battery dies, in the default run as much as under the flag.
  LINE   the MC-2 stdout counts equal the CSV's rows per kind; frames transmitted do not
         exceed frames billed.
  R      --triggeredUpdateK without --triggeredUpdates is refused (exit 1).
"""
import csv
import math
import os
import re
import sys

LINE = re.compile(r"MC-2 triggered updates: large correction (\d+) \(heard (\d+)\), last gasp "
                  r"(\d+) \(heard (\d+)\); frames billed (\d+), transmitted (\d+)")
DEAD = re.compile(r"battery exhausted at t=([0-9.]+) s")
K = 1.0  # --triggeredUpdateK default


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(d, n, f):
    return list(csv.DictReader(open(os.path.join(d, n, f))))


def field(p, k):
    m = re.search(r"(?:^|;)" + k + r"=(\d+)", p)
    return int(m.group(1)) if m else None


def node1_frames(ev):
    """{(tier, pkt): (first t, payload, msgType)} for frames node 1 ORIGINATED (packet id
    node_id << 20 plus a count), wherever heard -- its own and others' rebroadcasts."""
    out = {}
    for r in ev:
        pkt = field(r["payload"], "pkt")
        if pkt is None or pkt >> 20 != 1:
            continue
        k = (r["tier"], pkt)
        t = float(r["t"])
        if k not in out or t < out[k][0]:
            out[k] = (t, r["payload"], r["msgType"])
    return out


def main():
    if len(sys.argv) != 2:
        print("usage: triggered_updates_gate.py <dir>", file=sys.stderr)
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

    def c_runs():
        bad = [n for n in ("def", "off", "on") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fo = sorted(f for f in os.listdir(os.path.join(d, "off")) if f != "run_id.txt")
        same = fd == fo and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "off", f), "rb").read() for f in fd)
        lines = log(d, "def").count("MC-2")
        trig = [f for f in fd if f.startswith("triggered_")]
        marks = open(os.path.join(d, "def", "events_ca_tbl_full_seed7.csv")).read().count(";reason=")
        check("NULL", same and fd and lines == 0 and not trig and marks == 0,
              f"--triggeredUpdates=false vs default: {len(fd)} files, "
              f"{'identical' if same else 'DIFFER'}; under the default MC-2 lines {lines}, "
              f"triggered_ files {len(trig)}, reason= marks {marks} (need 0, 0, 0)")
    safe("NULL", c_null)

    def c_large():
        tg = [r for r in rows(d, "on", "triggered_ca_tbl_full_seed7.csv") if r["kind"] == "large_correction"]
        tr = rows(d, "on", "trace_ca_tbl_full_seed7.csv")
        idx = {round(float(r["t"]), 4): i for i, r in enumerate(tr)}
        tiers = {"868MHz": 0, "2.4GHz": 1, "UWB": 2}
        bad = []
        for r in tg:
            t = round(float(r["t"]), 4)
            i = idx[t]
            ex, ey = float(r["estX"]), float(r["estY"])
            px, py = float(r["predX"]), float(r["predY"])
            move, thr = float(r["moveM"]), float(r["threshM"])
            thr_tr = K * math.sqrt(float(tr[i - 1]["traceSigma"]))
            ok = (abs(ex - float(tr[i]["estX"])) < 1e-3 and abs(ey - float(tr[i]["estY"])) < 1e-3
                  and abs(math.hypot(ex - px, ey - py) - move) < 1e-3 and move > thr
                  and abs(thr - thr_tr) <= 1e-3 * max(1.0, thr)
                  and int(r["tier"]) == tiers[tr[i]["activeTier"]])
            if not ok:
                bad.append((t, move, thr, thr_tr, r["tier"], tr[i]["activeTier"]))
        check("LARGE", tg and not bad,
              f"on: {len(tg)} large-correction rows; {len(bad)} disagree with the trace or "
              f"themselves (need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("LARGE", c_large)

    def c_frames():
        tg = rows(d, "on", "triggered_ca_tbl_full_seed7.csv")
        tl = [float(r["t"]) for r in tg if r["kind"] == "large_correction"]
        tgp = [float(r["t"]) for r in tg if r["kind"] == "last_gasp"]
        fr = node1_frames(rows(d, "on", "events_ca_tbl_full_seed7.csv"))
        r1 = [v[0] for v in fr.values() if field(v[1], "reason") == 1]
        r2 = [v[0] for v in fr.values() if field(v[1], "reason") == 2]
        stray1 = [x for x in r1 if not any(0.0 <= x - t0 <= 2.0 for t0 in tl)]
        stray2 = [x for x in r2 if not any(0.0 <= x - t0 <= 2.0 for t0 in tgp)]
        check("FRAMES", r1 and not stray1 and not stray2,
              f"on: node-1 reason=1 frames heard {len(r1)} (need > 0), more than 2 s after "
              f"any trigger {len(stray1)}; reason=2 frames {len(r2)}, stray {len(stray2)} "
              f"(need 0, 0)")
    safe("FRAMES", c_frames)

    def c_gasp():
        tg = [r for r in rows(d, "on", "triggered_ca_tbl_full_seed7.csv") if r["kind"] == "last_gasp"]
        m = DEAD.search(log(d, "on"))
        fr = node1_frames(rows(d, "on", "events_ca_tbl_full_seed7.csv"))
        r2 = sorted(v[0] for v in fr.values() if field(v[1], "reason") == 2)
        last = max(v[0] for v in fr.values())
        tdead = float(m.group(1)) if m else None
        ok = (len(tg) == 1 and m and abs(float(tg[0]["t"]) - tdead) < 0.05 and len(r2) <= 1
              and (not r2 or r2[0] == last))
        check("GASP", ok,
              f"on: last_gasp rows {len(tg)} (need 1) at t={tg[0]['t'] if tg else '?'}, battery "
              f"dead at {tdead}; node-1 reason=2 frames {len(r2)} (need <= 1); its time "
              f"{r2[0] if r2 else '-'} vs the last frame node 1 originated, heard {last} "
              f"(need equal)")
    safe("GASP", c_gasp)

    def c_line():
        m = LINE.search(log(d, "on"))
        tg = rows(d, "on", "triggered_ca_tbl_full_seed7.csv")
        nl = sum(r["kind"] == "large_correction" for r in tg)
        ng = sum(r["kind"] == "last_gasp" for r in tg)
        billed = sum(int(r["framesBilled"]) for r in tg)
        ok = m and int(m.group(1)) == nl and int(m.group(3)) == ng and \
            int(m.group(5)) == billed and int(m.group(6)) <= billed
        check("LINE", ok,
              f"on: MC-2 line {m.groups() if m else 'ABSENT'}; CSV large {nl}, gasp {ng}, "
              f"billed {billed}")
    safe("LINE", c_line)

    def c_ref():
        got = rc(d, "r_k")
        has = "--triggeredUpdateK must be > 0 and needs --triggeredUpdates=true" in log(d, "r_k")
        check("R", got == 1 and has, f"r_k: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G34 {tag:7s} {name:6s} {why}")
    print(f"  G34: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
