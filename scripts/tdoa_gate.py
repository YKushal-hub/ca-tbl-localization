#!/usr/bin/env python3
"""L.2.1 / G19 -- tier-1 TDoA fixes are PRODUCED, FUSED and BILLED, in a run.

WHY THIS EXISTS. The unit suite (ca_tbl_tdoa_test) proves the solver, the covariance and
the policy hand-off on constructed inputs. It cannot prove that a run stamps anything,
solves anything or fuses anything -- the state this project's mechanisms have repeatedly
sat in with green suites (L.1.1, L.1.4: zero scenario call sites). This gate reads what a
RUN wrote.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9). With --tier1Tdoa the scenario writes
tdoa_<arm>_seed<N>.csv, one row per cycle whose fix set is tier 1 with >= 3 ranges:

    t,cycle,nRanges,nStamps,repliesBilled,downlinkBilled,verdict,seededBy,iterations,
    refAnchor,tdoaX,tdoaY,rxx,rxy,ryy,chi2,chanOk,trueX,trueY,tdoaErrM,rssiOk,rssiErrM,
    fused,rejected,estErrM,nRoots,tieChosen,tieOther

The defect this guards against has the form of ABSENCE -- the flag unwired, every solve
refused, fixes solved and never fused, stamps with no billed frame under them -- so it is
checked by COUNTING the rows of a file a fresh run must write (rule 12) and recounting
them against the run's own printed counters (rule 3), never by scanning for a bad value.

WHAT IT CHECKS
  1. The file exists, has the header above and at least one row.
  2. Every row is consistent with the mechanism's contract:
     - verdict and seededBy are known names; an OK row has a finite position within
       50 km of the truth (no tier-1 radio reaches 5 km; the first draft's two runaway
       "fixes" sat 1e17 m out), a positive-definite covariance, >= 3 stamps, and a seed;
     - seededBy=chan implies chanOk=1; seededBy=root implies exactly 3 stamps and 1 or 2
       roots; >= 4 stamps never report root; AMBIGUOUS has 2 roots, NO_ROOT has 0;
     - a stamp is a surveyed gateway's record of one of the cycle's replies, so
       nStamps <= nRanges, and -- the BILLING contract -- with replies billed
       (downlinkBilled=0) nStamps <= repliesBilled, while under the edge server
       (downlinkBilled=1) no reply is billed;
     - the fix follows the cycle's verdict: fused implies OK, and an OK row the integrity
       check did not reject is fused.
  3. The L.2.1 done-when: OK solves on at least --min-share (default 0.5) of the rows --
     the rows ARE the denominator, tier-1 fix cycles -- and at least one fused.
  4. --stdout LOG: exactly one "TDoA tier 1:" block (run a single arm). Its fix cycles,
     OK solves and fused count must equal the CSV's rows, OK rows and fused rows; the
     policy's own counter (GetAuxFusedCount) must equal the fused count; "stamps beyond a
     billed reply" must be 0; the stamps and replies billed on OK cycles must equal the
     CSV's sums; and the run's stamps may not exceed the replies they rode, nor its
     tier-1 exchanges the tier-1 transmissions.

usage: tdoa_gate.py [--min-share F] [--stdout LOG] <tdoa csv>
exit 0 PASS, 1 FAIL, 2 usage.
"""
import csv
import math
import re
import sys

COLS = ["t", "cycle", "nRanges", "nStamps", "repliesBilled", "downlinkBilled", "verdict",
        "seededBy", "iterations", "refAnchor", "tdoaX", "tdoaY", "rxx", "rxy", "ryy", "chi2",
        "chanOk", "trueX", "trueY", "tdoaErrM", "rssiOk", "rssiErrM", "fused", "rejected",
        "estErrM", "nRoots", "tieChosen", "tieOther"]
VERDICTS = {"OK", "TOO_FEW", "COLLINEAR", "SINGULAR", "NO_CONVERGE", "BAD_INPUT", "NO_ROOT",
            "AMBIGUOUS", "DIVERGED"}
SEEDS = {"none", "chan", "rssi", "root"}
FAR_M = 50000.0


def fnum(s):
    try:
        return float(s)
    except ValueError:
        return float("nan")


def main(argv):
    args = argv[1:]
    min_share = 0.5
    stdout_log = None
    while args and args[0].startswith("--"):
        a = args.pop(0)
        if a == "--min-share" and args:
            min_share = float(args.pop(0))
        elif a == "--stdout" and args:
            stdout_log = args.pop(0)
        else:
            print(__doc__)
            return 2
    if len(args) != 1:
        print(__doc__)
        return 2
    path = args[0]
    try:
        with open(path, newline="") as fh:
            rd = csv.reader(fh)
            header = next(rd, None)
            rows = [r for r in rd if r]
    except OSError as e:
        print(f"G19 FAIL: cannot read {path}: {e} -- the flag-on run wrote no TDoA log")
        return 1
    if header != COLS:
        print(f"G19 FAIL: header {header} is not {COLS}")
        return 1
    print(f"G19 rows: {len(rows)} in {path}")
    if not rows:
        print("G19 FAIL: zero rows -- no cycle's fix set was tier 1, or nothing was solved")
        return 1

    bad = []
    n_ok = n_fused = st_ok = rep_ok = 0
    for n, r in enumerate(rows, start=2):
        if len(r) != len(COLS):
            bad.append(f"line {n}: {len(r)} fields")
            continue
        d = dict(zip(COLS, r))
        try:
            n_rng = int(d["nRanges"])
            n_st = int(d["nStamps"])
            rep = int(d["repliesBilled"])
            dl = int(d["downlinkBilled"])
            chan = int(d["chanOk"])
            fused = int(d["fused"])
            rej = int(d["rejected"])
            roots = int(d["nRoots"])
        except ValueError:
            bad.append(f"line {n}: non-integer count field")
            continue
        v = d["verdict"]
        s = d["seededBy"]
        if v not in VERDICTS:
            bad.append(f"line {n}: unknown verdict {v!r}")
            continue
        if s not in SEEDS:
            bad.append(f"line {n}: unknown seededBy {s!r}")
        if n_st > n_rng:
            bad.append(f"line {n}: {n_st} stamps from {n_rng} ranges -- a stamp with no reply")
        if dl == 0 and n_st > rep:
            bad.append(f"line {n}: {n_st} stamps but {rep} replies billed -- a stamp rode "
                       "a frame nobody paid for")
        if dl == 1 and rep != 0:
            bad.append(f"line {n}: edge-server row bills {rep} replies")
        if v == "AMBIGUOUS" and roots != 2:
            bad.append(f"line {n}: AMBIGUOUS with {roots} roots")
        if v == "NO_ROOT" and roots != 0:
            bad.append(f"line {n}: NO_ROOT with {roots} roots")
        if fused and v != "OK":
            bad.append(f"line {n}: fused a {v} solve")
        if v != "OK":
            continue
        n_ok += 1
        st_ok += n_st
        rep_ok += rep
        x, y = fnum(d["tdoaX"]), fnum(d["tdoaY"])
        tx, ty = fnum(d["trueX"]), fnum(d["trueY"])
        rxx, rxy, ryy = fnum(d["rxx"]), fnum(d["rxy"]), fnum(d["ryy"])
        if not all(math.isfinite(q) for q in (x, y, rxx, rxy, ryy)):
            bad.append(f"line {n}: OK with a non-finite position or covariance")
            continue
        if math.hypot(x - tx, y - ty) > FAR_M:
            bad.append(f"line {n}: OK fix {math.hypot(x - tx, y - ty):.3g} m from the truth "
                       "-- a runaway, not a fix")
        if not (rxx > 0 and ryy > 0 and rxx * ryy - rxy * rxy > 0):
            bad.append(f"line {n}: OK with a covariance that is not positive definite")
        if n_st < 3:
            bad.append(f"line {n}: OK from {n_st} stamps")
        if s == "none":
            bad.append(f"line {n}: OK with no seed")
        if s == "chan" and chan != 1:
            bad.append(f"line {n}: seeded by chan but chanOk={chan}")
        if s == "root" and (n_st != 3 or roots not in (1, 2)):
            bad.append(f"line {n}: root-seeded with {n_st} stamps and {roots} roots")
        if n_st >= 4 and s == "root":
            bad.append(f"line {n}: {n_st} stamps reported as a three-anchor root")
        if rej == 0 and fused == 0:
            bad.append(f"line {n}: an OK fix on an accepted cycle was never fused")
        n_fused += fused

    for b in bad[:12]:
        print(f"G19 FAIL: {b}")
    if bad:
        print(f"G19 FAIL: {len(bad)} inconsistent row(s)")
        return 1

    share = n_ok / len(rows)
    print(f"G19 tier-1 fix cycles {len(rows)}, TDoA OK {n_ok} ({100.0 * share:.1f}%), fused "
          f"{n_fused}; stamps in OK solves {st_ok}, replies billed on them {rep_ok}")
    ok = True
    if n_ok == 0 or share < min_share:
        print(f"G19 FAIL: OK on {n_ok} of {len(rows)} tier-1 fix cycles, below the "
              f"{min_share:.2f} share the done-when asks for")
        ok = False
    if n_fused == 0:
        print("G19 FAIL: no TDoA fix ever reached the filter")
        ok = False

    if stdout_log is not None:
        try:
            text = open(stdout_log, encoding="utf-8", errors="replace").read()
        except OSError as e:
            print(f"G19 FAIL: cannot read {stdout_log}: {e}")
            return 1
        a = re.findall(r"TDoA tier 1: fix cycles (\d+) .*?solves OK (\d+) .*?; fused (\d+) "
                       r"\(policy (\d+)\)", text)
        b = re.findall(r"TDoA billing: (\d+) stamps in OK solves, replies billed on those "
                       r"cycles (\d+), downlinks (\d+); node tier-1 exchanges (\d+) .*?stamps "
                       r"(\d+) against replies (\d+) .*?stamps beyond a billed reply (\d+); "
                       r"tier-1 over the run: tx (\d+)", text)
        if len(a) != 1 or len(b) != 1:
            print(f"G19 FAIL: expected one TDoA block in {stdout_log}, found {len(a)}/{len(b)} "
                  "(run a single arm)")
            return 1
        fc, sok, fu, pol = (int(q) for q in a[0])
        s_ok, r_ok, _dls, exch, s_all, r_all, beyond, tx = (int(q) for q in b[0])
        if not (fc == len(rows) and sok == n_ok and fu == n_fused):
            print(f"G19 FAIL: printed fix cycles {fc}, OK {sok}, fused {fu} but the CSV holds "
                  f"{len(rows)}, {n_ok}, {n_fused}")
            ok = False
        if pol != fu:
            print(f"G19 FAIL: the policy fused {pol} but the scenario counted {fu}")
            ok = False
        if beyond != 0:
            print(f"G19 FAIL: {beyond} stamp(s) beyond a billed reply")
            ok = False
        if not (s_ok == st_ok and r_ok == rep_ok):
            print(f"G19 FAIL: printed OK-cycle stamps {s_ok} / replies {r_ok} but the CSV sums "
                  f"to {st_ok} / {rep_ok}")
            ok = False
        if s_all > r_all or exch > tx:
            print(f"G19 FAIL: stamps {s_all} > replies {r_all}, or tier-1 exchanges {exch} > "
                  f"tier-1 tx {tx}")
            ok = False
        if ok:
            print(f"G19 recount agrees: rows {fc}, OK {sok}, fused {fu} = policy {pol}; "
                  f"{s_all} stamps on {r_all} replies over {exch} broadcasts (tier-1 tx {tx}), "
                  f"none beyond a billed reply")
    if not ok:
        return 1
    print("G19 PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
