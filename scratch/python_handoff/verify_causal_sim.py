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
# Absence of run_id.txt is a FAILURE by default, not a warning. It used to be a
# warning whose detail string supplied its own excuse ("produced before the sidecar
# existed"), so the silent-write-failure path in the simulator -- which wrote no
# sidecar and said nothing -- was covered by the one gate written to accept absence.
# A check whose failure mode is silence must assert its own precondition.
REQUIRE_PROVENANCE = True
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
    gate = check if REQUIRE_PROVENANCE else warn
    gate("provenance: run_id.txt sidecar present",
         os.path.exists(ridp),
         "no run_id.txt. Either the run never wrote one -- which the simulator did "
         "silently until the ofstream checks landed -- or this directory predates the "
         "sidecar. Pass --no-require-provenance to audit a pre-sidecar directory.")
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
            # profile decides whether NS_ASSERT and NS_LOG exist, so two runs of one
            # source can differ. Recorded rather than derived from argv[0]'s suffix.
            prof = parts[5] if len(parts) > 5 and parts[4] == "profile" else None
            if prof is None:
                warn("provenance: run_id records the build profile", False,
                     "no 'profile' field -- built before the profile was recorded")
            else:
                check("provenance: build profile is a known value",
                      prof in ("optimized", "release", "debug"),
                      f"profile is {prof!r}; 'unknown' means the build defined no "
                      f"NS3_BUILD_PROFILE_* macro and the binary cannot say what it is")
        verify_output_digest(outdir, txt)

    global ARMS_AUDITED, ARMS_IN_SUMMARY
    ARMS_IN_SUMMARY = len({a for a, _ in expected})
    ARMS_AUDITED = len({a for a, _ in present & expected})


DIGEST_MARKER = "digest sha256"


def _writer_existed_at(commit):
    """Did the digest writer exist in the source this directory was built from?

    This is the pre-digest population problem, and it is the same one the sidecar
    itself had. The boundary must be the SOURCE HASH of the commit that introduced
    the writer, not a date -- dates were shown unreliable here, and a date cannot
    distinguish "old result" from "recent result whose digest was stripped".

    Located dynamically rather than hardcoded: a hardcoded boundary sha cannot be
    written into the commit that creates it, and amending to insert it changes the
    sha again. So ask the recorded commit directly whether its source contains the
    writer.

    FOUR outcomes, not two, because the ways of failing to answer are not the same
    fact and must not be reported as the same reason:

        "yes"          the recorded source contains the writer
        "no"           it resolves and does not -- a genuine pre-digest build
        "unresolved"   git works, but this commit is not in this repository. A
                       history rewrite does exactly this to every recorded sha,
                       and so does a fresh clone of a rewritten repo. Distinct
                       from "no": the question was never answered.
        "nogit"        not a git checkout at all (exported results directory)

    Collapsing "unresolved" into "no" would report a confident, false "predates the
    digest writer" -- the same defect twice corrected in this file's history, once
    for SIGPIPE (exit 141) and once here for a missing object (exit 128).
    """
    import subprocess
    try:
        out = subprocess.run(
            ["git", "cat-file", "-p", f"{commit}:scratch/ca_tbl_causal_sim.cc"],
            capture_output=True, text=True, timeout=30)
    except (OSError, subprocess.SubprocessError):
        return "nogit"
    if out.returncode != 0:
        # 128 is git's "no such object"; anything else non-zero means git itself
        # could not run here. Neither is evidence about the writer.
        try:
            inrepo = subprocess.run(["git", "rev-parse", "--is-inside-work-tree"],
                                    capture_output=True, text=True, timeout=30)
            return "unresolved" if inrepo.returncode == 0 else "nogit"
        except (OSError, subprocess.SubprocessError):
            return "nogit"
    return "yes" if DIGEST_MARKER in out.stdout else "no"


def verify_output_digest(outdir, ridtext):
    """Do the files in this directory match the ones the run says it produced?

    Until this existed the question was unanswerable, not merely unchecked. A
    `git reset --hard` rewrote results/causal/run_id.txt with content describing an
    Aug 5 build while the traces beside it came from an Aug 9 one -- and because
    the reset WROTE the file, its mtime was newer than the traces, so every
    ordering-based freshness gate passed. The content was wrong and the traces
    carried no stamp to compare it against.
    """
    import hashlib
    lines = ridtext.splitlines()
    if DIGEST_MARKER not in lines:
        src = [ln for ln in lines if ln.startswith("source ")]
        commit = src[0].split()[1] if src and len(src[0].split()) > 1 else None
        had = _writer_existed_at(commit) if commit else "nogit"
        if had == "unresolved":
            warn("digest: run_id lists the outputs it produced", False,
                 f"no digest block, and the recorded source {commit} is not in this "
                 f"repository -- history may have been rewritten, or this is a clone "
                 f"of a rewritten repo. This is NOT evidence that the build predates "
                 f"the digest writer; the question could not be asked.")
        elif had == "nogit":
            warn("digest: run_id lists the outputs it produced", False,
                 f"no digest block, and the recorded source {commit} could not be "
                 f"resolved because this is not a git checkout (exported results "
                 f"directory) -- belonging is unverified, not disproven")
        else:
            # Absence is only excusable if the writer did not yet exist. If it did,
            # the block was stripped or the run died before appending it, and both
            # are states a reader must distrust.
            check("digest: a build that can write a digest did write one",
                  had == "no",
                  f"the source at {commit} contains the digest writer, but this "
                  f"directory has no digest block -- the run died before appending "
                  f"it, or the block was removed. Either way these files are not "
                  f"vouched for.")
        return

    i = lines.index(DIGEST_MARKER)
    entries = []
    for ln in lines[i + 1:]:
        parts = ln.split(None, 1)
        if len(parts) == 2 and len(parts[0]) == 64:
            entries.append((parts[0], parts[1].strip()))
    check("digest: block is non-empty", bool(entries),
          "'digest sha256' present with no '<sha256>  <file>' entries under it")

    bad, missing = [], []
    for want, name in entries:
        p = os.path.join(outdir, name)
        if not os.path.exists(p):
            missing.append(name)
            continue
        h = hashlib.sha256()
        with open(p, "rb") as f:
            for chunk in iter(lambda: f.read(1 << 20), b""):
                h.update(chunk)
        if h.hexdigest() != want:
            bad.append(name)
    check(f"digest: all {len(entries)} recorded outputs are present", not missing,
          f"recorded but absent: {missing[:5]}")
    check(f"digest: all {len(entries)} recorded outputs match their sha256", not bad,
          f"content differs from what the run recorded: {bad[:5]} -- either this "
          f"sidecar belongs to a different run, or the results were edited after it")

    # An unrecorded output is a warning, not a failure: a later analysis step may
    # legitimately drop a file here. A recorded one that CHANGED is the failure.
    #
    # Scoped to the simulator's own output names. Checking every *.csv would flag
    # rho_summary.csv and probe_range_sweep.csv -- artefacts of earlier, unrelated
    # runs that share the directory -- on every single run, and a warning that is
    # always present is one nobody reads.
    listed = {n for _, n in entries}
    extra = sorted(f for f in os.listdir(outdir)
                   if (f == "arm_summary.csv" or f.startswith("trace_"))
                   and f.endswith(".csv") and f not in listed)
    warn("digest: no unrecorded simulator output alongside the recorded ones",
         not extra,
         f"present but not in the digest: {extra[:5]} -- written after the run "
         f"recorded its outputs, or left behind by an earlier one")


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

    No sidecar is not a licence to assume. See resolve_modes().
    """
    p = os.path.join(outdir, "run_id.txt")
    if not os.path.exists(p):
        return None
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

    Returns None when there is no sidecar. It used to return "legacy", which was
    the same defect as the unchecked ofstream one layer up: absence produced a
    confident value instead of a demand for one. "Predates the flag, so legacy"
    happens to be true of every such directory on this disk, but it was inferred
    from mtimes -- and mtimes were separately shown unreliable here (cp, restore,
    the WSL mount). The caller must now supply the mode. See resolve_modes().
    """
    ridp = os.path.join(outdir, "run_id.txt")
    if not os.path.exists(ridp):
        return None
    with open(ridp) as f:
        lines = f.read().splitlines()
    inv = lines[1] if len(lines) > 1 else ""
    for tok in inv.split():
        if tok.startswith("--nlosSource="):
            return tok.split("=", 1)[1]
    return "legacy"


NLOS_MODES = ("legacy", "propagation")
PCS_FORMS = ("legacy", "v2")


def _argv_mode(flag, allowed):
    """Parse --flag=value from argv, rejecting anything outside `allowed`.

    A typo (--nlosSource=propogation) must not become a silent third mode that
    matches no gate and therefore fails none of them.
    """
    for a in sys.argv[1:]:
        if a.startswith(flag + "="):
            v = a.split("=", 1)[1]
            if v not in allowed:
                sys.exit(f"FATAL: {flag}={v} is not one of {'|'.join(allowed)}")
            return v
    return None


def resolve_modes(outdir):
    """Settle (nlos_source, pcs_form) from the sidecar, or from explicit flags.

    Three cases, and the middle one is the whole point:

      sidecar present         -- it is the record; use it. If the caller ALSO
                                 passed a mode, that is an assertion about the
                                 record and is checked against it, not silently
                                 ignored.
      sidecar absent, flags   -- the caller states what the sidecar would have
                                 said, and owns that statement.
      sidecar absent, no flags -- FATAL. --no-require-provenance says "I know
                                 there is no sidecar"; it must not also mean
                                 "and I decline to say what was in it". The old
                                 behaviour silently defaulted to legacy, which
                                 reintroduced the exact defect the provenance
                                 gate was promoted to close: the six NLOS gates
                                 read a propagation-produced directory as legacy
                                 and pass, because each one is keyed to the
                                 population the mode selects.
    """
    sc_nlos, sc_pcs = read_nlos_source(outdir), read_pcs_form(outdir)
    cl_nlos = _argv_mode("--nlosSource", NLOS_MODES)
    cl_pcs = _argv_mode("--pcsForm", PCS_FORMS)

    if sc_nlos is None:
        missing = [f for f, v in (("--nlosSource", cl_nlos), ("--pcsForm", cl_pcs))
                   if v is None]
        if missing:
            verb = "was" if len(missing) == 1 else "were"
            opts = {"--nlosSource": NLOS_MODES, "--pcsForm": PCS_FORMS}
            state = "  ".join(f"{m}=<{'|'.join(opts[m])}>" for m in missing)
            if REQUIRE_PROVENANCE:
                # The caller did not waive anything, so do not lecture them about
                # a flag they did not pass. The primary fault is the missing
                # sidecar; the modes are only needed if they choose to override.
                sys.exit(
                    f"FATAL: {outdir} has no run_id.txt, so this directory cannot "
                    f"say what produced it. Provenance is required by default.\n"
                    f"       If it genuinely predates the sidecar, audit it with "
                    f"--no-require-provenance AND state the mode explicitly:\n"
                    f"       --no-require-provenance  {state}")
            sys.exit(
                f"FATAL: {outdir} has no run_id.txt, so the mode cannot be read "
                f"from provenance, and {' and '.join(missing)} {verb} not passed.\n"
                f"       --no-require-provenance waives the SIDECAR, not the MODE: "
                f"six NLOS gates and the PCS column gates are keyed to it, and a "
                f"wrong mode makes them pass vacuously.\n"
                f"       State what the sidecar would have said: " + state)
        return cl_nlos, cl_pcs

    # Sidecar present. An explicit flag is a claim about it -- verify, don't drop.
    for name, cl, sc in (("--nlosSource", cl_nlos, sc_nlos),
                         ("--pcsForm", cl_pcs, sc_pcs)):
        if cl is not None and cl != sc:
            sys.exit(f"FATAL: {name}={cl} contradicts run_id.txt, which records "
                     f"{sc}. The sidecar is the record; fix the flag.")
    return sc_nlos, sc_pcs


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
    # --require-provenance / --no-require-provenance. DEFAULT ON, and an explicit
    # flag rather than a heuristic on the directory name: a heuristic would have to
    # guess which directories predate the sidecar, and a guess is what let absence
    # pass in the first place.
    global REQUIRE_PROVENANCE
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    if "--no-require-provenance" in sys.argv:
        REQUIRE_PROVENANCE = False
    outdir = args[0] if args else "results/causal"
    verify_harness_tracked()
    mode, pcsform = resolve_modes(outdir)
    read_nlos_source_cache["mode"] = mode
    read_nlos_source_cache["pcsForm"] = pcsform
    src = "run_id.txt provenance" if os.path.exists(
        os.path.join(outdir, "run_id.txt")) else "EXPLICIT FLAGS -- no sidecar"
    print(f"nlos_source {mode}  pcs_form {pcsform}  (from {src})")
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
