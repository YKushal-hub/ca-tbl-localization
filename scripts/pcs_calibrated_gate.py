#!/usr/bin/env python3
"""G28: the spec's calibrated PCS is expressible, and what is expressed reaches the policy
(docs/SPEC_CONFORMANCE_PLAN.md DC-5/DC-6).

Reads a directory written by scripts/pcs_calibrated_gate.sh: one sub-directory per run,
plus <name>.log (stdout+stderr) and <name>.rc (exit code) beside it. Every check runs and
is reported; the exit code is 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9), per check:
  P1  calibrated with weights (1, 1, 1) and the legacy thresholds IS the legacy sum --
      1.0*t is exact in IEEE arithmetic and the terms are added in the same order -- so its
      arm_summary.csv and trace must be byte-identical to --pcsForm=legacy's. A path that
      dropped a weight, a threshold or the form would not be.
  P2/P3  at the LOOCV weights (3, 0, 0), every escalation threshold below any reachable
      score gives 0 escalations, and every one above it gives some: the thresholds steer.
  W0  --pcsW2=0 against the hand-set weighted form: the traces differ ONLY in pcs, and on
      every row with a score the difference is 0.5/max(gdop, 1) -- the geometry term at
      the default weight 0.5, removed. Zero was silently the default until 2026-09-30.
  T868 / T2P4 / TD2P4, each ONE threshold moved alone, the evidence gate off so tier 3 is
      reachable: --pcsThrUp868=-1 escalates never; --pcsThrUp2p4=-1 still escalates to
      2.4 GHz but never reaches UWB; --pcsThrDown2p4=10 changes the run.
  SLOTS  the NOTE the binary prints of what it resolved carries each flag's value in its
      own tier's slot; under v2 the unset slots are v2's derived ones, and under a band an
      explicit down threshold wins over the band's.
  R*  refusals: calibrated with a weight or a threshold missing, an inverted pair given
      explicitly, an inverted pair resolved against a built-in value, and the 868 MHz
      down threshold, which is not a flag -- each exits non-zero with its reason. The 868
      MHz up threshold above the built-in 1.75 is accepted: that down threshold decides
      nothing.

NOT CHECKED, AND WHY. The UWB thresholds' effect on a run. No regime this build has
delivers a UWB fix: with the evidence gate off, NC9's finder regime spends 155 cycles on
UWB with nbrUwb = 0 on every one, each fixed by the tier-1 floor, and so every UWB cycle
takes the blind-demotion path, which never reads a PCS threshold. That is the P5
checkpoint's question (plan MS-3), not this gate's; SLOTS shows the values reach the
config.
"""
import csv
import os
import sys

TOL_W0 = 1.5e-4  # pcs and gdop are printed to 4 dp: 2 x 5e-5 on pcs + 0.5 x 5e-5 on gdop


def rows(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def summary(d, name):
    r = rows(os.path.join(d, name, "arm_summary.csv"))
    if len(r) != 1:
        raise ValueError(f"{name}/arm_summary.csv has {len(r)} rows, expected 1")
    return r[0]


def read(path):
    with open(path, "rb") as f:
        return f.read()


def note(d, name):
    with open(os.path.join(d, name + ".log")) as f:
        m = [l for l in f if l.startswith("NOTE: PCS form")]
    if len(m) != 1:
        return None
    lab = m[0].split("resolved:", 1)[1]
    ups = lab.split("up 868/2p4/uwb", 1)[1].split(";", 1)[0].split()
    downs = lab.split("down 2p4/uwb", 1)[1].split(";", 1)[0].split()
    w = lab.split("weights", 1)[1].split() if "weights" in lab else []
    return [float(x) for x in ups], [float(x) for x in downs], [float(x) for x in w]


def rc(d, name):
    with open(os.path.join(d, name + ".rc")) as f:
        return int(f.read().strip())


def log_has(d, name, text):
    with open(os.path.join(d, name + ".log")) as f:
        return text in f.read()


def main():
    if len(sys.argv) != 2:
        print("usage: pcs_calibrated_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    results = []

    def check(name, ok, why):
        results.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, StopIteration) as e:
            results.append((name, None, f"input missing or malformed: {e}"))

    tr = "trace_ca_tbl_full_seed7.csv"

    # every scenario run the checks below read must have exited 0
    runs = ["legacy", "cal111", "never", "always", "wdef", "w0",
            "ev_leg", "ev_up868", "ev_up2p4", "ev_dn2p4", "slots", "slots_v2", "slots_band"]

    def c_rc():
        bad = [n for n in runs if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_rc)

    def c_p1():
        same_s = read(os.path.join(d, "legacy", "arm_summary.csv")) == \
            read(os.path.join(d, "cal111", "arm_summary.csv"))
        same_t = read(os.path.join(d, "legacy", tr)) == read(os.path.join(d, "cal111", tr))
        check("P1", same_s and same_t,
              "calibrated (1,1,1) + legacy thresholds == legacy: arm_summary "
              f"{'identical' if same_s else 'DIFFERS'}, trace {'identical' if same_t else 'DIFFERS'}")
    safe("P1", c_p1)

    def c_p23():
        e2 = int(summary(d, "never")["escalations"])
        e3 = int(summary(d, "always")["escalations"])
        check("P2", e2 == 0, f"(3,0,0), every up threshold -1: escalations {e2} (need 0)")
        check("P3", e3 > 0, f"(3,0,0), every up threshold 10: escalations {e3} (need > 0)")
    safe("P2/P3", c_p23)

    def c_w0():
        a = rows(os.path.join(d, "wdef", tr))
        b = rows(os.path.join(d, "w0", tr))
        if len(a) != len(b) or not a:
            check("W0", False, f"trace lengths {len(a)} vs {len(b)}")
            return
        other = [k for k in a[0] if k != "pcs"]
        nd = sum(1 for x, y in zip(a, b) if any(x[k] != y[k] for k in other))
        worst = 0.0
        scored = 0
        big = 0
        for x, y in zip(a, b):
            px, py = float(x["pcs"]), float(y["pcs"])
            if px == 0.0 and py == 0.0:
                continue  # no score on this cycle (hadFix 0): nothing to weigh
            scored += 1
            pred = 0.5 / max(float(x["gdop"]), 1.0)
            worst = max(worst, abs((px - py) - pred))
            if pred > 10 * TOL_W0:
                big += 1
        ok = nd == 0 and scored > 0 and big > 0 and worst <= TOL_W0
        check("W0", ok,
              f"--pcsW2=0: {nd} rows differ outside pcs (need 0); on {scored} scored rows "
              f"pcs(w=0.5) - pcs(w=0) - 0.5/max(gdop,1) is at most {worst:.2e} "
              f"(need <= {TOL_W0:.1e}; {big} rows where the term exceeds 10x that)")
    safe("W0", c_w0)

    def c_tiers():
        leg = summary(d, "ev_leg")
        u868 = summary(d, "ev_up868")
        u2p4 = summary(d, "ev_up2p4")
        el, eu = int(leg["escalations"]), int(u868["escalations"])
        check("T868", el > 0 and eu == 0,
              f"--pcsThrUp868=-1 alone: escalations {eu} (need 0; {el} without it, need > 0)")
        cl, cu, e2 = int(leg["cycles_uwb"]), int(u2p4["cycles_uwb"]), int(u2p4["escalations"])
        check("T2P4", cl > 0 and cu == 0 and e2 > 0,
              f"--pcsThrUp2p4=-1 alone: cycles_uwb {cu} (need 0; {cl} without it, need > 0) "
              f"while escalations to 2.4 GHz continue: {e2} (need > 0)")
        same = read(os.path.join(d, "ev_leg", "arm_summary.csv")) == \
            read(os.path.join(d, "ev_dn2p4", "arm_summary.csv"))
        check("TD2P4", not same,
              "--pcsThrDown2p4=10 alone: arm_summary " + ("IDENTICAL -- it reached nothing"
                                                          if same else "differs"))
    safe("T*", c_tiers)

    def c_slots():
        exp = {
            "slots": ([0.11, 0.12, 0.13], [0.24, 0.25], [0.31, 0.32, 0.33]),
            # v2: 868 and UWB up are v2's derived 0.0; both downs its log10(2)
            "slots_v2": ([0.0, 0.1, 0.0], [0.301029995663981, 0.301029995663981], []),
            # band 0.3 on legacy: down 2p4 = 1.70 + 0.3; UWB's explicit 3 wins over 2.60
            "slots_band": ([1.55, 1.70, 2.30], [2.0, 3.0], []),
        }
        for n, (u, dn, w) in exp.items():
            got = note(d, n)
            if got is None:
                check("SLOTS", False, f"{n}: no single NOTE line of what was resolved")
                continue
            ok = (len(got[0]) == 3 and len(got[1]) == 2 and len(got[2]) == len(w) and
                  all(abs(a - b) < 1e-12 for a, b in zip(got[0] + got[1] + got[2], u + dn + w)))
            check("SLOTS", ok, f"{n}: resolved up {got[0]} down {got[1]} weights {got[2]}; "
                               f"expected up {u} down {dn} weights {w}")
    safe("SLOTS", c_slots)

    def c_refusals():
        cases = [
            ("r_missing_thr", 1, "FATAL: --pcsForm=calibrated needs"),
            ("r_missing_w", 1, "FATAL: --pcsForm=calibrated needs"),
            ("r_inv_explicit", 1, "(as resolved)"),
            ("r_inv_resolved", 1, "(as resolved)"),
            ("r_down868", None, "pcsThrDown868"),
        ]
        for n, want, text in cases:
            got = rc(d, n)
            ok = (got != 0 if want is None else got == want) and log_has(d, n, text)
            check("R", ok, f"{n}: exit {got}, reason {'present' if log_has(d, n, text) else 'ABSENT'}"
                           f" ('{text}')")
        check("R", rc(d, "r_up868_ok") == 0,
              f"r_up868_ok: --pcsThrUp868=2.0 above the inert 1.75 accepted: exit {rc(d, 'r_up868_ok')}")
    safe("R", c_refusals)

    bad = 0
    missing = 0
    for name, ok, why in results:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G28 {tag:7s} {name:6s} {why}")
        bad += ok is False
        missing += ok is None
    print(f"  G28: {len(results) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
