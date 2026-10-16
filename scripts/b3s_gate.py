#!/usr/bin/env python3
"""G135: S1 --rssiRangeFromShadowing and S2 --tofErrorModel (master plan v2), read from probe sweeps.

  RUNS     a run's rc is not 0, or a run directory is missing.
  DEFAULT  the defaults given explicitly change an output (run_id.txt excepted).
  S1       868 MHz rows with PDR >= 0.95 (no reception truncation: lost packets are the unfavourably
           shadowed ones, so received readings at low PDR are biased small): mean |range error| / d is not within 15 % of the log-normal value
           E|10^(X/(10 n)) - 1|, X ~ N(0, s) clamped to [-2s, 3s] (the engine's clamp), s = that row's
           shadow_sd_eff_db, n = 2.3; or the 868 rows equal the control's.
  S2       per preset, 2.4 GHz rows with n >= 200 at 100-500 m: mean |range error| not within 20 % of the
           preset's own expectation (Monte Carlo of its law, z truncated at +-3); or the rows equal the control's.
"""
import csv, filecmp, glob, math, os, sys
import numpy as np

N_PL = 2.3
BIN = [50, 150, 250, 350, 450, 550, 650, 750, 850, 950, 1050, 1150, 1250, 1350]
MEAN = [3.2, 11.6, 46.7, 23.2, 35.4, 33.5, 29.0, 62.8, 60.5, 90.5, 67.0, 66.3, 48.0, 47.6]
SD = [13.1, 33.8, 99.2, 49.2, 71.7, 56.6, 62.6, 95.5, 90.1, 103.9, 97.8, 88.8, 95.1, 59.6]
rng = np.random.default_rng(1)
Z = rng.standard_normal(400000); Z = Z[np.abs(Z) <= 3.0]


def rows(d, r):
    return list(csv.DictReader(open(os.path.join(d, r, "probe_range_sweep.csv"))))


def expected(model, d, nlos_frac):
    if model == "gottschalk":
        return float(np.mean(np.abs((1.5 + 0.01 * d) * Z)))
    if model == "muller":
        mu, sd = np.interp(d, BIN, MEAN), np.interp(d, BIN, SD)
        return float(np.mean(np.abs(0.95 * (d + mu + sd * Z) + 1.0 - d)))
    los = np.mean(np.abs(75.0 + 69.0 * Z)); nl = np.mean(np.abs(144.0 + 112.0 * Z))
    return float((1 - nlos_frac) * los + nlos_frac * nl)


def main():
    d = sys.argv[1]; res = []
    runs = ["base", "off", "s1", "gottschalk", "muller", "wolf"]
    bad = [r for r in runs if not os.path.isdir(os.path.join(d, r)) or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"bad: {bad}" if bad else "6 runs exit 0"))
    if bad:
        return report(res)
    fa = sorted(os.listdir(os.path.join(d, "base")))
    diff = [f for f in fa if f != "run_id.txt" and not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "off", f), shallow=False)]
    res.append(("DEFAULT", not diff and fa == sorted(os.listdir(os.path.join(d, "off"))), f"{len(diff)} differ {diff[:2]}"))
    b, s1 = rows(d, "base"), rows(d, "s1")
    bad1, n1 = [], 0
    for r in s1:
        if r["tier"] != "868MHz" or float(r["pdr"]) < 0.95 or int(r["n"]) < 200:
            continue
        s = float(r["shadow_sd_eff_db"]); X = np.clip(Z * s, -2 * s, 3 * s)
        exp = float(np.mean(np.abs(10 ** (X / (10 * N_PL)) - 1)))
        got = float(r["mean_abs_range_err_m"]) / float(r["distance_m"])
        n1 += 1
        if abs(got - exp) > 0.15 * exp:
            bad1.append(f"{r['distance_m']}: {got:.3f} vs {exp:.3f}")
    same1 = [x for x in b if x["tier"] == "868MHz"] == [x for x in s1 if x["tier"] == "868MHz"]
    res.append(("S1", n1 > 0 and not bad1 and not same1, f"{n1} rows checked, off: {bad1[:3]}, identical to control: {same1}"))
    msgs, ok2 = [], True
    for m in ("gottschalk", "muller", "wolf"):
        rr = rows(d, m); nchk = 0
        for r in rr:
            if r["tier"] != "2.4GHz" or int(r["n"]) < 200 or not (100 <= float(r["distance_m"]) <= 500):
                continue
            exp = expected(m, float(r["distance_m"]), float(r["nlos_frac"]))
            got = float(r["mean_abs_range_err_m"]); nchk += 1
            if abs(got - exp) > 0.20 * exp:
                ok2 = False; msgs.append(f"{m}@{r['distance_m']}: {got:.1f} vs {exp:.1f}")
        same = [x for x in b if x["tier"] == "2.4GHz"] == [x for x in rr if x["tier"] == "2.4GHz"]
        if nchk == 0 or same:
            ok2 = False; msgs.append(f"{m}: {nchk} rows checked, identical to control {same}")
    res.append(("S2", ok2, "; ".join(msgs[:4]) if msgs else "3 presets reproduce their laws"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G135 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
