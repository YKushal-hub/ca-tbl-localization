#!/usr/bin/env python3
"""G120: the tier-2 field-error flags and the covariance trace columns.

usage: tof_field_gate.py <dir written by scripts/tof_field_gate.sh>

--tofFieldSigmaM raises the 2.4 GHz ToF sigma floors to a measured field level, --tofFieldBiasM
adds a deterministic over-estimate to every 2.4 GHz range, --traceCovariance appends tag 0's
posterior pxx,pxy,pyy to every trace row. All three are default-off.

Checks (each prints "G120 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     every run exits 0.
  DEFAULT  the three flags given explicitly at their defaults: every output file except
           run_id.txt is byte-identical to the control's.
  COV      --traceCovariance: every trace is the control's with ",pxx,pxy,pyy" appended to the
           header and three values to every row, and nothing else changes (the other files
           byte-identical). The firing on disk: a changed pre-existing cell, or a missing column.
  TRACE    on every row pxx + pyy equals traceSigma within its 4-decimal rounding, and the
           matrix is positive semi-definite: the columns are the filter's covariance.
  REACH    rule 4: some row has pxy != 0, and the control's header has no pxx column.
  SIGMA    --tofFieldSigmaM=83.6: on every static_2p4 row whose fix came from 2.4 GHz ranges
           (hadFix 1, fixTier 2.4GHz, usedFloor 0, nRangesEkf > 0), meanSigmaRawM >= 83.6. A cycle
           with fewer than three ranges has no fix and prints 0 there, so it is not read. The control
           has such rows, and on them meanSigmaRawM < 83.6 (rule 4: the floor is what moved).
  BIAS     --tofFieldBiasM=1000 on the probe: the rows of the other tiers are unchanged; on
           2.4 GHz rows every column but mean_abs_range_err_m and measured_over_crlb is
           unchanged (no random draw moved), and where the tier delivered,
           |mae_on - 1000| <= mae_off (triangle inequality per trial, within 3-decimal
           rounding). The control's mae is below 500 there, so the check discriminates.
"""
import csv
import filecmp
import glob
import os
import sys

BIAS = 1000.0
SIGMA = 83.6
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G120 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G120 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G120 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def files(d):
    out = set()
    for root, _, fs in os.walk(d):
        for f in fs:
            out.add(os.path.relpath(os.path.join(root, f), d))
    out.discard("run_id.txt")
    return out


def traces(d):
    return sorted(os.path.basename(p) for p in glob.glob(os.path.join(d, "trace_*.csv")))


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]
    runs = ("base", "zero", "cov", "sig", "probe_off", "probe_bias")
    bad = [n for n in runs if rc(d, n) != 0]
    report(not bad, "RUNS", f"{len(runs)} runs exited 0" if not bad else f"non-zero exit: {bad}")
    base, zero, cov, sig = (os.path.join(d, n) for n in ("base", "zero", "cov", "sig"))

    # DEFAULT
    fb, fz = files(base), files(zero)
    diff = sorted(f for f in fb & fz if not filecmp.cmp(os.path.join(base, f), os.path.join(zero, f), shallow=False))
    ok = fb == fz and not diff and len(traces(base)) == 2
    report(ok, "DEFAULT", f"{len(fb)} files; differing {diff[:3]}; only one side {sorted(fb ^ fz)[:3]}; traces {len(traces(base))} (need 2)")

    # COV, TRACE, REACH
    fc = files(cov)
    other = sorted(f for f in (fb & fc) if not f.startswith("trace_") and not filecmp.cmp(os.path.join(base, f), os.path.join(cov, f), shallow=False))
    cov_bad, trace_bad, psd_bad, nrows, npxy = [], [], [], 0, 0
    for t in traces(base):
        if not os.path.exists(os.path.join(cov, t)):
            cov_bad.append(f"{t}: absent")
            continue
        lb = open(os.path.join(base, t)).read().splitlines()
        lc = open(os.path.join(cov, t)).read().splitlines()
        if len(lb) != len(lc) or not lb:
            cov_bad.append(f"{t}: {len(lb)} vs {len(lc)} lines")
            continue
        if lc[0] != lb[0] + ",pxx,pxy,pyy":
            cov_bad.append(f"{t}: header")
            continue
        hdr = lb[0].split(",")
        it = hdr.index("traceSigma")
        for i in range(1, len(lb)):
            c = lc[i].split(",")
            if ",".join(c[:-3]) != lb[i] or len(c) != len(hdr) + 3:
                cov_bad.append(f"{t}: row {i}")
                break
            nrows += 1
            try:
                pxx, pxy, pyy = (float(x) for x in c[-3:])
            except ValueError:
                cov_bad.append(f"{t}: row {i} not numeric")
                break
            ts = float(c[it])
            if abs(pxx + pyy - ts) > 5e-5 + 1e-9 * abs(ts):
                trace_bad.append((t, i, pxx + pyy, ts))
            if pxx <= 0.0 or pyy <= 0.0 or pxx * pyy - pxy * pxy < -1e-9 * pxx * pyy:
                psd_bad.append((t, i, pxx, pxy, pyy))
            if pxy != 0.0:
                npxy += 1
    report(not cov_bad and not other and fb == fc and nrows > 0, "COV",
           f"{nrows} rows carry the old row unchanged plus 3 columns; other files differing {other[:3]}; "
           f"problems {cov_bad[:3]}")
    report(nrows > 0 and not trace_bad and not psd_bad, "TRACE",
           f"pxx+pyy != traceSigma on {len(trace_bad)} rows {trace_bad[:2]}; not PSD on {len(psd_bad)} {psd_bad[:2]}")
    base_hdr = [open(os.path.join(base, t)).readline().strip().split(",") for t in traces(base)]
    report(npxy > 0 and base_hdr and all("pxx" not in h for h in base_hdr), "REACH",
           f"rows with pxy != 0: {npxy} (need > 0); control header without pxx: {all('pxx' not in h for h in base_hdr)}")

    # SIGMA
    def sig_rows(run):
        p = glob.glob(os.path.join(d, run, "trace_static_2p4_seed*.csv"))
        if not p:
            return None
        out = []
        for r in csv.DictReader(open(p[0])):
            if r["hadFix"] == "1" and r["fixTier"] == "2.4GHz" and r["usedFloor"] == "0" and int(r["nRangesEkf"]) > 0:
                out.append(float(r["meanSigmaRawM"]))
        return out

    on, off = sig_rows("sig"), sig_rows("base")
    if on is None or off is None:
        missing("SIGMA", "no static_2p4 trace")
    else:
        low = [s for s in on if s < SIGMA - 1e-9]
        report(len(on) > 0 and not low and len(off) > 0 and all(s < SIGMA for s in off), "SIGMA",
               f"{len(on)} tier-2 fix rows, {len(low)} with meanSigmaRawM < {SIGMA} {low[:3]}; "
               f"control: {len(off)} rows, max {max(off) if off else None}")

    # BIAS
    def probe(run):
        p = os.path.join(d, run, "probe_range_sweep.csv")
        if not os.path.exists(p):
            return None
        return list(csv.DictReader(open(p)))

    po, pb = probe("probe_off"), probe("probe_bias")
    if po is None or pb is None:
        missing("BIAS", "no probe_range_sweep.csv")
    else:
        moved = {"mean_abs_range_err_m", "measured_over_crlb"}
        bad, n2, nreach = [], 0, 0
        if len(po) != len(pb):
            bad.append(f"{len(po)} vs {len(pb)} rows")
        for a, b in zip(po, pb):
            if a["tier"] != "2.4GHz":
                if a != b:
                    bad.append(f"{a['tier']} {a['distance_m']} changed")
                continue
            if any(a[k] != b[k] for k in a if k not in moved):
                bad.append(f"2.4GHz {a['distance_m']}: a column other than the error moved")
                continue
            if float(a["pdr"]) > 0.0:
                n2 += 1
                mo, mb = float(a["mean_abs_range_err_m"]), float(b["mean_abs_range_err_m"])
                if abs(mb - BIAS) > mo + 1e-3:
                    bad.append(f"2.4GHz {a['distance_m']}: |{mb} - {BIAS}| > {mo}")
                if mo < BIAS / 2:
                    nreach += 1
        report(n2 > 0 and nreach == n2 and not bad, "BIAS",
               f"{n2} delivered 2.4 GHz rows, {nreach} with control mae < {BIAS / 2}; problems {bad[:3]}")

    print(f"  G120: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
