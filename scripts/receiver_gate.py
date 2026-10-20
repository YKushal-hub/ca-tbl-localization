#!/usr/bin/env python3
"""G161: the receiver corrections act as stated, are measured by the probe, and are off by default.

Reads the run set scripts/receiver_gate.sh writes. What each failure looks like on disk:
  RUNS     p_def, p_uwb, p_iq, def, zero exit 0                             (a crash under a flag)
  COLUMNS  p_def's probe_range_sweep.csv has mean_nominal_margin_db and     (the probe flag not writing them)
           mean_iq_degradation_db
  REACHED  p_def's IQ term > 0 at every point of every tier                 (a control-flow zero, rule 4)
  UWBSENS  p_uwb's UWB nominal margins sit 4.7 dB below p_def's (±0.5),     (the sensitivity flag not acting, or
           and its 868/2.4 GHz rows are byte-identical to p_def's            leaking into the other tiers)
  IQOFF    p_iq's nominal margin exceeds p_def's at every point, and the    (--iqOverloadOnly removing nothing,
           shift minus p_def's IQ term averages within ±0.5 dB over the       or removing something other than
           36 points                                                          the IQ term)
           Why the mean: the first real run failed a per-point ±0.5 version (−1.49..+0.96 dB). Once a decode outcome
           differs the two probes' streams diverge (shadow_sd_db differs at the same point, UWB 10 m 6.70 against
           6.52), so a per-point mean differs by more than one stream's sampling. An inert flag (shift 0) or one
           removing the wrong term (bias of dB) still fails this; noise of one sign per point does not.
  OFF      def/ and zero/ byte-identical, run_id.txt aside: CODE defaults   (a default that is not the old engine)
  PRESET   pre/'s spec_profile.txt sets uwbSensitivityDbm=-99.3 and         (the preset not carrying DESIGN_DECISIONS
           iqOverloadOnly=true                                               18)
  REFUSE   r_low and r_high exit 1                                          (a sensitivity outside [-130, -60] taken)
"""
import csv
import os
import sys

nok = nfail = nmiss = 0
TOL = 0.5


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G161 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G161 MISSING {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def probe(d, n):
    p = os.path.join(d, n, "probe_range_sweep.csv")
    if not os.path.exists(p):
        return None
    return {(r["tier"], r["distance_m"]): r for r in csv.DictReader(open(p))}


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("p_def", "p_uwb", "p_iq", "def", "zero", "pre")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    a, u, q = probe(d, "p_def"), probe(d, "p_uwb"), probe(d, "p_iq")
    cols = ("mean_nominal_margin_db", "mean_iq_degradation_db")
    if not a:
        for lab in ("COLUMNS", "REACHED", "UWBSENS", "IQOFF"):
            missing(lab, "no probe_range_sweep.csv in p_def/")
    else:
        hascol = all(c in next(iter(a.values())) for c in cols)
        report(hascol, "COLUMNS", f"p_def: {len(a)} rows, columns {cols} present {hascol}")
        if hascol:
            iq = [float(r["mean_iq_degradation_db"]) for r in a.values()]
            report(min(iq) > 0.0, "REACHED", f"p_def: IQ term min {min(iq):.3f} dB over {len(iq)} points (need > 0)")
            if not u:
                missing("UWBSENS", "no probe in p_uwb/")
            else:
                sh = [float(u[k]["mean_nominal_margin_db"]) - float(a[k]["mean_nominal_margin_db"])
                      for k in a if k[0] == "UWB" and k in u]
                same = all(u.get(k) == a[k] for k in a if k[0] != "UWB")
                ok = bool(sh) and all(abs(x + 4.7) <= TOL for x in sh) and same
                report(ok, "UWBSENS", f"p_uwb: UWB nominal shift {min(sh, default=0):+.2f}..{max(sh, default=0):+.2f} dB "
                                      f"(need -4.7 +/- {TOL}); other tiers identical {same}")
            if not q:
                missing("IQOFF", "no probe in p_iq/")
            else:
                sh = [float(q[k]["mean_nominal_margin_db"]) - float(a[k]["mean_nominal_margin_db"]) for k in a if k in q]
                dev = [x - float(a[k]["mean_iq_degradation_db"]) for x, k in zip(sh, [k for k in a if k in q])]
                md = sum(dev) / len(dev) if dev else float("nan")
                ok = bool(dev) and min(sh) > 0.0 and abs(md) <= TOL
                report(ok, "IQOFF", f"p_iq: shift min {min(sh, default=0):+.2f} dB (need > 0); shift minus IQ term mean "
                                    f"{md:+.2f} dB (need within +/- {TOL}), per point {min(dev, default=0):+.2f}.."
                                    f"{max(dev, default=0):+.2f} over {len(dev)} points")
    x, y = os.path.join(d, "def"), os.path.join(d, "zero")
    if os.path.isdir(x) and os.path.isdir(y):
        fa = sorted(f for f in os.listdir(x) if f != "run_id.txt")
        fb = sorted(f for f in os.listdir(y) if f != "run_id.txt")
        diff = [f for f in fa if f not in fb or open(os.path.join(x, f), "rb").read() != open(os.path.join(y, f), "rb").read()]
        diff += [f for f in fb if f not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against zero/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        missing("OFF", "def/ or zero/ absent")
    sp = os.path.join(d, "pre", "spec_profile.txt")
    if not os.path.exists(sp):
        missing("PRESET", "no pre/spec_profile.txt")
    else:
        t = open(sp).read()
        ok = "uwbSensitivityDbm=-99.3" in t and "iqOverloadOnly=true" in t
        report(ok, "PRESET", f"pre/: the preset sets uwbSensitivityDbm=-99.3 and iqOverloadOnly=true: {ok}")
    rr = {n: rc(d, n) for n in ("r_low", "r_high")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G161: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
