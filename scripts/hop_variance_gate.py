#!/usr/bin/env python3
"""G58: per-hop variance and NLOS enter a relayed range's sigma (NW-1 step 3).

usage: hop_variance_gate.py <dir written by scripts/hop_variance_gate.sh>

Runs read:
  on/        --hopVarianceTerms: relay_chain_<arm>_seed7.csv carries each leg's readings and
             terms; on.log carries one NW-1 hop variance line per arm.
  off/       the same without the flag (the control).
  r_nochain, r_noclass   refusals: without --relayChain, without --nlosClassifier.

Checks (each prints "G58 ok|FAIL|MISSING <LABEL> ..."):
  RUNS     every run the checks read exited 0.
  COLS     every decision row (either leg, not COMPOSE_REFUSED) carries the leg columns, and
           fuse and refused rows leave them empty.
  VAR      legN and legSDb are recomputed from the readings: per (consumer, relay) link, in
           row order, the last 8 readings, the larger von Neumann sigma of RSSI and SNR, given
           from 3 readings.
  CONV     legVarSigmaM = legSDb / legSlopeDbPerM (0 below 3 readings); the slope is the
           tier's log-distance slope, so slope x range is one constant per tier.
  NLOS     legNlos is legScore >= the threshold the line states (and 0 when unscored).
  SIGMA    sigmaLegM = sqrt(sigmaRawM^2 + legVarSigmaM^2), times the line's inflation when NLOS.
  EFF      an admitted row's sigmaEffM^2 - sigmaLegM^2 lies in [0, carriedVarM2] (u'P u for a
           unit u is at most trace P); a refused row carries no sigmaEffM.
  REACH    rule 4: admitted legs with a variance term, admitted legs inflated as NLOS, and
           admitted legs at depths 1 and 2.
  COUNTS   each NW-1 line is recounted from its arm's rows.
  CONTROL  without the flag: no leg columns and no NW-1 hop variance line.
  R        both refusals exit 1 with the reason.
"""
import csv
import glob
import math
import os
import re
import sys

LEGC = ["legRssiDbm", "legSnrDb", "legRangeM", "legN", "legSDb", "legSlopeDbPerM",
        "legVarSigmaM", "legScore", "legNlos", "sigmaLegM"]
LINE = re.compile(r"NW-1 hop variance terms \((\S+)\): legs (\d+); a variance term on (\d+) "
                  r"\(mean sigma_var ([-\d.]+) m\); NLOS-scored (\d+), inflated (\d+) "
                  r"\(x([-\d.]+) at score >= ([-\d.]+)\); mean sigma raw ([-\d.]+) m -> leg "
                  r"([-\d.]+) m")
nok = nfail = nmiss = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G58 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G58 FAIL    {label:<8}{msg}")


def missing(label, msg):
    global nmiss
    nmiss += 1
    print(f"  G58 MISSING {label:<8}{msg}")


def rc(d, n):
    try:
        return int(open(os.path.join(d, n + ".rc")).read().strip())
    except (OSError, ValueError):
        return None


def vn(xs):
    return math.sqrt(sum((xs[i] - xs[i - 1]) ** 2 for i in range(1, len(xs))) / (2.0 * (len(xs) - 1)))


def close(a, b, rel=1e-4, abs_=2e-4):
    return abs(a - b) <= max(abs_, rel * max(abs(a), abs(b)))


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    d = sys.argv[1]

    bad = [n for n in ("on", "off") if rc(d, n) != 0]
    report(not bad, "RUNS", "every run the checks read exited 0" if not bad else f"non-zero exit: {bad}")

    files = sorted(glob.glob(os.path.join(d, "on", "relay_chain_*_seed*.csv")))
    try:
        log = open(os.path.join(d, "on.log")).read()
    except OSError:
        log = ""
    lines = {m.group(1): m for m in LINE.finditer(log)}
    if not files:
        missing("COLS", "no relay_chain_* in on/")
        return 1

    colbad, rows_by_arm = [], {}
    for f in files:
        arm = re.search(r"relay_chain_(.+)_seed\d+\.csv$", f).group(1)
        rd = csv.DictReader(open(f))
        if any(c not in (rd.fieldnames or []) for c in LEGC):
            colbad.append((arm, "header lacks the leg columns"))
            rows_by_arm[arm] = []
            continue
        rows = list(rd)
        rows_by_arm[arm] = rows
        for r in rows:
            decision = r["leg"] in ("relay", "tag") and r["verdict"] != "COMPOSE_REFUSED"
            filled = all(r[c] != "" for c in LEGC)
            empty = all(r[c] == "" for c in LEGC)
            if decision and not filled:
                colbad.append((arm, r["t"], r["consumerId"], r["relayId"], "decision row without terms"))
            if not decision and not empty:
                colbad.append((arm, r["t"], r["leg"], r["verdict"], "terms on a non-decision row"))
    legs = {a: [r for r in rows if r["leg"] in ("relay", "tag") and r["verdict"] != "COMPOSE_REFUSED"
                and r["tier"] in ("1", "2")] for a, rows in rows_by_arm.items()}
    nlegs = sum(len(v) for v in legs.values())
    report(not colbad and nlegs > 0, "COLS",
           f"{nlegs} legs with terms over {len(files)} arms; misplaced {len(colbad)} (need 0)"
           + (f": {colbad[:3]}" if colbad else ""))

    # VAR: recompute the windows.
    varbad = []
    for a, rs in legs.items():
        hist = {}
        for r in rs:
            k = (r["consumerId"], r["relayId"])
            h = hist.setdefault(k, [])
            h.append((float(r["legRssiDbm"]), float(r["legSnrDb"])))
            del h[:-8]
            n = len(h)
            s = max(vn([x for x, _ in h]), vn([y for _, y in h])) if n >= 3 else 0.0
            if int(r["legN"]) != n or not close(float(r["legSDb"]), s, abs_=1e-3):
                varbad.append((a, r["t"], k, r["legN"], n, r["legSDb"], round(s, 6)))
    report(not varbad, "VAR", f"legN and legSDb recomputed from the readings; mismatches {len(varbad)} (need 0)"
           + (f": {varbad[:3]}" if varbad else ""))

    # CONV.
    convbad, k_by_tier = [], {}
    for a, rs in legs.items():
        for r in rs:
            n, sdb, sl, var = int(r["legN"]), float(r["legSDb"]), float(r["legSlopeDbPerM"]), float(r["legVarSigmaM"])
            want = sdb / sl if (n >= 3 and sl > 0) else 0.0
            if not close(var, want, abs_=1e-3):
                convbad.append((a, r["t"], "sigma_var", var, round(want, 6)))
            rng = max(float(r["legRangeM"]), 1.0 / 0.99)  # the clamp the slope is taken at
            if rng <= 1000.0:
                k_by_tier.setdefault(r["tier"], []).append(sl * rng)
    spread = {t: (min(v), max(v)) for t, v in k_by_tier.items()}
    for t, (lo, hi) in spread.items():
        if hi - lo > 1e-3 * hi + 2e-6 * 1000:
            convbad.append(("tier", t, "slope x range not one constant", lo, hi))
    report(not convbad and bool(spread), "CONV",
           f"sigma_var = s / slope on every leg; slope x range per tier {{{', '.join(f'{t}: {lo:.4f}..{hi:.4f}' for t, (lo, hi) in spread.items())}}} dB; bad {len(convbad)} (need 0)"
           + (f": {convbad[:3]}" if convbad else ""))

    # NLOS and SIGMA need the line's threshold and inflation.
    if not lines:
        missing("NLOS", "no NW-1 hop variance line in on.log")
        missing("SIGMA", "no NW-1 hop variance line in on.log")
    else:
        anyl = next(iter(lines.values()))
        infl, thr = float(anyl.group(7)), float(anyl.group(8))
        nlbad, sgbad = [], []
        for a, rs in legs.items():
            for r in rs:
                sc, nl = float(r["legScore"]), int(r["legNlos"])
                want = 1 if (sc >= 0.0 and sc >= thr) else 0
                if nl != want:
                    nlbad.append((a, r["t"], sc, nl))
                raw, var = float(r["sigmaRawM"]), float(r["legVarSigmaM"])
                leg = math.sqrt(raw * raw + var * var) * (infl if nl else 1.0)
                if not close(float(r["sigmaLegM"]), leg, abs_=1e-3):
                    sgbad.append((a, r["t"], r["sigmaLegM"], round(leg, 6)))
        report(not nlbad, "NLOS", f"legNlos = score >= {thr}; mismatches {len(nlbad)} (need 0)"
               + (f": {nlbad[:3]}" if nlbad else ""))
        report(not sgbad, "SIGMA", f"sigmaLegM = sqrt(raw^2 + var^2) x {infl} when NLOS; mismatches {len(sgbad)} (need 0)"
               + (f": {sgbad[:3]}" if sgbad else ""))

    # EFF.
    effbad, nadm = [], 0
    for a, rs in legs.items():
        for r in rs:
            eff, leg, car = float(r["sigmaEffM"]), float(r["sigmaLegM"]), float(r["carriedVarM2"])
            if r["verdict"] == "ADMIT":
                nadm += 1
                proj = eff * eff - leg * leg
                if proj < -1e-2 or proj > car + 1e-2 + 1e-4 * car:
                    effbad.append((a, r["t"], round(proj, 4), car))
            elif eff != 0.0:
                effbad.append((a, r["t"], r["verdict"], eff))
    report(not effbad and nadm > 0, "EFF",
           f"{nadm} admitted legs, sigma_eff composed from sigma_leg within [0, trace P]; bad {len(effbad)} (need 0)"
           + (f": {effbad[:3]}" if effbad else ""))

    # REACH.
    adm = [r for rs in legs.values() for r in rs if r["verdict"] == "ADMIT"]
    withv = sum(1 for r in adm if float(r["legVarSigmaM"]) > 0)
    infl_n = sum(1 for r in adm if r["legNlos"] == "1")
    depths = sorted({int(r["hopDepth"]) for r in adm})
    ok = withv > 0 and infl_n > 0 and 1 in depths and 2 in depths
    report(ok, "REACH", f"admitted legs with a variance term {withv}, inflated as NLOS {infl_n} (need > 0 each); depths {depths} (need 1 and 2)")

    # COUNTS.
    cbad = []
    for a, rs in legs.items():
        m = lines.get(a)
        if not m:
            cbad.append((a, "no line"))
            continue
        n = len(rs)
        wv = [r for r in rs if int(r["legN"]) >= 3 and float(r["legSlopeDbPerM"]) > 0]
        sc = sum(1 for r in rs if float(r["legScore"]) >= 0)
        inf = sum(1 for r in rs if r["legNlos"] == "1")
        mv = sum(float(r["legVarSigmaM"]) for r in wv) / len(wv) if wv else 0.0
        mr = sum(float(r["sigmaRawM"]) for r in rs) / n if n else 0.0
        ml = sum(float(r["sigmaLegM"]) for r in rs) / n if n else 0.0
        got = (int(m.group(2)), int(m.group(3)), int(m.group(5)), int(m.group(6)))
        want = (n, len(wv), sc, inf)
        means_ok = close(float(m.group(4)), mv, abs_=2e-3) and close(float(m.group(9)), mr, abs_=2e-3) \
            and close(float(m.group(10)), ml, abs_=2e-3)
        if got != want or not means_ok:
            cbad.append((a, got, want, m.group(4), round(mv, 4), m.group(9), round(mr, 4), m.group(10), round(ml, 4)))
    report(not cbad and len(lines) == len(legs), "COUNTS",
           f"NW-1 lines {len(lines)} for {len(legs)} arms, recounted; bad {len(cbad)} (need 0)" + (f": {cbad[:2]}" if cbad else ""))

    # CONTROL.
    offcols = 0
    for f in glob.glob(os.path.join(d, "off", "relay_chain_*_seed*.csv")):
        hdr = open(f).readline().strip().split(",")
        offcols += sum(1 for c in LEGC if c in hdr)
    try:
        offlog = open(os.path.join(d, "off.log")).read()
    except OSError:
        offlog = None
    if offlog is None:
        missing("CONTROL", "no off.log")
    else:
        offl = len(LINE.findall(offlog)) + offlog.count("NW-1 hop variance terms")
        report(offcols == 0 and offl == 0, "CONTROL", f"without the flag: leg columns {offcols}, NW-1 hop variance lines {offl} (need 0)")

    # R.
    rb = []
    for n in ("r_nochain", "r_noclass"):
        code = rc(d, n)
        try:
            txt = open(os.path.join(d, n + ".log")).read()
        except OSError:
            txt = ""
        if code != 1 or "--hopVarianceTerms needs" not in txt:
            rb.append((n, code))
    report(not rb, "R", "both refusals exit 1 with the reason" if not rb else f"refusals not as expected: {rb}")

    print(f"  G58: {nok} ok, {nfail} failed, {nmiss} missing input")
    return 0 if (nfail == 0 and nmiss == 0) else 1


if __name__ == "__main__":
    sys.exit(main())
