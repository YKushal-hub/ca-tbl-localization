#!/usr/bin/env python3
"""
verify_causal_sim.py -- invariant checks for the Phase S1/S2 causal simulation.

This is not a results script. It is the thing that has to pass before any number
out of ca_tbl_causal_sim is worth reading, because the failure mode being fixed
in S1 (a tier decision that does not actually gate the radio) is invisible in the
summary table -- the RMSE and the joules both look perfectly reasonable while
being meaningless.

Each check below corresponds to a way the old pipeline was silently wrong.

    python3 scratch/python_handoff/verify_causal_sim.py results/causal
"""

import sys
import os
import glob
import csv

FAIL = []
WARN = []
WARN_OK = []
PASSED = []
# Documented findings: a check whose failure is the measured result rather than a
# defect. Separate from WARN so that "expected, understood, reported" cannot be
# confused with "unexplained", and separate from FAIL so a permanent known failure
# does not train the reader to ignore the gate -- which is how the pre-S2.5 results
# survived review.
NOTE = []


def check(name, condition, detail=""):
    if condition:
        PASSED.append(name)
    else:
        FAIL.append(f"{name}: {detail}")


def warn(name, condition, detail=""):
    # WARN_OK exists because this function used to record nothing on success, which
    # made the reported gate total a non-conserved quantity: a warning that started
    # passing simply vanished. That is how "77 gates two sessions ago, 76 now" was
    # read as a check having been deleted, when in fact all 72 check() gates were
    # still present and one WARNING had merely started passing -- specifically
    # "ca_tbl_full cheaper than always-on UWB", which flipped when the AccountListen()
    # search charge raised static_uwb from 0.7618 J to 630.51 J. A gate that becomes
    # invisible the moment it is satisfied cannot be audited for still being wired up.
    if condition:
        WARN_OK.append(name)
    else:
        WARN.append(f"{name}: {detail}")


def load_csv(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def verify_traces(outdir):
    traces = sorted(glob.glob(os.path.join(outdir, "trace_*.csv")))
    if not traces:
        FAIL.append(f"no trace files found in {outdir}")
        return

    for path in traces:
        arm = os.path.basename(path)
        rows = load_csv(path)
        if not rows:
            FAIL.append(f"{arm}: empty trace")
            continue

        # 1. Exactly one tier is active per step. The whole point of S1.
        tiers_per_step = [r["activeTier"] for r in rows]
        check(f"{arm}: one active tier per row",
              all(t in ("868MHz", "2.4GHz", "UWB") for t in tiers_per_step),
              "unexpected tier label")

        # 2. Energy is monotonically non-decreasing. A drop means an account was
        #    reset mid-run or a negative duration was charged.
        cum = [float(r["cumJ"]) for r in rows]
        monotonic = all(b >= a - 1e-12 for a, b in zip(cum, cum[1:]))
        check(f"{arm}: cumulative energy monotonic", monotonic,
              "energy decreased between steps")

        # 3. nextTier at step k must equal activeTier at step k+1. If this fails,
        #    the decision is not actually feeding forward into the radio -- which
        #    is exactly the bug S1 exists to fix, reappearing.
        mismatches = 0
        for a, b in zip(rows, rows[1:]):
            if a["nextTier"] != b["activeTier"]:
                mismatches += 1
        check(f"{arm}: decision feeds forward into next cycle", mismatches == 0,
              f"{mismatches} steps where nextTier != following activeTier")

        # 4. Static arms must never change tier.
        if "static_" in arm:
            distinct = set(tiers_per_step)
            check(f"{arm}: static arm never switches tier", len(distinct) == 1,
                  f"observed tiers {sorted(distinct)}")
            esc = sum(int(r["escalated"]) for r in rows)
            check(f"{arm}: static arm records zero escalations", esc == 0,
                  f"{esc} escalations recorded")

        # 5. Integrity flag only fires when the policy could have evaluated it.
        #    A suspect flag on a step with no fix means the gate ran on nothing.
        bad_suspect = sum(1 for r in rows if int(r["suspect"]) == 1 and int(r["hadFix"]) == 0)
        check(f"{arm}: no integrity flag without a fix", bad_suspect == 0,
              f"{bad_suspect} suspect flags on no-fix steps")

        # 6. A rejected measurement must coincide with a suspect flag.
        bad_reject = sum(1 for r in rows if int(r["rejected"]) == 1 and int(r["suspect"]) == 0)
        check(f"{arm}: rejection implies suspect", bad_reject == 0,
              f"{bad_reject} rejections without a suspect flag")

        # 7. Sanity on the estimate itself.
        nonfinite = sum(1 for r in rows
                        if not all(_finite(r[k]) for k in ("estX", "estY", "errM", "pcs")))
        check(f"{arm}: no NaN/inf in estimate or PCS", nonfinite == 0,
              f"{nonfinite} rows with non-finite values")

        # 8. Fix coverage. Pre-S2.5 this was a soft warning at 20%, and five of
        #    six arms sat at 2-17% -- which is precisely how a set of results
        #    consisting almost entirely of dead-reckoning drift got as far as
        #    being written into a report. The bar is now 50% and it is checked
        #    per arm, because an arm below it is not producing a localisation
        #    result at all and its RMSE must not be quoted.
        #
        #    static_uwb is reported rather than failed. It is a single-band baseline
        #    pinned to a tier whose measured r80 is 13.5 m, sited only at four facility
        #    clusters inside a +/-1000 m box. Low fix coverage IS the result for that
        #    arm -- single-band UWB is not viable at this deployment scale -- and a
        #    permanent known FAIL would teach the reader to skip this gate. The
        #    exemption prints the number so the constraint stays visible, and its
        #    unconditional rmseM still must never be quoted as a localisation figure.
        fixes = sum(int(r["hadFix"]) for r in rows)
        frac = fixes / len(rows)
        if "static_uwb" in arm:
            NOTE.append(f"{arm}: fix coverage {fixes}/{len(rows)} ({frac:.1%}) -- expected, "
                        f"not a defect: single-band UWB at r80=13.5 m cannot cover this "
                        f"deployment. Quote rmseFixedM for this arm, never rmseM")

            # Detector for a dead UWB tier. This REPLACES the old check that required
            # static_uwb to have the lowest RMSE of any arm.
            #
            # That check existed to catch the pre-S2.5 bug where static_uwb completed
            # zero ranging exchanges in 600 cycles and was therefore never charged for
            # one, which inverted the energy axis. It was never a claim about filtered
            # position accuracy under arbitrary coverage. It cannot serve its purpose
            # any more: with r80 = 13.5 m the arm coasts 91% of the time, so its RMSE
            # -- even conditioned on having a fix -- mixes steady-state ranging with
            # post-outage reconvergence in a ratio set by COVERAGE, not by ranging
            # quality. static_2p4 never has an outage and so never carries that term.
            #
            # This version isolates the quantity the detector was always about: when
            # UWB has been in coverage for at least two consecutive cycles, the position
            # error must be consistent with UWB's measured ranging sigma. The probe puts
            # that at ~0.10-0.14 m and the observed sustained median is 0.142 m, so the
            # 1.0 m bar is roughly 7x headroom -- while a tier that is not actually
            # ranging lands in the tens of metres. That is a TIGHTER detector than the
            # one it replaces, not a weaker one.
            sustained = [float(r["errM"]) for i, r in enumerate(rows)
                         if int(r["hadFix"]) == 1 and i > 0 and int(rows[i - 1]["hadFix"]) == 1]
            if not sustained:
                FAIL.append(f"{arm}: no two consecutive cycles with a UWB fix -- the tier "
                            f"never holds coverage, so its ranging cannot be assessed")
            else:
                s = sorted(sustained)
                med = s[len(s) // 2]
                check(f"{arm}: UWB sustained-coverage median error matches its ranging floor",
                      med < 1.0,
                      f"median {med:.3f} m over {len(sustained)} sustained-coverage fixes "
                      f"against a probe ranging sigma of ~0.14 m -- UWB is not ranging")
                NOTE.append(f"{arm}: sustained-coverage median error {med:.3f} m over "
                            f"{len(sustained)} cycles. This, not rmseM or rmseFixedM, is the "
                            f"figure that characterises UWB ranging precision")
        else:
            check(f"{arm}: fix coverage above 50%", frac > 0.50,
                  f"only {fixes}/{len(rows)} ({frac:.1%}) steps produced a fix -- "
                  f"this arm's RMSE is dead reckoning, not localisation")

        # 9. S2.5 ground-truth coverage columns must be present and consistent.
        #    shouldUwb is defined as visUwb >= 3 in the simulation; if that
        #    identity ever breaks, the trigger confusion matrix is scoring
        #    against the wrong label.
        if "visUwb" not in rows[0]:
            FAIL.append(f"{arm}: missing S2.5 coverage columns "
                        f"(vis868/vis2p4/visUwb/shouldUwb) -- rebuild the simulation")
            continue
        bad_label = sum(1 for r in rows
                        if (int(r["visUwb"]) >= 3) != (int(r["shouldUwb"]) == 1))
        check(f"{arm}: shouldUwb == (visUwb >= 3)", bad_label == 0,
              f"{bad_label} rows where the ground-truth label disagrees with the geometry")

        # 10. The scenario must actually pose the escalation question. If the node
        #     is never inside UWB coverage there is nothing to escalate to and the
        #     policy is untested; if it is always inside, escalating is
        #     unconditionally correct and the policy is equally untested.
        cov = sum(int(r["shouldUwb"]) for r in rows)
        covfrac = cov / len(rows)
        warn(f"{arm}: UWB coverage is a meaningful minority", 0.05 <= covfrac <= 0.45,
             f"{covfrac:.1%} of steps are inside 3-anchor UWB coverage -- "
             f"outside 5-45% the escalation decision is not being exercised")


def verify_summary(outdir):
    path = os.path.join(outdir, "arm_summary.csv")
    if not os.path.exists(path):
        FAIL.append(f"missing {path}")
        return
    rows = load_csv(path)
    if not rows:
        FAIL.append("arm_summary.csv is empty")
        return

    by_seed = {}
    for r in rows:
        by_seed.setdefault(r["seed"], {})[r["arm"]] = r

    for seed, arms in by_seed.items():
        # 11. THE key S1 check. An arm that never escalates must have spent
        #    exactly zero energy on the tiers it never used. Under the old
        #    acausal pipeline this was structurally impossible.
        if "static_868" in arms:
            a = arms["static_868"]
            check(f"seed {seed}: static_868 spends no 2.4GHz TX energy",
                  float(a["airtime_2p4"]) == 0.0,
                  f"airtime_2p4={a['airtime_2p4']}")
            check(f"seed {seed}: static_868 spends no UWB TX energy",
                  float(a["airtime_uwb"]) == 0.0,
                  f"airtime_uwb={a['airtime_uwb']}")
            check(f"seed {seed}: static_868 never occupies tier 2 or 3",
                  int(a["cycles_2p4"]) == 0 and int(a["cycles_uwb"]) == 0,
                  f"cycles_2p4={a['cycles_2p4']} cycles_uwb={a['cycles_uwb']}")

        # 12. The energy ordering the architecture assumes. If UWB is not the
        #     most expensive tier, the power profiles are wrong and every
        #     downstream conclusion inverts.
        #
        #     Pre-S2.5 this check FAILED and the failure was the tell: static_uwb
        #     came out at 0.68 J against static_868's 5.07 J. Not because UWB is
        #     cheap, but because it never completed a single ranging exchange in
        #     600 cycles, so it was never charged for one. If this check fails
        #     again, look at noFixCycles before touching the power profiles.
        if "static_uwb" in arms and "static_868" in arms:
            check(f"seed {seed}: always-on UWB costs more than 868-only",
                  float(arms["static_uwb"]["totalJ"]) > float(arms["static_868"]["totalJ"]),
                  f"UWB arm cost {arms['static_uwb']['totalJ']} J vs 868's "
                  f"{arms['static_868']['totalJ']} J. Check noFixCycles="
                  f"{arms['static_uwb']['noFixCycles']} first: a radio that never ranges "
                  f"is never charged, which inverts the axis")

            # 13. UWB is the accuracy ceiling -- but only WHERE IT HAS COVERAGE.
            #
            #     This check used to compare unconditional rmseM, and that encoded an
            #     assumption that has since been falsified by measurement. It was
            #     written when UWB was assumed to reach ~100 m. The decorrelated probe
            #     puts r80 at 13.5 m, and UWB anchors exist only at four facility
            #     sites inside a +/-1000 m box, so static_uwb fixes on ~12% of cycles
            #     and coasts the rest. Its unconditional RMSE is therefore a
            #     dead-reckoning statistic and says nothing about UWB ranging
            #     precision, which is what this check exists to protect.
            #
            #     Conditioning on "had a fix" was not sufficient either: it removes the
            #     coverage confound and leaves the reconvergence one. Post-outage cycles
            #     carry a drifted prior, and only an arm WITH outages pays that term, so
            #     rmseFixedM still cannot be compared across arms with different coverage.
            #
            #     The pass/fail detector for UWB ranging now lives in verify_traces(),
            #     on the sustained-coverage median. What remains here is the comparison
            #     itself, REPORTED rather than enforced -- because it is a real and
            #     reportable finding that a single SX1280 radio delivers a better
            #     filtered position than a single DW3000 radio at this deployment scale,
            #     despite ranging 20x less precisely. That is an architectural result
            #     about coverage beating precision, and burying it in a passing check
            #     would lose it.
            if "rmseFixedM" in arms["static_uwb"]:
                uwb_rmse_fix = float(arms["static_uwb"]["rmseFixedM"])
                others_fix = {n: float(a["rmseFixedM"]) for n, a in arms.items()
                              if n != "static_uwb" and float(a["rmseFixedM"]) > 0.0}
                if others_fix:
                    best = min(others_fix, key=others_fix.get)
                    if others_fix[best] < uwb_rmse_fix:
                        NOTE.append(
                            f"seed {seed}: {best} achieves a better fix-conditional RMSE "
                            f"({others_fix[best]:.2f} m) than always-on UWB "
                            f"({uwb_rmse_fix:.2f} m), despite ranging far less precisely. "
                            f"UWB's sustained-coverage error is at its ranging floor; the "
                            f"gap is post-outage reconvergence, which only an arm with "
                            f"coverage gaps pays. Coverage is beating precision -- report "
                            f"this, it is the finding")

        # 14. Trigger confusion matrix must be complete and internally consistent
        #     (S2.5). Every cycle lands in exactly one of the four cells.
        for name, a in arms.items():
            if "trig_tp" not in a:
                FAIL.append(f"seed {seed}/{name}: missing trigger columns -- "
                            f"rebuild the simulation")
                break
            cells = sum(int(a[k]) for k in ("trig_tp", "trig_fp", "trig_fn", "trig_tn"))
            check(f"seed {seed}/{name}: confusion matrix covers every cycle",
                  cells == int(a["steps"]),
                  f"tp+fp+fn+tn={cells} but steps={a['steps']}")
            check(f"seed {seed}/{name}: tp+fn equals cycles in UWB coverage",
                  int(a["trig_tp"]) + int(a["trig_fn"]) == int(a["cyclesInUwbCoverage"]),
                  "positives do not sum to the ground-truth positive class")

        # 15. Static arms bound the trigger metric by construction, and that is
        #     what makes the policy arms' numbers interpretable. static_uwb is
        #     always on UWB so it cannot miss a positive (recall 1.0) and pays for
        #     it in precision; static_868 is never on UWB so it scores zero.
        #     If these do not hold, the labels are wired wrong.
        if "static_uwb" in arms:
            check(f"seed {seed}: static_uwb has perfect trigger recall",
                  int(arms["static_uwb"]["trig_fn"]) == 0,
                  f"trig_fn={arms['static_uwb']['trig_fn']} for an always-on-UWB arm")
        if "static_868" in arms:
            check(f"seed {seed}: static_868 never claims a UWB positive",
                  int(arms["static_868"]["trig_tp"]) == 0
                  and int(arms["static_868"]["trig_fp"]) == 0,
                  "an arm pinned to 868 recorded UWB occupancy")

        # 16. Duty-cycle compliance: WPC G.S.R. 853(E) Table-II allows 2.5% for
        #     non-access-point devices. Exceeding it is a regulatory violation,
        #     not a tuning preference, and must be caught here rather than in review.
        for name, a in arms.items():
            duty = float(a["duty_868"])
            warn(f"seed {seed}/{name}: 868 duty cycle within WPC 2.5%", duty <= 0.025,
                 f"duty={duty:.4%} exceeds the Table-II field-node budget")

        # 17. Sanity: the proposed arm should sit strictly between the two
        #     extremes on both axes. If it does not, say so -- do not tune until
        #     it does.
        if all(k in arms for k in ("ca_tbl_full", "static_868", "static_uwb")):
            full = arms["ca_tbl_full"]
            warn(f"seed {seed}: ca_tbl_full beats 868-only on accuracy",
                 float(full["rmseM"]) < float(arms["static_868"]["rmseM"]),
                 "the hybrid is no more accurate than the cheapest single band")
            warn(f"seed {seed}: ca_tbl_full cheaper than always-on UWB",
                 float(full["totalJ"]) < float(arms["static_uwb"]["totalJ"]),
                 "the hybrid is not saving energy against the always-on baseline")

        if "ca_tbl_full" in arms and "fixed_threshold" in arms:
            warn(f"seed {seed}: ca_tbl_full beats fixed_threshold on accuracy/joule",
                 float(arms["ca_tbl_full"]["accuracyPerJoule"]) >
                 float(arms["fixed_threshold"]["accuracyPerJoule"]),
                 "the policy is not earning its complexity against the naive baseline")


def _finite(s):
    try:
        v = float(s)
        return v == v and abs(v) != float("inf")
    except (TypeError, ValueError):
        return False


def main():
    outdir = sys.argv[1] if len(sys.argv) > 1 else "results/causal"
    verify_traces(outdir)
    verify_summary(outdir)

    total = len(PASSED) + len(WARN_OK) + len(NOTE) + len(WARN) + len(FAIL)
    print(f"\n{len(PASSED)} checks passed")
    print(f"{len(WARN_OK)}/{len(WARN_OK) + len(WARN)} warning gates clear")
    print(f"{total} gates evaluated "
          f"({len(PASSED)} check + {len(WARN_OK) + len(WARN)} warn + {len(NOTE)} note)")

    if NOTE:
        print(f"\n{len(NOTE)} DOCUMENTED FINDINGS (a gate whose failure is the measured "
              f"result -- these belong in the report as results):")
        for n in NOTE:
            print(f"  = {n}")

    if WARN:
        print(f"\n{len(WARN)} WARNINGS (these are findings, not defects -- "
              f"report them, do not silently tune them away):")
        for w in WARN:
            print(f"  ! {w}")

    if FAIL:
        print(f"\n{len(FAIL)} FAILURES (the simulation is not trustworthy until these clear):")
        for f in FAIL:
            print(f"  X {f}")
        sys.exit(1)

    print("\nAll invariants hold. The tier decision is causal.")
    sys.exit(0)


if __name__ == "__main__":
    main()
