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
ARMS_AUDITED = 0
ARMS_IN_SUMMARY = 0
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


def verify_coverage(outdir):
    """Does the evidence on disk cover what arm_summary.csv says was run?

    Two holes this closes, both found by audit rather than by a gate.

    D1: nothing asserted WHICH arms should be present. Deleting 2 of 6 trace files
    took the report from 72 checks / 90 gates to 54 / 68 and still printed "All
    invariants hold" with exit 0. A batch that silently produced four arms passed
    cleanly, and the gate count -- the only signal -- was compared against nothing.
    The expected set is derived from arm_summary.csv rather than hardcoded to six,
    because arms are still being added (fixed_threshold_gated, the E.2 matrix) and
    a `== 6` assertion would break on the first of those and get deleted.

    D2: nothing related trace mtimes to the summary. The v3 section 3.10 incident --
    the verifier reading a previous session's traces alongside a fresh summary --
    was caught by an internal contradiction, not by a gate, and stayed ungated.

    LIMITATION, stated deliberately: mtime survives a copy, so this catches the
    accident that actually happened (a stale file left in place by a --trace=false
    run) and not a deliberate reconstruction.
    """
    spath = os.path.join(outdir, "arm_summary.csv")
    traces = sorted(glob.glob(os.path.join(outdir, "trace_*.csv")))
    if not os.path.exists(spath) or not traces:
        return  # verify_summary / verify_traces already FAIL on these

    with open(spath, newline="") as f:
        expected = {(r["arm"], r["seed"]) for r in csv.DictReader(f)}

    present = set()
    for p in traces:
        base = os.path.basename(p)[len("trace_"):-len(".csv")]
        arm, _, seed = base.rpartition("_seed")
        if arm:
            present.add((arm, seed))

    missing = expected - present
    check("evidence coverage: every arm/seed in arm_summary has a trace",
          not missing,
          f"{len(missing)} of {len(expected)} missing: {sorted(missing)[:6]}")

    # Freshness must compare the summary against the NEWEST trace, not against each
    # trace. Arms run sequentially, so within one legitimate run the traces span the
    # whole run (measured: 283 s spread) while the summary is written 0.128 s after
    # the last one. A per-trace comparison therefore flags every healthy run.
    #
    # The failure this catches is the v3 section 3.10 incident: a later --trace=false
    # pass rewrites arm_summary.csv into a directory whose traces are from an earlier
    # run, leaving the summary many minutes newer than the newest trace. Any full run
    # takes minutes, so 60 s separates the two cases cleanly.
    #
    # LIMITATION, stated deliberately: mtime survives a copy, so this catches the
    # accident that actually happened and not a deliberate reconstruction. It also
    # cannot detect a partial regeneration in which only some traces were rewritten.
    smt = os.path.getmtime(spath)
    mts = [os.path.getmtime(p) for p in traces]
    newest, oldest = max(mts), min(mts)
    lag = smt - newest

    # Mode 1 (the incident that happened): a later --trace=false run rewrites the
    # summary into a directory whose traces are from an earlier run.
    check("evidence freshness: arm_summary is not newer than the traces it describes",
          lag <= 60.0,
          f"arm_summary.csv is {lag:.0f} s newer than the newest trace -- a later "
          f"--trace=false run almost certainly rewrote it over stale traces")

    # Mode 2: the reverse. A run wrote traces and died before writing the summary,
    # leaving a stale summary to be read against fresh traces. Within a healthy run
    # the summary is written LAST, so it is never meaningfully older.
    check("evidence freshness: arm_summary is not older than the traces it describes",
          lag >= -60.0,
          f"arm_summary.csv is {-lag:.0f} s OLDER than the newest trace -- the run "
          f"that produced these traces did not write a summary")

    # Mode 3: traces mixed from two runs, e.g. a batch resumed after a crash that
    # regenerated some traces and not others. A legitimate run writes all arms
    # sequentially -- measured spread 283 s for six arms -- so anything past a
    # couple of hours is two runs, not one.
    spread = newest - oldest
    check("evidence consistency: all traces come from a single run",
          spread <= 7200.0,
          f"traces span {spread / 3600.0:.1f} h (measured single-run spread is ~283 s) "
          f"-- this directory mixes output from more than one run")

    # Run-id sidecar. Unlike mtime, a run id survives cp and touch, so it closes the
    # staleness class rather than the two modes the mtime checks cover. Absent is a
    # WARNING not a FAILURE, so results produced before the sidecar existed still
    # audit -- a gate that refuses to run on historical data gets bypassed.
    ridp = os.path.join(outdir, "run_id.txt")
    warn("provenance: run_id.txt sidecar present",
         os.path.exists(ridp),
         "no run_id.txt -- produced before the sidecar existed, or by a different tool")
    if os.path.exists(ridp):
        with open(ridp) as f:
            rid = f.readline().strip()
        check("provenance: run id is well formed", len(rid) == 16 and all(
                  c in "0123456789abcdef" for c in rid),
              f"run_id.txt first line is {rid!r}, expected 16 hex digits")

    # Source provenance. WARN not FAIL, deliberately: results produced from a dirty
    # tree are not necessarily wrong -- remediation item 7 was probe-only and its
    # arm results were sound -- but they cannot be regenerated from any commit, and
    # that is worth surfacing every run rather than discovering by audit.
    # Absent sidecar is also a warning, so pre-existing result directories still audit.
    rpath = os.path.join(outdir, "run_id.txt")
    if not os.path.exists(rpath):
        warn("provenance: run_id.txt present", False,
             "no run_id sidecar -- directory predates provenance recording")
    else:
        with open(rpath) as f:
            txt = f.read()
        src = [ln for ln in txt.splitlines() if ln.startswith("source ")]
        if not src:
            warn("provenance: run_id records the source commit", False,
                 "run_id has no 'source <sha> dirty <n>' line -- built before "
                 "provenance recording, so a working-tree build is indistinguishable "
                 "from a committed one")
        else:
            parts = src[0].split()
            n = parts[3] if len(parts) > 3 else "?"
            warn("provenance: built from a clean tree", n == "0",
                 f"built with {n} uncommitted file(s) at {parts[1] if len(parts) > 1 else '?'}"
                 f" -- these results cannot be regenerated from a commit")

    global ARMS_AUDITED, ARMS_IN_SUMMARY
    ARMS_IN_SUMMARY = len({a for a, _ in expected})
    ARMS_AUDITED = len({a for a, _ in present & expected})


def verify_harness_tracked():
    """Is every file of the validation harness actually under version control?

    scratch/.gitignore ignores `/*`, which matched this directory. Git does not
    descend into an ignored directory, so every new file here was silently
    untracked -- verify_causal_sim.py itself had to be added with `git add -f`,
    and the next validation script dropped in beside it would simply not have
    existed as far as the repository was concerned.

    A validation harness that can quietly fail to exist is a worse failure mode
    than any it checks for: the gates would appear to pass on a checkout that
    never contained them. Hence a gate on the gates.

    Not being in a git checkout is a WARNING, not a failure, so the verifier
    still runs against an exported results directory.
    """
    import subprocess

    here = os.path.dirname(os.path.abspath(__file__))
    try:
        untracked = subprocess.run(
            ["git", "ls-files", "--others", "--exclude-standard", "--", here],
            cwd=here, capture_output=True, text=True, timeout=30)
        ignored = subprocess.run(
            ["git", "ls-files", "--others", "--ignored", "--exclude-standard", "--", here],
            cwd=here, capture_output=True, text=True, timeout=30)
    except (OSError, subprocess.SubprocessError):
        warn("harness: version-control status checkable", False,
             "git not available -- cannot confirm the validation harness is tracked")
        return

    if untracked.returncode != 0 or ignored.returncode != 0:
        warn("harness: version-control status checkable", False,
             "not inside a git checkout -- cannot confirm the harness is tracked")
        return

    stray = [p for p in untracked.stdout.split() if p.endswith(".py")]
    check("harness: no untracked files in scratch/python_handoff",
          not stray,
          f"{len(stray)} untracked: {stray[:5]} -- these exist on this machine and "
          f"nowhere else; commit them or the gates they implement are not in the repo")

    hidden = [p for p in ignored.stdout.split() if p.endswith(".py")]
    check("harness: no IGNORED files in scratch/python_handoff",
          not hidden,
          f"{len(hidden)} ignored by .gitignore: {hidden[:5]} -- scratch/.gitignore "
          f"has regressed to swallowing the validation harness")


def read_pcs_form(outdir):
    """Which --pcsForm produced this directory, from the run_id.txt invocation.

    Read from provenance rather than guessed from the data: gating the legacy
    form's "sigmaGeoM must be absent" check on a value inferred FROM sigmaGeoM
    would make the check vacuous.
    """
    p = os.path.join(outdir, "run_id.txt")
    if not os.path.exists(p):
        return "legacy"
    with open(p) as f:
        txt = f.read()
    return "v2" if "--pcsForm=v2" in txt else "legacy"


def read_nlos_source(outdir):
    """Which --nlosSource produced this directory, from the run_id sidecar.

    The mode is NOT a column in arm_summary.csv and must not become one: that file
    is byte-compared against results/baseline_v3 as the acceptance test for every
    flag in the project, so adding a column would break the very gate the NLOS
    work is being held to. run_id.txt line 2 is the full invocation, which already
    carries the flag, and it is written by the run itself -- so this reads
    provenance rather than inferring the mode from the data it is about to check.

    Absent sidecar means legacy: results produced before the flag existed are
    legacy by definition.
    """
    ridp = os.path.join(outdir, "run_id.txt")
    if not os.path.exists(ridp):
        return "legacy"
    with open(ridp) as f:
        lines = f.read().splitlines()
    inv = lines[1] if len(lines) > 1 else ""
    for tok in inv.split():
        if tok.startswith("--nlosSource="):
            return tok.split("=", 1)[1]
    return "legacy"


def verify_pcs_columns(arm, rows, pcs_form):
    """PCS v2's inputs must be present and physically sane.

    sigmaGeoM is the exact sqrt(trace((J^T R^-1 J)^-1)) -- the measurement-only
    radial position sigma that PCS v2 uses in place of the legacy 1/max(gdop,1)
    term. It is emitted so the pre-registered per-term variance decomposition
    (report/PCS_REDESIGN_RESULTS.md §0.4a) can be computed from the trace alone,
    rather than by re-deriving the weighted solve in a second implementation that
    could drift from the first.

    Negative means "not computed": the legacy form never solves for it, and the v2
    form leaves it negative when the weighted solve fails on degenerate geometry.
    Those are different conditions, so they are gated differently.
    """
    if "sigmaGeoM" not in rows[0]:
        FAIL.append(f"{arm}: missing sigmaGeoM column -- rebuild the simulation")
        return

    fix = [r for r in rows if int(r["hadFix"]) == 1]
    if not fix:
        return
    vals = [float(r["sigmaGeoM"]) for r in fix]

    if pcs_form == "legacy":
        # The legacy path must not have gained a weighted solve it never had, or
        # baseline reproduction becomes an argument rather than a diff.
        check(f"{arm}: legacy PCS form does not compute sigmaGeoM",
              all(v < 0.0 for v in vals),
              f"{sum(1 for v in vals if v >= 0.0)} fix cycles carry a sigmaGeoM "
              f"under --pcsForm=legacy -- the legacy path has gained a solve")
        return

    solved = [v for v in vals if v > 0.0]
    frac = len(solved) / len(vals)
    # A degenerate solve is legitimate but must be rare; if most cycles fail it,
    # PCS v2 is running on its maximally-unconfident fallback and the score is not
    # being tested at all.
    check(f"{arm}: PCS v2 has a valid weighted solve on most fix cycles",
          frac >= 0.90,
          f"only {frac:.1%} of fix cycles produced a positive sigmaGeoM -- v2 is "
          f"mostly running on its degenerate-geometry fallback, so the score under "
          f"test is the fallback, not the form")
    if solved:
        check(f"{arm}: sigmaGeoM is finite and physically bounded",
              all(v == v and v < 1e5 for v in solved),
              "a measurement-only radial sigma exceeded 100 km or was NaN")


def verify_nlos_columns(arm, rows):
    """The NLOS populations must be internally consistent and correctly keyed.

    Gates the defect fixed in this change: nNlos used to be
    (nlosDb > 0 || hiddenChannelState > 0), and hiddenChannelState is a 4-state
    Markov FADING REGIME uniform on [0, 3.99), so '> 0' tagged ~75% of links as
    obstructed. nNlosPropagation and nHiddenChannel are now emitted separately so
    that the composition of the operative flag is checkable rather than arguable.
    """
    if "nNlosPropagation" not in rows[0] or "nHiddenChannel" not in rows[0]:
        FAIL.append(f"{arm}: missing NLOS population columns "
                    f"(nNlosPropagation/nHiddenChannel) -- rebuild the simulation")
        return

    fix = [r for r in rows if int(r["hadFix"]) == 1]
    if not fix:
        return

    # Each population is a subset of the links actually fused, so none can exceed
    # nAnchors. Catches a count taken over the wrong measurement set -- e.g. over
    # the polled set rather than the set that survived to the EKF.
    bad = sum(1 for r in fix
              if not (int(r["nNlos"]) <= int(r["nAnchors"])
                      and int(r["nNlosPropagation"]) <= int(r["nAnchors"])
                      and int(r["nHiddenChannel"]) <= int(r["nAnchors"])))
    check(f"{arm}: NLOS counts never exceed the fused link count", bad == 0,
          f"{bad} rows where a population count exceeds nAnchors")

    # Anti-regression, and it applies in BOTH modes: the hidden-channel population
    # must still be observed and recorded. The cheap wrong way to make the NLOS
    # numbers look right is to stop counting the Markov regime rather than to stop
    # keying the R matrix to it -- that would silence the evidence while leaving
    # the defect in place. This gate makes the population's disappearance a
    # failure. It is deliberately mode-independent so the two modes carry the same
    # number of gates and a mode change cannot quietly reduce coverage.
    hidden_total = sum(int(r["nHiddenChannel"]) for r in fix)
    check(f"{arm}: hidden-channel population is still recorded", hidden_total > 0,
          "nHiddenChannel is zero on every fix cycle -- the Markov regime is no "
          "longer being observed, so the size of the legacy defect can no longer "
          "be measured from this trace")

    if read_nlos_source_cache["mode"] == "legacy":
        # The legacy flag is an OR of the two populations, so per row it is bounded
        # below by the larger of them and above by their sum (capped at nAnchors).
        # Catches a flag keyed to neither population, which is what a partially
        # applied re-keying would look like.
        under = sum(1 for r in fix
                    if int(r["nNlos"]) < max(int(r["nNlosPropagation"]),
                                             int(r["nHiddenChannel"])))
        check(f"{arm}: legacy NLOS flag is at least the larger population",
              under == 0,
              f"{under} rows where nNlos < max(nNlosPropagation, nHiddenChannel) -- "
              f"an OR cannot be smaller than either of its operands")
        over = sum(1 for r in fix
                   if int(r["nNlos"]) > min(int(r["nAnchors"]),
                                            int(r["nNlosPropagation"])
                                            + int(r["nHiddenChannel"])))
        check(f"{arm}: legacy NLOS flag never exceeds the union bound", over == 0,
              f"{over} rows where nNlos exceeds nNlosPropagation + nHiddenChannel")
    else:
        # Under --nlosSource=propagation the operative flag IS the propagation
        # population, exactly. This is the gate that the flag was actually re-keyed
        # rather than merely documented as re-keyed.
        mism = sum(1 for r in fix
                   if int(r["nNlos"]) != int(r["nNlosPropagation"]))
        check(f"{arm}: --nlosSource=propagation keys the flag to the propagation "
              f"population", mism == 0,
              f"{mism} rows where nNlos != nNlosPropagation despite "
              f"--nlosSource=propagation -- the flag was not re-keyed")

        # The re-keying must actually CHANGE something. If the operative flag still
        # matched the hidden-channel population on every row, --nlosSource would be
        # an inert flag that merely renamed the defect -- which is precisely the
        # failure mode a flag-gated "fix" invites.
        same_as_hidden = all(int(r["nNlos"]) == int(r["nHiddenChannel"]) for r in fix)
        check(f"{arm}: propagation flag is distinguishable from the Markov regime",
              not same_as_hidden,
              "nNlos equals nHiddenChannel on every fix cycle even under "
              "--nlosSource=propagation -- the flag was renamed, not re-keyed")

    links = sum(int(r["nAnchors"]) for r in fix)
    if links:
        NOTE.append(
            f"{arm}: NLOS populations over {links} fused links -- "
            f"operative {sum(int(r['nNlos']) for r in fix) / links:.1%}, "
            f"propagation {sum(int(r['nNlosPropagation']) for r in fix) / links:.1%}, "
            f"hiddenChannel {sum(int(r['nHiddenChannel']) for r in fix) / links:.1%}; "
            f"all-LOS on {sum(1 for r in fix if int(r['nNlos']) == 0)}/{len(fix)} "
            f"fix cycles ({sum(1 for r in fix if int(r['nNlos']) == 0) / len(fix):.2%}). "
            f"The all-LOS rate is the integrity gate's eligible population")


read_nlos_source_cache = {"mode": "legacy"}


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

        # 9b. NLOS populations (the --nlosSource chain).
        verify_nlos_columns(arm, rows)

        # 9c. PCS v2 inputs (the --pcsForm chain).
        verify_pcs_columns(arm, rows, read_nlos_source_cache.get("pcsForm", "legacy"))

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
        #
        # PROMOTED FROM warn() TO check(). It was a warning while the governor was
        # only partially consulted, so a breach was ambiguous between "the scenario
        # is pushing the budget" and "a call site does not ask". That ambiguity is
        # now gone: every transmitting call site consults CanTransmit(), so a breach
        # can only mean a NEW call site was added without asking. The gate is not
        # deleted or weakened -- it is made decisive, which is what the change to
        # the code underneath it now supports.
        #
        # This will FAIL on results produced before the escalation handshake was
        # gated (results/nlos_propagation reaches 0.02511). That is correct and
        # deliberate: those runs violated the airtime budget, and a gate that passes
        # on them would be asserting something false.
        for name, a in arms.items():
            duty = float(a["duty_868"])
            check(f"seed {seed}/{name}: 868 duty cycle within WPC 2.5%", duty <= 0.025,
                  f"duty={duty:.4%} exceeds the Table-II field-node budget -- some "
                  f"transmitting call site is not consulting CanTransmit()")

        # 16b. Escalation-handshake deferral bookkeeping.
        #
        # The cheap way to make the duty-cycle gate above pass is to stop charging
        # the handshake at all. These gates make that visible instead of silent:
        # every deferral is counted, and an escalation rolled back for want of
        # airtime must correspond to a deferral that was recorded.
        for name, a in arms.items():
            if "handshakeDeferrals" not in a:
                continue
            defer = int(a["handshakeDeferrals"])
            wait = float(a["handshakeDeferralDelayS"])
            check(f"seed {seed}/{name}: deferral wait is non-negative and finite",
                  wait >= 0.0 and wait == wait,
                  f"handshakeDeferralDelayS={wait}")
            check(f"seed {seed}/{name}: a deferral count implies a recorded wait",
                  (defer == 0) == (wait == 0.0),
                  f"{defer} deferrals but {wait:.3f} s of recorded wait -- the "
                  f"counter and the latency it is supposed to explain disagree")
            if "escalationsDeferredAway" in a:
                away = int(a["escalationsDeferredAway"])
                check(f"seed {seed}/{name}: rolled-back escalations are a subset of deferrals",
                      away <= defer,
                      f"{away} escalations rolled back against only {defer} deferred "
                      f"handshakes -- bookkeeping does not conserve")
                # Static arms never escalate, so they can never defer one.
                if name.startswith("static_"):
                    check(f"seed {seed}/{name}: a static arm defers no escalation",
                          away == 0 and defer == 0,
                          f"a pinned-tier arm recorded {defer} handshake deferrals")

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
    verify_harness_tracked()
    read_nlos_source_cache["mode"] = read_nlos_source(outdir)
    read_nlos_source_cache["pcsForm"] = read_pcs_form(outdir)
    print(f"nlos_source {read_nlos_source_cache['mode']}  "
          f"pcs_form {read_nlos_source_cache['pcsForm']}  (from run_id.txt provenance)")
    verify_coverage(outdir)
    verify_traces(outdir)
    verify_summary(outdir)

    total = len(PASSED) + len(WARN_OK) + len(NOTE) + len(WARN) + len(FAIL)
    print(f"\n{len(PASSED)} checks passed")
    print(f"{len(WARN_OK)}/{len(WARN_OK) + len(WARN)} warning gates clear")
    print(f"{total} gates evaluated "
          f"({len(PASSED)} check + {len(WARN_OK) + len(WARN)} warn + {len(NOTE)} note)")
    # Printed rather than left implicit: the gate count alone cannot distinguish
    # "all arms checked" from "some arms had no trace and were silently skipped".
    print(f"arms_audited {ARMS_AUDITED}/{ARMS_IN_SUMMARY}")

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
