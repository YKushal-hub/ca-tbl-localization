#!/usr/bin/env python3
"""G29: a SUSPECT fix is downweighted, not rejected (docs/SPEC_CONFORMANCE_PLAN.md DC-7).

The spec (Physical 2.6): flag the fix SUSPECT, do not let it alone justify a
de-escalation, and "downweight in PCS regardless of raw trace(Sigma)". Under
--suspectRecovery=downweight the fix is fused at R x --suspectInflateFactor and the
cycle's PCS is scaled by --suspectPcsFactor.

Reads a directory written by scripts/suspect_downweight_gate.sh: one sub-directory per
run, with <name>.log and <name>.rc beside it. Every check runs and is reported; exit 1 if
any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  POP    downweight: the trace has suspect cycles, and not one is rejected. The default
         (none), the negative control that shows the column means what it says: every
         suspect cycle rejected.
  COUNT  the stdout line's "N of M" is N = M = the trace's suspect rows = arm_summary's
         suspects: every suspect fix went down the downweight path (recounted, rule 3).
  EXACT  at inflate 1 and PCS factor 1, downweight IS bounded with N = 1 (a full-R
         force-accept of the same cycle): the traces are identical in every column but
         `rejected`, which differs on exactly the suspect rows (bounded 1, downweight 0),
         and arm_summary.csv is byte-identical.
  PCSF   the PCS factor alone (0.5 against 1, both at inflate 1): the traces differ in
         pcs only, only on suspect rows, and there by exactly x 0.5 (to the 4 dp
         printed); arm_summary.csv byte-identical. The factor touches nothing else.
  RINF   inflate 4 against 1, both at PCS factor 1: every row before the first suspect is
         identical, and at it the inflated run's trace(P) is LARGER -- it fused less of
         the suspect fix.
  R      refusals, each exit 1 with its reason: --suspectPcsFactor without downweight
         (it would do nothing), 0 and 1.5 (not a downweight), and an unknown mode.
"""
import csv
import os
import sys

TOL = 1e-4  # pcs printed to 4 dp: 5e-5 on each side, the 0.5 halves the second


def rows(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def read(path):
    with open(path, "rb") as f:
        return f.read()


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


TR = "trace_ca_tbl_full_seed7.csv"


def main():
    if len(sys.argv) != 2:
        print("usage: suspect_downweight_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, StopIteration) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def tr(n):
        return rows(os.path.join(d, n, TR))

    def same_summary(a, b):
        return read(os.path.join(d, a, "arm_summary.csv")) == read(os.path.join(d, b, "arm_summary.csv"))

    runs = ["none", "dw", "dw11", "b1", "dw1h", "dw4_1"]

    def c_runs():
        bad = [n for n in runs if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_pop():
        t = tr("dw")
        s = [r for r in t if r["suspect"] == "1"]
        rej = sum(r["rejected"] == "1" for r in t)
        check("POP", len(s) > 0 and rej == 0,
              f"downweight: {len(s)} suspect cycles (need > 0), {rej} rejected cycles (need 0)")
        t = tr("none")
        s = [r for r in t if r["suspect"] == "1"]
        rs = sum(r["rejected"] == "1" for r in s)
        check("POP", len(s) > 0 and rs == len(s),
              f"none (the control): {rs} of {len(s)} suspect cycles rejected (need all, > 0)")
    safe("POP", c_pop)

    def c_count():
        line = [l for l in log(d, "dw").splitlines() if "DC-7 downweighted suspect fixes:" in l]
        if len(line) != 1:
            check("COUNT", False, f"{len(line)} 'DC-7 downweighted' lines on stdout (need 1)")
            return
        w = line[0].split(":", 1)[1].split()
        n, m = int(w[0]), int(w[2])
        ntr = sum(r["suspect"] == "1" for r in tr("dw"))
        summ = rows(os.path.join(d, "dw", "arm_summary.csv"))
        ns = int(summ[0]["suspects"]) if len(summ) == 1 else -1
        check("COUNT", n == m == ntr == ns and n > 0,
              f"stdout {n} of {m}; trace suspect rows {ntr}; arm_summary suspects {ns} (need all equal, > 0)")
        none_lines = [l for l in log(d, "none").splitlines() if "DC-7" in l]
        check("COUNT", not none_lines,
              f"the line is flag-gated: {len(none_lines)} under --suspectRecovery=none (need 0)")
    safe("COUNT", c_count)

    def c_exact():
        a, b = tr("dw11"), tr("b1")
        if len(a) != len(b) or not a:
            check("EXACT", False, f"trace lengths {len(a)} vs {len(b)}")
            return
        other = [k for k in a[0] if k != "rejected"]
        nd = sum(1 for x, y in zip(a, b) if any(x[k] != y[k] for k in other))
        wrong = sum(1 for x, y in zip(a, b)
                    if (x["rejected"] != y["rejected"]) !=
                    (x["suspect"] == "1" and x["rejected"] == "0" and y["rejected"] == "1"))
        ns = sum(x["suspect"] == "1" for x in a)
        ss = same_summary("dw11", "b1")
        check("EXACT", nd == 0 and wrong == 0 and ns > 0 and ss,
              f"downweight at (1, 1) vs bounded N=1: {nd} rows differ outside `rejected` (need 0); "
              f"{wrong} rows where `rejected` differs other than on a suspect row as 0 vs 1 "
              f"(need 0; {ns} suspect rows); arm_summary {'identical' if ss else 'DIFFERS'}")
    safe("EXACT", c_exact)

    def c_pcsf():
        a, b = tr("dw1h"), tr("dw11")
        if len(a) != len(b) or not a:
            check("PCSF", False, f"trace lengths {len(a)} vs {len(b)}")
            return
        other = [k for k in a[0] if k != "pcs"]
        nd = sum(1 for x, y in zip(a, b) if any(x[k] != y[k] for k in other))
        clean_moved = sum(1 for x, y in zip(a, b) if x["suspect"] != "1" and x["pcs"] != y["pcs"])
        worst, n, big = 0.0, 0, 0
        for x, y in zip(a, b):
            if x["suspect"] == "1":
                n += 1
                worst = max(worst, abs(float(x["pcs"]) - 0.5 * float(y["pcs"])))
                big += float(y["pcs"]) > 10 * TOL
        ss = same_summary("dw1h", "dw11")
        check("PCSF", nd == 0 and clean_moved == 0 and n > 0 and big > 0 and worst <= TOL and ss,
              f"--suspectPcsFactor 0.5 vs 1: {nd} rows differ outside pcs (need 0); pcs moved on "
              f"{clean_moved} clean rows (need 0); on {n} suspect rows |pcs - 0.5 x pcs(1)| <= "
              f"{worst:.1e} (need <= {TOL:.0e}; {big} rows well above rounding); arm_summary "
              f"{'identical' if ss else 'DIFFERS'}")
    safe("PCSF", c_pcsf)

    def c_rinf():
        a, b = tr("dw4_1"), tr("dw11")
        first = next((i for i, x in enumerate(b) if x["suspect"] == "1"), None)
        if first is None:
            check("RINF", False, "no suspect row at inflate 1")
            return
        before = sum(1 for x, y in zip(a[:first], b[:first]) if x != y)
        sa, sb = a[first], b[first]
        ok = (before == 0 and sa["suspect"] == "1" and sa["t"] == sb["t"] and
              float(sa["traceSigma"]) > float(sb["traceSigma"]))
        check("RINF", ok,
              f"inflate 4 vs 1: {before} rows differ before the first suspect (need 0); at it "
              f"(t = {sb['t']}) trace(P) {sa['traceSigma']} vs {sb['traceSigma']} (need larger at 4)")
    safe("RINF", c_rinf)

    def c_ref():
        for n, text in [("r_nodw", "read only under --suspectRecovery=downweight"),
                        ("r_zero", "must lie in (0, 1]"),
                        ("r_big", "must lie in (0, 1]"),
                        ("r_mode", "none|inflate|bounded|reinit|downweight")]:
            got, has = rc(d, n), text in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G29 {tag:7s} {name:5s} {why}")
    print(f"  G29: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
