#!/usr/bin/env python3
"""G20b: --uwbNlosModel=ieee802154a implements report/PREREG_UWB_NLOS.md sect 2 as written.

WHAT IT READS. One run's nlos_links_<arm>_seed<N>.csv and nlos_summary_<arm>_seed<N>.csv
(written by --nlosClassifier). Only rows with tier == UWB are read.

WHAT IT ASSERTS (the registration's P4, as a control-flow check, not a result):
  1. every UWB row with truthNlos = 1 has nlosDb equal to the 802.15.4a NLOS-minus-LOS
     path loss at that row's distM, within 1e-6 dB;
  2. every UWB row with truthNlos = 0 has nlosDb = 0;
  3. the model is wired into the scenario: the NLOS share of polled UWB links, decoded or
     lost, is at least --min-nlos-frac (default 0.10). Polled = decoded rows + lostLos +
     lostNlos from the summary; NLOS = decoded NLOS rows + lostNlos. Under legacy the outdoor
     NLOS probability is below 0.4 % at any distance up to 200 m (1 - exp(-d / 54 km)), so
     10 % is over 25x anything legacy can produce. "At least one NLOS poll" was tried first
     and REFUTED by its own negative fixture: a legacy run drew one NLOS on a 44 m poll.
     Absence is the failure (CLAUDE.md rule 9).

WHY NOT REQUIRE A DECODED NLOS ROW. Almost every NLOS UWB poll is lost -- the excess is
27-37 dB against UWB's margin -- so decoded NLOS rows are rare and assertion 1 is often
vacuous in a short run. It says so when it is. The non-vacuous check of the formula on EVERY
evaluation, decoded or not, is in the unit suite (scratch/ca_tbl_nlos_test.cc, "engine,
ieee802154a"), which G20 runs.

THE FORMULA IS RE-DERIVED HERE, not imported: the registration's own parameter table
(IEEE P802.15-04-0662-02-004a, CM3/CM4 office, CM5/CM6 outdoor), so the C++ is checked
against an independent transcription (rule 3).

  dPL = (PL0_N - PL0_L) + 10 (n_N - n_L) log10(max(d, 1)) + 20 (k_N - k_L) log10(f / 5 GHz)

Usage: uwb_nlos_model_gate.py [--office] [--carrier-hz F] LINKS.csv SUMMARY.csv
Exit 0 pass, 1 fail, 2 usage / unreadable input.
"""
import csv
import math
import sys

PAIRS = {
    # name: (PL0_L, PL0_N, n_L, n_N, kappa_L, kappa_N)
    "outdoor": (45.6, 73.0, 1.76, 2.5, 0.12, 0.13),
    "office": (35.4, 57.9, 1.63, 3.07, 0.03, 0.71),
}
TOL_DB = 1e-6


def excess_db(d, pair, f_hz):
    pl0l, pl0n, nl, nn, kl, kn = PAIRS[pair]
    return (pl0n - pl0l) + 10.0 * (nn - nl) * math.log10(max(d, 1.0)) + \
        20.0 * (kn - kl) * math.log10(f_hz / 5e9)


def main(argv):
    pair = "outdoor"
    f_hz = 7.99e9
    min_frac = 0.10
    args = []
    i = 0
    while i < len(argv):
        a = argv[i]
        if a == "--office":
            pair = "office"
        elif a == "--carrier-hz":
            i += 1
            f_hz = float(argv[i])
        elif a == "--min-nlos-frac":
            i += 1
            min_frac = float(argv[i])
        else:
            args.append(a)
        i += 1
    if len(args) != 2:
        print(__doc__.strip().splitlines()[-2])
        return 2
    links, summary = args
    try:
        with open(links, newline="") as fh:
            rows = [r for r in csv.DictReader(fh) if r.get("tier") == "UWB"]
        with open(summary, newline="") as fh:
            srows = [r for r in csv.DictReader(fh)
                     if r.get("kind") == "confusion" and r.get("tier") == "UWB"]
    except OSError as e:
        print(f"  G20b ERROR: {e}")
        return 2
    if len(srows) != 1:
        print(f"  G20b ERROR: expected one UWB confusion row in {summary}, found {len(srows)}")
        return 2

    fail = []
    nlos_rows = [r for r in rows if r["truthNlos"] == "1"]
    los_rows = [r for r in rows if r["truthNlos"] == "0"]
    worst = 0.0
    for r in nlos_rows:
        got = float(r["nlosDb"])
        want = excess_db(float(r["distM"]), pair, f_hz)
        worst = max(worst, abs(got - want))
        if abs(got - want) > TOL_DB:
            fail.append(f"NLOS row t={r['t']} anchor={r['anchor']} distM={r['distM']}: "
                        f"nlosDb {got} != {pair} dPL {want:.9f}")
    for r in los_rows:
        if float(r["nlosDb"]) != 0.0:
            fail.append(f"LOS row t={r['t']} anchor={r['anchor']}: nlosDb {r['nlosDb']} != 0")
    lost_nlos = int(srows[0]["lostNlos"])
    lost_los = int(srows[0]["lostLos"])
    polled = len(rows) + lost_los + lost_nlos
    nlos_polls = len(nlos_rows) + lost_nlos
    frac = (nlos_polls / polled) if polled else 0.0
    print(f"  G20b: {len(rows)} decoded UWB rows ({len(nlos_rows)} NLOS, {len(los_rows)} LOS), "
          f"lost {lost_los} LOS + {lost_nlos} NLOS; NLOS share of {polled} polls {frac:.3f} "
          f"(need >= {min_frac}); pair {pair}; worst |nlosDb - dPL| on NLOS rows "
          f"{worst:.3g} dB (tolerance {TOL_DB})")
    if frac < min_frac:
        fail.append(f"NLOS share of polled UWB links {nlos_polls}/{polled} = {frac:.3f} is "
                    f"below {min_frac}: the model is not in the scenario's UWB engine (a "
                    f"legacy run, or --uwbNlosModel not reaching it)")
    elif not nlos_rows:
        print("  G20b note: no decoded NLOS row, so assertion 1 is vacuous in this run; the "
              "formula on every evaluation is the unit suite's check")
    for m in fail[:10]:
        print("  G20b FAIL: " + m)
    if len(fail) > 10:
        print(f"  G20b FAIL: ... and {len(fail) - 10} more")
    if fail:
        return 1
    print("  G20b PASS: NLOS drawn on UWB polls; every decoded NLOS row carries the 802.15.4a "
          "excess; every LOS row 0")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
