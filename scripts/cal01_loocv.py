#!/usr/bin/env python3
"""CAL-01 -- PCS weight calibration by leave-one-SEED-out cross-validation.

Applies report/PREREG_CAL01_LOOCV.md. Committed WITH that registration and before the
first cell ran, so the analysis existed before the data did. A correction made after the
data is recorded in report/CAL01_LOOCV_RESULTS.md, never folded in here silently.

    python3 scripts/cal01_loocv.py --root results/cal01_loocv_20260928 --source <sha>
    python3 scripts/cal01_loocv.py --selftest

numpy only (there is no scipy on this machine). Spearman is Pearson on average ranks.

WHAT IS CALIBRATED. PCS under --pcsForm=weighted is w1*t1 + w2*t2 + w3*t3 with
    t1 = traceRef/(traceRef + traceSigma), traceRef = 10 (the default)
    t2 = 1/max(gdop, 1)
    t3 = mean(1 - NLOS_score) over the fix set's scored links (--pcsNlosClassifier), or
         the LOS fraction when no link in the set is scored.
t1 and t2 are recomputed from the trace. t3 is not in the trace, so it is back-derived
from the trace's pcs with the hand-set weights the runs used (2, 0.5, 0.5), and -- rule 3
-- checked against a quantity the derivation never read: the same mean recomputed from
nlos_links_<arm>_seed<N>.csv (abort A7).

THE CONTROL is the registered monotone-of-filter-statistic control of
report/PREREG_PCS_FORMS.md: Spearman rho(-traceSigma, error). t1 is strictly decreasing
in traceSigma, so the control ranks cycles exactly as PCS with weights (1, 0, 0) does --
the control is a point of the family being calibrated (abort A9 checks that on the data).

THE DECIDING FIT, FIT-R, is the weighting that minimises the registered loss itself: the
mean per-seed rho(score, error) over the training seeds (more negative = a confidence
score that ranks error better). It is searched over a fixed grid of directions in which
only RATIOS matter, because a rank is invariant to positive scale: w = s*(1, r2, r3) with
r in {0} U {+-10^(k/8), k = -48..48} and s = +-1, plus the w1 = 0 circle at 0.5 degree,
plus the hand-set direction. The ratio grid is logarithmic because t1 is saturated: on a
tier-1 fix it lives near 1e-3 while t2 and t3 vary by tenths, so a weighting that lets t1
speak within tier 1 needs w2/w1 near 1e-3, which a uniform grid on the sphere cannot
resolve. Grid index 0 is (1, 0, 0), the control itself, and argmin takes the first index
on a tie -- so FIT-R can only differ from the control where the data pay for it.

FIT-LS is the committed machinery's model (src/lora-physics-engine/model/ca-tbl-loocv.cc):
OLS of error in metres on (t1, t2, t3), no intercept. Reported, never decisive: with no
intercept the near-constant t3 has to carry the mean error level, which gives t3 a large
weight for a reason that has nothing to do with ranking. FIT-LS1 adds the intercept. The
machinery's own cycle-level LOOCV verdict is reproduced on the pooled set and reported as
descriptive only: cycles within a seed are serially correlated, so a cycle-level hold-out
leaks each held-out cycle's neighbours into its own fit.
"""
import argparse
import csv
import math
import os
import sys

# One BLAS thread: a multi-threaded reduction may sum in a different order from run to run,
# and another campaign shares this machine's cores.
for _v in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_v, "1")

import numpy as np  # noqa: E402 -- after the thread limit, which numpy reads at import

# ----------------------------------------------------------------- registered --

REGIMES = ["default", "arc", "searcher"]
ARMS = ["ca_tbl_full", "static_868"]
PRIMARY_ARM = "ca_tbl_full"
SEEDS = list(range(1, 21))
ROWS_PER_TRACE = 720          # --duration=1800 at dt = 2.5 s, counted (rule 12)
MIN_EVAL_CYCLES = 30          # a held-out seed with fewer fix cycles is not evaluated
MIN_EVAL_SEEDS = 10           # a cell with fewer evaluable seeds is NOT TESTABLE
NBOOT = 10000
BOOT_SEED = 20260929
ALPHA = 0.05
N_PRIMARY = 3                 # Bonferroni over the three primary cells
TRACE_REF = 10.0
HANDSET = np.array([2.0, 0.5, 0.5])
LEGACY = np.array([1.0, 1.0, 1.0])
T3_TOL = 1e-3                 # A3: back-derived t3 must lie in [-T3_TOL, 1 + T3_TOL]
JOIN_TOL = 5e-4               # A7: |back-derived t3 - recomputed t3| per fix cycle
JOIN_MAX_MISMATCH = 0.05      # A7: mismatch share above which a primary cell halts
ERR_TOL = 1e-3                # A8: |errM - hypot(est - true)| per fix cycle, metres
FED_MIN = 0.5                 # A4: share of fix cycles whose t3 was the live score
TIER_MAP = {"868MHz": "868", "2.4GHz": "2.4", "UWB": "UWB"}

COMMON_FLAGS = ["--arms=ca_tbl_full,static_868", "--duration=1800", "--trace=true",
                "--pcsForm=weighted", "--nlosClassifier=true", "--pcsNlosClassifier=true"]
ARC_FLAGS = ["--boxHalf=2000", "--nGateways=5", "--gwArcDeg=140", "--gwArcCentreDeg=180"]
FINDER_FLAGS = ["--nFinders=4", "--finderPseudoAnchor=true", "--finderMode=2",
                "--finderDispersal=1.0", "--sendHandshakeFrames=true",
                "--finderDigestPeriodS=20"]
REGIME_FLAGS = {"default": [], "arc": ARC_FLAGS, "searcher": ARC_FLAGS + FINDER_FLAGS}


# ----------------------------------------------------------------- statistics --

def avg_rank_last(v):
    """Average ranks (1-based, ties share their mean rank) along the LAST axis --
    scipy.stats.rankdata(method='average') applied row by row."""
    v = np.asarray(v, dtype=float)
    n = v.shape[-1]
    o = np.argsort(v, axis=-1, kind="mergesort")
    s = np.take_along_axis(v, o, axis=-1)
    idx = np.broadcast_to(np.arange(n), s.shape)
    new = np.ones(s.shape, dtype=bool)
    new[..., 1:] = s[..., 1:] != s[..., :-1]
    end = np.ones(s.shape, dtype=bool)
    end[..., :-1] = s[..., 1:] != s[..., :-1]
    first = np.maximum.accumulate(np.where(new, idx, 0), axis=-1)
    last = np.minimum.accumulate(np.where(end, idx, n)[..., ::-1], axis=-1)[..., ::-1]
    r = np.empty(s.shape)
    np.put_along_axis(r, o, (first + last) / 2.0 + 1.0, axis=-1)
    return r


def spearman(a, b):
    a = np.asarray(a, dtype=float)
    b = np.asarray(b, dtype=float)
    if a.shape[0] < 3:
        return float("nan")
    ra = avg_rank_last(a)
    rb = avg_rank_last(b)
    ra = ra - ra.mean()
    rb = rb - rb.mean()
    den = math.sqrt(float(ra @ ra) * float(rb @ rb))
    return float(ra @ rb) / den if den > 0 else 0.0


def scores(T, W):
    """(D, n) matrix of w . t for every direction. Elementwise, not a BLAS product, so two
    cycles with identical terms get bit-identical scores wherever they sit in the matrix
    and a tie stays a tie."""
    return (W[0][:, None] * T[0][None, :] + W[1][:, None] * T[1][None, :]
            + W[2][:, None] * T[2][None, :])


def rho_dirs(T, err, W, chunk=4096):
    """Spearman rho(w . t, error) for every column w of W. A score that is constant on the
    seed has no ranking; its rho is taken as 0."""
    ry = avg_rank_last(err)
    ry = ry - ry.mean()
    ryy = float(ry @ ry)
    out = np.empty(W.shape[1])
    for c0 in range(0, W.shape[1], chunk):
        rs = avg_rank_last(scores(T, W[:, c0:c0 + chunk]))
        rs -= rs.mean(axis=1, keepdims=True)
        den = np.sqrt((rs * rs).sum(axis=1) * ryy)
        with np.errstate(invalid="ignore", divide="ignore"):
            out[c0:c0 + chunk] = np.where(den > 0, (rs @ ry) / np.where(den > 0, den, 1), 0.0)
    return out


def build_grid():
    """The registered search grid, confidence convention (higher = more confident)."""
    mags = 10.0 ** (np.arange(-48, 49) / 8.0)
    V = np.concatenate([[0.0], mags, -mags])
    r2, r3 = np.meshgrid(V, V, indexing="ij")
    A = np.stack([np.ones(r2.size), r2.ravel(), r3.ravel()])
    phi = 2.0 * math.pi * np.arange(720) / 720.0
    B = np.stack([np.zeros(720), np.cos(phi), np.sin(phi)])
    return np.concatenate([A, -A, B, HANDSET[:, None]], axis=1)


GRID = build_grid()


def describe(w):
    if w[0] == 0.0:
        return "w1 = 0, (w2, w3) at %.1f deg" % math.degrees(math.atan2(w[2], w[1]))
    r2, r3 = w[1] / w[0], w[2] / w[0]
    return "%s(1, %.4g, %.4g)" % ("+" if w[0] > 0 else "-", r2, r3)


def norm3(w):
    s = float(np.sum(w))
    return w * (3.0 / s) if s > 1e-300 else None


def fmtw(w):
    return "(%s)" % ", ".join("%.4g" % x for x in w) if w is not None else "n/a (sum <= 0)"


def ols(X, y):
    A = X.T @ X
    if np.linalg.cond(A) > 1e14:
        return None
    return np.linalg.solve(A, X.T @ y)


def boot_ci(vals, idx):
    """Seed-level percentile bootstrap of the mean: (mean, lo95, hi95, loB, hiB), where
    B is the Bonferroni level 1 - ALPHA/N_PRIMARY. The same resample indices are used
    for every model in a cell, so the comparisons are paired."""
    v = np.asarray(vals, dtype=float)
    b = v[idx].mean(axis=1)
    aB = ALPHA / N_PRIMARY
    q = np.percentile(b, [100 * ALPHA / 2, 100 * (1 - ALPHA / 2), 100 * aB / 2,
                          100 * (1 - aB / 2)])
    return float(v.mean()), float(q[0]), float(q[1]), float(q[2]), float(q[3])


def verdict(lo, hi):
    if lo > 0:
        return "BEATS"
    if hi < 0:
        return "LOSES"
    return "INDISTINGUISHABLE"


# -------------------------------------------------------------------- loading --

def fnum(x):
    try:
        return float(x)
    except (TypeError, ValueError):
        return float("nan")


def read_run_id(path):
    inv = src = dirty = prof = None
    for line in open(path, errors="replace"):
        if line.startswith("build ") and "|" in line:
            inv = line.split("|", 1)[1].split()
        elif line.startswith("source "):
            p = line.split()
            src, dirty, prof = p[1], p[3], p[5]
    return inv, src, dirty, prof


def check_invocation(inv, rg, sd):
    """A1: the recorded invocation is the registered one, flag for flag."""
    if not inv:
        return "no invocation line"
    if not inv[0].endswith("/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"):
        return "binary %s" % inv[0]
    args = inv[1:]
    outs = [a for a in args if a.startswith("--outdir=")]
    rest = sorted(a for a in args if not a.startswith("--outdir="))
    want = sorted(COMMON_FLAGS + REGIME_FLAGS[rg] + ["--seed=%d" % sd])
    if rest != want:
        return "flags differ: extra %s, missing %s" % (sorted(set(rest) - set(want)),
                                                      sorted(set(want) - set(rest)))
    if len(outs) != 1 or not outs[0].rstrip("/").endswith(
            "/results/cal01_loocv_20260928/%s/seed_%d" % (rg, sd)):
        return "outdir %s" % outs
    return None


def load_seed(d, arm, sd, wts, problems, tag):
    """One arm at one seed. A3 and A8 are recorded in problems; A7 and A4 are returned."""
    tp = os.path.join(d, "trace_%s_seed%d.csv" % (arm, sd))
    rows = list(csv.DictReader(open(tp)))
    if len(rows) != ROWS_PER_TRACE:
        problems.append("A2 %s: %d trace rows, want %d" % (tag, len(rows), ROWS_PER_TRACE))
    fix = [r for r in rows if r["hadFix"].strip() == "1"]
    col = lambda k: np.array([fnum(r[k]) for r in fix])
    t, err, ts, gd, pcs = col("t"), col("errM"), col("traceSigma"), col("gdop"), col("pcs")
    nA, nN = col("nAnchors"), col("nNlos")
    ex, ey, tx, ty = col("estX"), col("estY"), col("trueX"), col("trueY")
    tier = [r["fixTier"].strip() for r in fix]
    t1 = TRACE_REF / (TRACE_REF + np.maximum(ts, 0.0))
    t2 = 1.0 / np.maximum(gd, 1.0)
    t3 = (pcs - wts[0] * t1 - wts[1] * t2) / wts[2]
    finite = np.isfinite(err) & np.isfinite(ts) & np.isfinite(gd) & np.isfinite(pcs)
    if not finite.all():
        problems.append("A3 %s: %d non-finite fix rows" % (tag, int((~finite).sum())))
    bad = (t3 < -T3_TOL) | (t3 > 1.0 + T3_TOL)
    if bad.any():
        problems.append("A3 %s: back-derived t3 outside [0,1] on %d of %d fix cycles (first"
                        " %.6f at t=%.1f)" % (tag, int(bad.sum()), len(t3), float(t3[bad][0]),
                                              float(t[bad][0])))
    derr = np.abs(err - np.hypot(ex - tx, ey - ty))
    if len(derr) and float(derr.max()) > ERR_TOL:
        problems.append("A8 %s: errM disagrees with hypot(est - true) by up to %.4g m on %d"
                        " fix cycles" % (tag, float(derr.max()), int((derr > ERR_TOL).sum())))
    s = {"t": t, "err": err, "ts": ts, "T": np.stack([t1, t2, t3]), "pcs": pcs,
         "nA": nA, "nN": nN, "tier": tier, "rows": len(rows), "n": len(fix)}
    s.update(unused_t3(d, arm, sd, s))
    s.update(fed_share(d, arm, sd))
    return s


def unused_t3(d, arm, sd, s):
    """A7, rule 3: t3 recomputed from nlos_links rows, which the back-derivation never
    read -- mean(1 - min(1, score)) over the fix tier's scored links at that instant, or
    the LOS fraction (nAnchors - nNlos)/nAnchors when none is scored, as the policy does."""
    p = os.path.join(d, "nlos_links_%s_seed%d.csv" % (arm, sd))
    if not os.path.isfile(p):
        return {"joinChecked": 0, "joinMismatch": -1, "joinScored": 0, "joinCount": 0}
    links = {}
    for r in csv.DictReader(open(p)):
        key = (int(round(fnum(r["t"]) * 1000.0)), r["tier"].strip())
        links.setdefault(key, []).append(fnum(r["score"]))
    mism = scored = countOk = 0
    t3 = s["T"][2]
    for i in range(s["n"]):
        got = links.get((int(round(s["t"][i] * 1000.0)), TIER_MAP.get(s["tier"][i], "?")), [])
        sc = [x for x in got if x >= 0.0]
        if len(got) == int(s["nA"][i]):
            countOk += 1
        if sc:
            scored += 1
            pred = sum(1.0 - min(1.0, x) for x in sc) / len(sc)
        else:
            pred = (s["nA"][i] - s["nN"][i]) / s["nA"][i] if s["nA"][i] > 0 else 0.0
        if abs(pred - t3[i]) > JOIN_TOL:
            mism += 1
    return {"joinChecked": s["n"], "joinMismatch": mism, "joinScored": scored,
            "joinCount": countOk}


def fed_share(d, arm, sd):
    p = os.path.join(d, "nlos_summary_%s_seed%d.csv" % (arm, sd))
    if os.path.isfile(p):
        for r in csv.DictReader(open(p)):
            if r["kind"] == "pcs":
                return {"fedFix": int(r["fixCycles"]), "fedFed": int(r["fedCycles"])}
    return {"fedFix": 0, "fedFed": 0}


def check_and_load(root, source):
    """A1 (provenance and invocation) and A2 (completeness, rows counted) for every run;
    A3 and A8 per trace. Returns data[regime][arm][seed] and the problem list."""
    problems = []
    data = {rg: {a: {} for a in ARMS} for rg in REGIMES}
    for rg in REGIMES:
        for sd in SEEDS:
            d = os.path.join(root, rg, "seed_%d" % sd)
            rid = os.path.join(d, "run_id.txt")
            if not os.path.isfile(rid):
                problems.append("A2 %s seed %d: no run_id.txt" % (rg, sd))
                continue
            inv, src, dirty, prof = read_run_id(rid)
            if src != source or dirty != "0" or prof != "optimized":
                problems.append("A1 %s seed %d: source %s dirty %s profile %s (want %s dirty 0"
                                " optimized)" % (rg, sd, src, dirty, prof, source))
            bad = check_invocation(inv, rg, sd)
            if bad:
                problems.append("A1 %s seed %d: %s" % (rg, sd, bad))
            summ = os.path.join(d, "arm_summary.csv")
            seen = []
            if os.path.isfile(summ):
                seen = [r["arm"] for r in csv.DictReader(open(summ))]
            if sorted(seen) != sorted(ARMS):
                problems.append("A2 %s seed %d: arm_summary.csv rows %s, want one per %s"
                                % (rg, sd, seen, ARMS))
            for arm in ARMS:
                tag = "%s/%s seed %d" % (rg, arm, sd)
                for f in ("trace", "nlos_links", "nlos_summary"):
                    if not os.path.isfile(os.path.join(d, "%s_%s_seed%d.csv" % (f, arm, sd))):
                        problems.append("A2 %s: %s file missing" % (tag, f))
                if os.path.isfile(os.path.join(d, "trace_%s_seed%d.csv" % (arm, sd))):
                    data[rg][arm][sd] = load_seed(d, arm, sd, HANDSET, problems, tag)
    return data, problems


# ------------------------------------------------------------------- analysis --

def analyse_cell(cell, cell_index):
    seeds = sorted(cell)
    o = {"seeds": len(seeds)}
    allT = np.concatenate([cell[s]["T"] for s in seeds], axis=1)
    ally = np.concatenate([cell[s]["err"] for s in seeds])
    o["fix"] = int(ally.shape[0])
    o["rows"] = int(sum(cell[s]["rows"] for s in seeds))
    tiers = {}
    for s in seeds:
        for x in cell[s]["tier"]:
            tiers[x] = tiers.get(x, 0) + 1
    o["tiers"] = tiers
    o["terms"] = [(float(allT[k].mean()), float(allT[k].std()), float(allT[k].min()),
                   float(allT[k].max())) for k in range(3)]
    o["errQ"] = [float(np.percentile(ally, q)) for q in (50, 90, 99)] if len(ally) else [0] * 3
    for k in ("joinChecked", "joinScored", "joinCount", "fedFix", "fedFed"):
        o[k] = int(sum(cell[s][k] for s in seeds))
    o["joinMismatch"] = int(sum(max(cell[s]["joinMismatch"], 0) for s in seeds))
    o["joinMissing"] = int(sum(1 for s in seeds if cell[s]["joinMismatch"] < 0))
    o["fedShare"] = o["fedFed"] / o["fedFix"] if o["fedFix"] else 0.0

    for k in range(3):
        if allT[k].max() - allT[k].min() < 1e-12:
            o["status"] = "DEGENERATE (term %d constant)" % (k + 1)
            return o
    if o["fedShare"] < FED_MIN:
        o["status"] = "NOT TESTABLE (A4: t3 fed on %.1f%% of fix cycles)" % (100 * o["fedShare"])
        return o
    ev = [s for s in seeds if cell[s]["n"] >= MIN_EVAL_CYCLES]
    o["evalSeeds"] = len(ev)
    if len(ev) < MIN_EVAL_SEEDS:
        o["status"] = "NOT TESTABLE (A6: %d evaluable seeds)" % len(ev)
        return o
    o["status"] = "OK"
    nev = len(ev)

    # The registered loss for every grid direction on every evaluable seed, once.
    R = np.array([rho_dirs(cell[s]["T"], cell[s]["err"], GRID) for s in ev])
    rc = np.array([spearman(-cell[s]["ts"], cell[s]["err"]) for s in ev])
    # A9: grid index 0 is (1, 0, 0); it must rank every seed exactly as the control does.
    o["a9"] = float(np.max(np.abs(R[:, 0] - rc)))

    per = {m: [] for m in ("FIT-R", "FIT-LS", "FIT-LS1", "HAND", "LEG")}
    ctlS, flips, chosen = [], 0, []
    sse = {"FIT-LS": 0.0, "t1": 0.0, "FIT-LS1": 0.0, "t1+c": 0.0}
    nres = 0
    XtX = {s: cell[s]["T"] @ cell[s]["T"].T for s in seeds}
    Xty = {s: cell[s]["T"] @ cell[s]["err"] for s in seeds}
    for s in seeds:
        tr = [q for q in seeds if q != s]
        w = np.linalg.solve(sum(XtX[q] for q in tr), sum(Xty[q] for q in tr))
        Xtr = np.concatenate([cell[q]["T"] for q in tr], axis=1).T
        ytr = np.concatenate([cell[q]["err"] for q in tr])
        c1 = float(Xtr[:, 0] @ ytr) / float(Xtr[:, 0] @ Xtr[:, 0])
        Xi = np.column_stack([np.ones(len(ytr)), Xtr])
        wi = ols(Xi, ytr)
        ci = ols(Xi[:, :2], ytr)
        Xs, ys = cell[s]["T"].T, cell[s]["err"]
        sse["FIT-LS"] += float(((ys - Xs @ w) ** 2).sum())
        sse["t1"] += float(((ys - c1 * Xs[:, 0]) ** 2).sum())
        if wi is not None and ci is not None:
            sse["FIT-LS1"] += float(((ys - wi[0] - Xs @ wi[1:]) ** 2).sum())
            sse["t1+c"] += float(((ys - ci[0] - ci[1] * Xs[:, 0]) ** 2).sum())
        nres += len(ys)
        if s not in ev:
            continue
        j = ev.index(s)
        trj = [k for k in range(nev) if k != j]
        b = int(np.argmin(R[trj].mean(axis=0)))
        chosen.append(b)
        per["FIT-R"].append(R[j, b])
        per["FIT-LS"].append(spearman(-(Xs @ w), ys))
        per["FIT-LS1"].append(spearman(-(Xs @ wi[1:]), ys) if wi is not None else 0.0)
        per["HAND"].append(spearman(Xs @ HANDSET, ys))
        per["LEG"].append(spearman(Xs @ LEGACY, ys))
        # The sign-refitted control: the training seeds decide which way the trace ranks
        # error, exactly as FIT-R's family is free to.
        sg = np.mean(rc[trj])
        if sg > 0:
            flips += 1
        ctlS.append(-rc[j] if sg > 0 else rc[j])
    o["flips"] = flips
    o["foldDistinct"] = len(set(chosen))
    o["foldControl"] = sum(1 for b in chosen if b == 0)
    o["rmse"] = {k: math.sqrt(v / nres) for k, v in sse.items()}
    o["rhoCtl"] = float(rc.mean())
    o["rhoCtlS"] = float(np.mean(ctlS))

    rng = np.random.default_rng(BOOT_SEED + cell_index)
    idx = rng.integers(0, nev, size=(NBOOT, nev))
    res = {}
    for m, v in per.items():
        v = np.array(v)
        A = boot_ci(rc - v, idx)            # vs the registered, fixed-sign control
        As = boot_ci(np.array(ctlS) - v, idx)  # vs the sign-refitted control
        vB = verdict(As[3], As[4])
        if vB == "BEATS" and not A[3] > 0:
            vB = "BEATS SIGN-REFITTED ONLY"
        res[m] = {"rho": float(v.mean()), "A": A, "As": As, "vB": vB,
                  "v95": verdict(As[1], As[2])}
    o["res"] = res
    o["rhoT2"] = float(np.mean([spearman(cell[s]["T"][1], cell[s]["err"]) for s in ev]))
    o["rhoT3"] = float(np.mean([spearman(cell[s]["T"][2], cell[s]["err"]) for s in ev]))

    # All-seed FIT-R, its seed-bootstrap spread, and how it ranks against the trace.
    ball = int(np.argmin(R.mean(axis=0)))
    wall = GRID[:, ball]
    o["fitR"] = wall
    o["fitRtrain"] = float(R.mean(axis=0)[ball])
    o["fitRvsT1"] = float(np.mean([spearman(wall @ cell[s]["T"], cell[s]["T"][0]) for s in ev]))
    bs = []
    for c0 in range(0, NBOOT, 250):
        cnt = np.zeros((min(250, NBOOT - c0), nev))
        for r, row in enumerate(idx[c0:c0 + 250]):
            cnt[r] = np.bincount(row, minlength=nev)
        bs.extend(np.argmin(cnt @ R, axis=1).tolist())
    Wb = GRID[:, bs]
    pos = Wb[0] > 0
    o["fitRboot"] = {"pos": int(pos.sum()), "neg": int((Wb[0] < 0).sum()),
                     "zero": int((Wb[0] == 0).sum()), "ctl": int(sum(1 for b in bs if b == 0))}
    if pos.any():
        r2 = Wb[1, pos] / Wb[0, pos]
        r3 = Wb[2, pos] / Wb[0, pos]
        o["fitRratioCI"] = [(float(np.percentile(x, 2.5)), float(np.percentile(x, 97.5)))
                            for x in (r2, r3)]

    # FIT-LS / FIT-LS1 on all seeds, with seed-bootstrap CIs (confidence convention).
    wls = np.linalg.solve(sum(XtX.values()), sum(Xty.values()))
    o["fitLS"] = -wls
    o["fitLSmach"] = wls * (3.0 / wls.sum()) if abs(wls.sum()) > 1e-12 else wls
    bl = []
    for row in rng.integers(0, len(seeds), size=(NBOOT, len(seeds))):
        pick = [seeds[i] for i in row]
        try:
            bl.append(-np.linalg.solve(sum(XtX[q] for q in pick), sum(Xty[q] for q in pick)))
        except np.linalg.LinAlgError:
            pass
    bl = np.array(bl)
    o["fitLSci"] = [(float(np.percentile(bl[:, k], 2.5)), float(np.percentile(bl[:, k], 97.5)))
                    for k in range(3)]
    Xall = np.column_stack([np.ones(len(ally)), allT.T])
    wli = ols(Xall, ally)
    o["fitLS1"] = -wli[1:] if wli is not None else None

    # The committed machinery's own answer on the pooled set: cycle-level LOO, as
    # CaTblCalibrateLoocv computes it (descriptive only).
    X = allT.T
    Ainv = np.linalg.inv(X.T @ X)
    h = np.einsum("ij,jk,ik->i", X, Ainv, X)
    e = ally - X @ (Ainv @ (X.T @ ally))
    loo = math.sqrt(float(np.mean((e / (1.0 - h)) ** 2)))
    t1 = X[:, 0]
    h1 = t1 * t1 / float(t1 @ t1)
    e1 = ally - t1 * (float(t1 @ ally) / float(t1 @ t1))
    loo1 = math.sqrt(float(np.mean((e1 / (1.0 - h1)) ** 2)))
    o["mach"] = (loo, loo1, "OK" if loo < loo1 else "NO_IMPROVEMENT")
    return o


def final_verdict(results, halt):
    prim = [results[(rg, PRIMARY_ARM)] for rg in REGIMES]
    cv = [o["res"]["FIT-R"]["vB"] if o["status"] == "OK" else "NOT TESTABLE" for o in prim]
    beats = sum(1 for v in cv if v == "BEATS")
    loses = sum(1 for v in cv if v == "LOSES")
    testable = sum(1 for v in cv if v != "NOT TESTABLE")
    if halt:
        return cv, "HALTED: an abort condition fired; no verdict is citable"
    if testable < 2:
        return cv, "NOT TESTABLE: fewer than two primary cells are testable"
    if beats >= 2 and loses == 0:
        return cv, "CALIBRATED PCS INFORMATIVE"
    if beats >= 1:
        return cv, "REGIME-DEPENDENT"
    return cv, "CALIBRATION DOES NOT RESCUE PCS"


# --------------------------------------------------------------------- report --

def ci(t, lo, hi):
    return "%+.4f [%+.4f, %+.4f]" % (t[0], t[lo], t[hi])


def report(results, problems, root, source):
    fatal = [p for p in problems if p.split()[0] in ("A1", "A2", "A3", "A8")]
    print("# CAL-01 analysis, applying report/PREREG_CAL01_LOOCV.md\n")
    print("`python3 scripts/cal01_loocv.py --root %s --source %s`\n" % (root, source))
    print("%d regimes x %d seeds x %d arms. Bootstrap: %d seed-level resamples, seed %d + cell"
          " index. Primary CI level %.4f (Bonferroni over %d primary cells).\n"
          % (len(REGIMES), len(SEEDS), len(ARMS), NBOOT, BOOT_SEED, 1 - ALPHA / N_PRIMARY,
             N_PRIMARY))
    print("## 0. Abort conditions\n")
    if fatal:
        for p in fatal:
            print("- " + p)
        print("\nA1/A2/A3/A8 fired. No verdict is computed.")
        return 2
    print("A1 (provenance and invocation), A2 (completeness, rows counted), A3 (t3 range),"
          " A8 (errM against hypot(est - true)): none fired. %d traces x %d rows.\n"
          % (len(REGIMES) * len(SEEDS) * len(ARMS), ROWS_PER_TRACE))

    print("## 1. Populations (descriptive)\n")
    print("Population: fix cycles (`hadFix == 1`) of the measured tag; window t = 2.5 ..."
          " 1800 s of each run; each cycle's error and terms are instantaneous, so the window"
          " truncates nothing.\n")
    print("| regime | arm | fix cycles / rows | fix tier | t3 fed (A4) | err p50 / p90 / p99 m |")
    print("|---|---|---|---|---|---|")
    for (rg, arm), o in results.items():
        print("| %s | %s | %d / %d | %s | %d / %d = %.3f | %.1f / %.1f / %.1f |"
              % (rg, arm, o["fix"], o["rows"],
                 ", ".join("%s %d" % kv for kv in sorted(o["tiers"].items())),
                 o["fedFed"], o["fedFix"], o["fedShare"], *o["errQ"]))
    print("\n| regime | arm | t1 mean / sd [min, max] | t2 mean / sd [min, max] |"
          " t3 mean / sd [min, max] |")
    print("|---|---|---|---|---|")
    for (rg, arm), o in results.items():
        print("| %s | %s | %s |" % (rg, arm, " | ".join(
            "%.4g / %.4g [%.4g, %.4g]" % t for t in o["terms"])))

    print("\n## 2. Rule 3: t3 against a quantity its derivation never read (A7), and A9\n")
    print("| regime | arm | fix cycles | t3 mismatches > %.0e | share | fix-set size matches"
          " nlos_links | cycles with a scored link | A9 max abs(rho(t1-only) - rho_ctl) |"
          % JOIN_TOL)
    print("|---|---|---|---|---|---|---|---|")
    halt = False
    for (rg, arm), o in results.items():
        share = o["joinMismatch"] / o["joinChecked"] if o["joinChecked"] else 1.0
        a9 = o.get("a9", float("nan"))
        if arm == PRIMARY_ARM and (share > JOIN_MAX_MISMATCH or o["joinMissing"]
                                   or (o["status"] == "OK" and a9 > 1e-12)):
            halt = True
        print("| %s | %s | %d | %d | %.4f | %d | %d | %.1e |" % (
            rg, arm, o["joinChecked"], o["joinMismatch"], share, o["joinCount"],
            o["joinScored"], a9))
    if halt:
        print("\n**A7 or A9 fired on a primary cell.**")

    print("\n## 3. The registered test: FIT-R against the control, leave-one-seed-out\n")
    print("Per held-out seed, A = rho_control - rho_model (positive = the calibrated score ranks"
          " error better). Mean over evaluable seeds, seed-bootstrap CI. `As` is against the"
          " sign-refitted control; `A` against the registered fixed-sign rho(-traceSigma)."
          " The verdict is read from the Bonferroni CI of As, and BEATS also requires A's"
          " Bonferroni CI above zero.\n")
    print("| regime | arm | status | eval seeds | rho_ctl | rho_ctl (sign-refitted) | rho FIT-R |"
          " As, 95%% CI | As, %.2f%% CI | A, %.2f%% CI | verdict |"
          % (100 * (1 - ALPHA / N_PRIMARY), 100 * (1 - ALPHA / N_PRIMARY)))
    print("|---|---|---|---|---|---|---|---|---|---|---|")
    for (rg, arm), o in results.items():
        if o["status"] != "OK":
            print("| %s | %s | %s | | | | | | | | NOT TESTABLE |" % (rg, arm, o["status"]))
            continue
        r = o["res"]["FIT-R"]
        print("| %s | %s | OK | %d | %.4f | %.4f | %.4f | %s | %s | %s | %s |" % (
            rg, arm, o["evalSeeds"], o["rhoCtl"], o["rhoCtlS"], r["rho"], ci(r["As"], 1, 2),
            ci(r["As"], 3, 4), ci(r["A"], 3, 4), r["vB"]))
    print("\nFolds whose training seeds' mean rho(-traceSigma, error) was POSITIVE (the"
          " sign-refitted control flipped): " + ", ".join(
              "%s/%s %d" % (rg, arm, o.get("flips", 0)) for (rg, arm), o in results.items()))
    print("\nFIT-R fold choices: distinct directions / folds that chose the control itself: "
          + ", ".join("%s/%s %s/%s" % (rg, arm, o.get("foldDistinct", "-"),
                                       o.get("foldControl", "-"))
                      for (rg, arm), o in results.items()))

    print("\n## 4. Every model, every cell (descriptive except FIT-R on the primary arm)\n")
    print("| regime | arm | model | mean rho(score, error) | As, 95% CI | verdict at 95% |")
    print("|---|---|---|---|---|---|")
    for (rg, arm), o in results.items():
        if o["status"] != "OK":
            continue
        for m in ("FIT-R", "FIT-LS", "FIT-LS1", "HAND", "LEG"):
            r = o["res"][m]
            print("| %s | %s | %s | %.4f | %s | %s |" % (rg, arm, m, r["rho"], ci(r["As"], 1, 2),
                                                         r["v95"]))
    print("\nSingle terms, mean per-seed rho with error (confidence convention): " + "; ".join(
        "%s/%s t2 %.4f, t3 %.4f" % (rg, arm, o["rhoT2"], o["rhoT3"])
        for (rg, arm), o in results.items() if o["status"] == "OK"))

    print("\n## 5. Fitted weights on all 20 seeds (confidence convention: higher = more"
          " confident)\n")
    for (rg, arm), o in results.items():
        if o["status"] != "OK":
            continue
        bt = o["fitRboot"]
        rci = o.get("fitRratioCI")
        print("- **%s / %s**. FIT-R %s, normalised to sum 3: %s; training mean rho %.4f; mean"
              " per-seed rank agreement with t1 (i.e. with the trace) %.4f. Bootstrap (%d):"
              " w1 > 0 in %d, w1 < 0 in %d, w1 = 0 in %d, the control itself in %d%s."
              % (rg, arm, describe(o["fitR"]), fmtw(norm3(o["fitR"])), o["fitRtrain"],
                 o["fitRvsT1"], NBOOT, bt["pos"], bt["neg"], bt["zero"], bt["ctl"],
                 ("; among w1 > 0, w2/w1 95%% CI [%.4g, %.4g], w3/w1 [%.4g, %.4g]"
                  % (rci[0][0], rci[0][1], rci[1][0], rci[1][1])) if rci else ""))
        print("  FIT-LS (machinery model, sign flipped to confidence) %s, 95%% CI %s; the"
              " machinery's own sum-3 normalisation of the error-convention fit %s. FIT-LS1"
              " (with intercept) %s."
              % (fmtw(o["fitLS"]), "[" + "; ".join("%.4g, %.4g" % c for c in o["fitLSci"]) + "]",
                 fmtw(o["fitLSmach"]), fmtw(o["fitLS1"])))

    print("\n## 6. Secondary: RMSE, the machinery's own loss\n")
    print("| regime | arm | seed-LOO RMSE FIT-LS | t1-only | FIT-LS1 | t1 + intercept |"
          " machinery, cycle-level LOO: fit / t1-only / verdict |")
    print("|---|---|---|---|---|---|---|")
    for (rg, arm), o in results.items():
        if o["status"] != "OK":
            continue
        r = o["rmse"]
        print("| %s | %s | %.2f | %.2f | %.2f | %.2f | %.2f / %.2f / %s |" % (
            rg, arm, r["FIT-LS"], r["t1"], r["FIT-LS1"], r["t1+c"], *o["mach"]))

    print("\n## 7. The registered decision rule, applied\n")
    cv, final = final_verdict(results, halt)
    for rg, v in zip(REGIMES, cv):
        print("- %s / %s: FIT-R %s" % (rg, PRIMARY_ARM, v))
    print("\n**VERDICT: %s**" % final)
    return 0


def run(root, source):
    data, problems = check_and_load(root, source)
    results = {}
    k = 0
    for rg in REGIMES:
        for arm in ARMS:
            results[(rg, arm)] = analyse_cell(data[rg][arm], k)
            k += 1
    return report(results, problems, root, source)


# ------------------------------------------------------------------ self-test --

def synth_cell(kind, rng, n=400):
    """Seeds of iid synthetic cycles. 'null': error depends on the trace alone. 'pos':
    geometry carries error beyond the trace. 'sign': the trace ranks error upside down."""
    cell = {}
    for s in SEEDS:
        ts = np.exp(rng.normal(8.0, 1.5, n))
        gd = 1.0 + rng.gamma(2.0, 1.0, n)
        t3 = 1.0 - rng.beta(1.0, 8.0, n)
        z = rng.normal(0.0, 1.0, n)
        if kind == "null":
            err = np.sqrt(ts) * np.exp(0.6 * z)
        elif kind == "pos":
            err = np.sqrt(ts) * gd * np.exp(0.6 * z)
        else:
            err = 1e4 / np.sqrt(ts) * np.exp(0.6 * z)
        T = np.stack([TRACE_REF / (TRACE_REF + ts), 1.0 / gd, t3])
        cell[s] = {"T": T, "err": err, "ts": ts, "tier": ["868MHz"] * n, "rows": n, "n": n,
                   "joinChecked": n, "joinMismatch": 0, "joinScored": n, "joinCount": n,
                   "fedFix": n, "fedFed": n}
    return cell


def selftest():
    ok = True
    v = np.array([3.0, 1.0, 4.0, 1.0, 5.0, 9.0, 2.0, 6.0, 5.0])
    naive = np.array([sum(1.0 for y in v if y < x) + (sum(1.0 for y in v if y == x) + 1) / 2
                      for x in v])
    if not np.allclose(avg_rank_last(v), naive):
        print("FAIL avg_rank_last on ties")
        ok = False
    if abs(GRID[0, 0] - 1) + abs(GRID[1, 0]) + abs(GRID[2, 0]) != 0:
        print("FAIL grid index 0 is not (1, 0, 0)")
        ok = False
    rng = np.random.default_rng(7)
    expect = {"null": lambda x: x != "BEATS", "pos": lambda x: x == "BEATS",
              "sign": lambda x: x != "BEATS"}
    for i, kind in enumerate(("null", "pos", "sign")):
        o = analyse_cell(synth_cell(kind, rng), 100 + i)
        r = o["res"]["FIT-R"]
        good = o["status"] == "OK" and expect[kind](r["vB"]) and o["a9"] < 1e-12
        ok = ok and good
        print("%s %-4s: FIT-R %s, As %s (Bonferroni), A %s, rho_ctl %.3f, fit %s, flips %d, "
              "A9 %.1e" % ("PASS" if good else "FAIL", kind, r["vB"], ci(r["As"], 3, 4),
                           ci(r["A"], 3, 4), o["rhoCtl"], describe(o["fitR"]), o["flips"],
                           o["a9"]))
    print("SELFTEST %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--root")
    ap.add_argument("--source", help="short SHA every run_id.txt must cite")
    ap.add_argument("--selftest", action="store_true")
    a = ap.parse_args()
    if a.selftest:
        return selftest()
    if not a.root or not a.source:
        ap.error("--root and --source are required")
    return run(a.root, a.source)


if __name__ == "__main__":
    sys.exit(main())
