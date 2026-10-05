#!/usr/bin/env python3
"""G55: LOCATED_PRECISE on a confirmed UWB grant, and the batch's precision spread
(docs/SPEC_CONFORMANCE_PLAN.md TA-3 + TA-4, --sessionPreciseOnGrant).

Transport sect 4: "ON UWB grant confirmed: session.state = LOCATED_PRECISE". The session went
precise on any result from a UWB fix. Under --sessionPreciseOnGrant the requester learns of
a grant the only way it can -- a delivered entry saying its producer is on UWB with a
grantor (NW-7, NW-9) -- and a UWB fix without one is coarse. TA-4: the batch session reports
its spread in time-to-precision, or UNDEFINED below two targets.

Reads a directory written by scripts/precise_gate.sh (grant, tier, r_nogate). Every check
runs and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9), from session_*:
  GRANT    every first-precise moment of a target (TA-1 line) is a result row for it whose
           entry carried a confirmed grant (uwbActive 1, grantedBy != 0), the first such
           non-suspect one; the session moved to LOCATED_PRECISE on one; some target did.
  NOTIER   no target is stamped precise by a result from a UWB fix (tierSource 2) without
           a grant, and the session never became precise on such a row.
  SPREAD   the batch spread is max - min of the targets' first-precise times when two or
           more have one (0.1 s), and UNDEFINED otherwise.
  CONTROL  without the flag the first-precise moments are the first UWB-fix results
           (the default rule), whatever their grant.
  R        --sessionPreciseOnGrant without --grantGatedUwb is refused.
"""
import csv
import os
import re
import sys

LINE = re.compile(r"TA-1 base-station session \((\w+)\): answers delivered (\d+), path (\S+); "
                  r"first coarse/precise \(s\):(.*?)(?:; batch precision spread (\S+)(?: s)?)?$",
                  re.M)


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(p):
    with open(p) as f:
        return list(csv.DictReader(f))


def parse(d, n):
    m = LINE.search(log(d, n))
    if not m:
        return None, None
    per = {mm.group(1): (float(mm.group(2)), float(mm.group(3)))
           for mm in re.finditer(r"m(\d+) (-?[0-9.]+)/(-?[0-9.]+)", m.group(4))}
    return per, m.group(5)


def main():
    if len(sys.argv) != 2:
        print("usage: precise_gate.py <dir>", file=sys.stderr)
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

    def sess(n):
        return rows(os.path.join(d, n, "session_ca_tbl_full_seed7.csv"))

    def c_runs():
        bad = [n for n in ("grant", "tier") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def first_by(rs, pred):
        out = {}
        for r in rs:
            if r["event"] == "result" and r["suspect"] == "0" and pred(r) and r["manifest"] not in out:
                out[r["manifest"]] = float(r["t"])
        return out

    def c_grant():
        rs = sess("grant")
        per, _ = parse(d, "grant")
        want = first_by(rs, lambda r: r["uwbActive"] == "1" and r["grantedBy"] != "0")
        bad = []
        for man, (_, prec) in (per or {}).items():
            w = want.get(man)
            if (w is None) != (prec < 0) or (w is not None and abs(prec - w) > 0.05):
                bad.append((man, prec, w))
        moved = [r for r in rs if r["stateAfter"] == "LOCATED_PRECISE" and r["stateBefore"] != "LOCATED_PRECISE"]
        moved_ok = all(r["event"] == "result" and r["grantedBy"] != "0" for r in moved)
        check("GRANT", per and want and not bad and moved and moved_ok,
              f"targets precise on a grant {sorted(want)} (need some); first-precise times not the "
              f"first granted result: {len(bad)} (need 0); moves to PRECISE {len(moved)}, all on "
              f"a granted result {'yes' if moved_ok else 'NO'}{': ' + str(bad[:2]) if bad else ''}")
    safe("GRANT", c_grant)

    def c_notier():
        rs = sess("grant")
        per, _ = parse(d, "grant")
        ungranted_uwb = first_by(rs, lambda r: r["tierSource"] == "2" and r["grantedBy"] == "0")
        granted = first_by(rs, lambda r: r["uwbActive"] == "1" and r["grantedBy"] != "0")
        bad = [m for m, t in ungranted_uwb.items()
               if per and m in per and 0 <= per[m][1] < granted.get(m, 1e18) - 0.05]
        check("NOTIER", not bad,
              f"targets stamped precise by an ungranted UWB-fix result: {bad or 'none'}; such "
              f"results seen for {sorted(ungranted_uwb)}")
    safe("NOTIER", c_notier)

    def c_spread():
        per, spread = parse(d, "grant")
        precs = [p for _, p in (per or {}).values() if p >= 0]
        if len(precs) >= 2:
            ok = spread not in (None, "UNDEFINED") and abs(float(spread) - (max(precs) - min(precs))) <= 0.1
        else:
            ok = spread == "UNDEFINED"
        check("SPREAD", per is not None and ok,
              f"targets precise {len(precs)}; spread printed {spread} (need max - min, or "
              f"UNDEFINED below two)")
    safe("SPREAD", c_spread)

    def c_control():
        rs = sess("tier")
        per, _ = parse(d, "tier")
        want = first_by(rs, lambda r: r["tierSource"] == "2")
        bad = []
        for man, (_, prec) in (per or {}).items():
            w = want.get(man)
            if (w is None) != (prec < 0) or (w is not None and abs(prec - w) > 0.05):
                bad.append((man, prec, w))
        check("CONTROL", per is not None and not bad,
              f"without the flag, first-precise times not the first UWB-fix result: {len(bad)} "
              f"(need 0){': ' + str(bad[:2]) if bad else ''}")
    safe("CONTROL", c_control)

    def c_ref():
        got = rc(d, "r_nogate")
        has = "FATAL: --sessionPreciseOnGrant needs --sessionOverMesh and --grantGatedUwb" in log(d, "r_nogate")
        check("R", got == 1 and has, f"r_nogate: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G55 {tag:7s} {name:7s} {why}")
    print(f"  G55: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
