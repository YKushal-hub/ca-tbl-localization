#!/usr/bin/env python3
"""CAL-03 analysis (report/PREREG_CAL03.md).

usage: cal03_analyse.py fit  <outdir>   -> <outdir>/fit.json, <outdir>/FIT.md
       cal03_analyse.py eval <outdir>   -> <outdir>/EVAL.md

fit: weights by CAL-01's FIT-R on the preset's fit seeds (1-20, 'main'), scored on held-out
seeds 21-40 against the trace control; per-tier thresholds by F1 against the registered
labels, on the preset POOLED with its endgame variant (fit seeds 1-20 of 'main' and 'end'),
scored by leave-one-seed-out and on held-out seeds 21-40.
eval: the calibrated preset (eval/) against the legacy preset and fixed_threshold (gen/), on
the held-out seeds 21-40, per configuration.
"""
import csv
import json
import math
import os
import sys

for _v in ("OPENBLAS_NUM_THREADS", "OMP_NUM_THREADS", "MKL_NUM_THREADS"):
    os.environ.setdefault(_v, "1")
import numpy as np  # noqa: E402

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import cal01_loocv as c1  # noqa: E402 -- the registered FIT-R machinery, reused unchanged

FIT_SEEDS = list(range(1, 21))
HELD_SEEDS = list(range(21, 41))
CONFIGS = ["main", "end"]
SUSPECT_FACTOR = 0.5           # DC-7: a suspect fix's PCS is halved under downweight
T3_TOL = 1e-3
T3_MAX_BAD = 0.01              # abort if back-derived t3 is out of range on > 1 % of fix cycles
MIN_POS = 30                   # a threshold is fitted only with >= 30 positive and negative cycles
MIN_CYC = 30                   # a tier with fewer pooled cycles is UNEXERCISED
NBOOT = 10000
BOOT_SEED = 20261003
LEGACY_THR = {"pcsThrUp868": 1.55, "pcsThrUp2p4": 1.70, "pcsThrUpUwb": 2.30,
              "pcsThrDown2p4": 1.90, "pcsThrDownUwb": 2.55}
# threshold -> (tier, label, fires when): the registered definitions (prereg sect 4)
THR = {
    "pcsThrUp868": ("868MHz", lambda r: int(r["vis2p4"]) >= 3, "below"),
    "pcsThrUp2p4": ("2.4GHz", lambda r: r["shouldUwb"] == "1", "below"),
    "pcsThrUpUwb": ("UWB", lambda r: r["shouldUwb"] == "1", "below"),
    "pcsThrDown2p4": ("2.4GHz", lambda r: int(r["vis868"]) >= 3, "above"),
    "pcsThrDownUwb": ("UWB", lambda r: int(r["vis2p4"]) >= 3, "above"),
}
HYST = 1.12                    # a Down below its tier's Up is raised to Up x 1.12 (legacy ratio)


def load(root, phase, cfg, seed, arm="ca_tbl_full"):
    p = os.path.join(root, phase, cfg, f"s{seed}", f"trace_{arm}_seed{seed}.csv")
    allfix = [r for r in csv.DictReader(open(p)) if r["hadFix"] == "1"]
    # A fix with no geometry computed (two ranges) has gdop 0 and PCS 0 under every form,
    # so it has no terms to decompose: excluded and counted (prereg sect 3).
    rows = [r for r in allfix if float(r["gdop"]) != 0.0]
    ts = np.array([max(float(r["traceSigma"]), 0.0) for r in rows])
    t1 = c1.TRACE_REF / (c1.TRACE_REF + ts)
    t2 = 1.0 / np.maximum(np.array([float(r["gdop"]) for r in rows]), 1.0)
    sus = np.array([r["suspect"] == "1" for r in rows])
    pcs = np.array([float(r["pcs"]) for r in rows])
    raw = np.where(sus, pcs / SUSPECT_FACTOR, pcs)
    t3 = raw - t1 - t2
    return dict(rows=rows, T=np.stack([t1, t2, t3]), err=np.array([float(r["errM"]) for r in rows]),
                ts=ts, sus=sus, pcs=pcs, n=len(rows), nogeo=len(allfix) - len(rows))


def boot_ci(v, rng):
    v = np.asarray(v)
    idx = rng.integers(0, len(v), size=(NBOOT, len(v)))
    m = v[idx].mean(axis=1)
    return float(np.quantile(m, 0.025)), float(np.quantile(m, 0.975))


def f1(fire, lab):
    tp = int(np.sum(fire & lab)); fp = int(np.sum(fire & ~lab)); fn = int(np.sum(~fire & lab))
    return (2 * tp / (2 * tp + fp + fn)) if (2 * tp + fp + fn) > 0 else float("nan")


def tier_data(D, seeds, cfgs, tier, label, w):
    s, lab, leg, sd = [], [], [], []
    for c in cfgs:
        for k in seeds:
            d = D[(c, k)]
            sc = (w[0] * d["T"][0] + w[1] * d["T"][1] + w[2] * d["T"][2]) * np.where(d["sus"], SUSPECT_FACTOR, 1.0)
            for i, r in enumerate(d["rows"]):
                if r["activeTier"] == tier:
                    s.append(sc[i]); lab.append(label(r)); leg.append(d["pcs"][i]); sd.append(k)
    return np.array(s), np.array(lab, dtype=bool), np.array(leg), np.array(sd)


def fit_thr(s, lab, mode):
    cand = np.unique(np.quantile(s, np.linspace(0, 1, 1001)))
    cand = np.concatenate([[cand[0] - 1e-9], cand, [cand[-1] + 1e-9]])
    best, bt = -1.0, None
    for c in cand:
        fire = (s < c) if mode == "below" else (s > c)
        v = f1(fire, lab)
        v = -1.0 if math.isnan(v) else v
        # ties: more escalation for Up (higher c), fewer demotions for Down (higher c)
        if v > best or (v == best and bt is not None and c > bt):
            best, bt = v, c
    return float(bt), best


def fit(root):
    D, problems = {}, []
    for c in CONFIGS:
        for k in FIT_SEEDS + HELD_SEEDS:
            rc = open(os.path.join(root, "gen", c, f"s{k}", "rc")).read().strip()
            if rc != "0":
                problems.append(f"gen/{c}/s{k} exit {rc}")
                continue
            D[(c, k)] = load(root, "gen", c, k)
    if problems:
        print("ABORT:", problems)
        return 1
    for c in CONFIGS:
        allt3 = np.concatenate([D[(c, k)]["T"][2] for k in FIT_SEEDS + HELD_SEEDS])
        bad = float(np.mean((allt3 < -T3_TOL) | (allt3 > 1 + T3_TOL)))
        if bad > T3_MAX_BAD:
            print(f"ABORT: back-derived t3 out of range on {bad:.3%} of {c} fix cycles")
            return 1
    # --- weights: FIT-R on main fit seeds
    rho = np.mean([c1.rho_dirs(D[("main", k)]["T"], D[("main", k)]["err"], c1.GRID) for k in FIT_SEEDS], axis=0)
    wdir = c1.GRID[:, int(np.argmin(rho))]
    wc = np.maximum(wdir, 0.0)
    clamped = [i for i in range(3) if wdir[i] < 0]
    w = c1.norm3(wc) if np.sum(wc) > 0 else np.array([3.0, 0.0, 0.0])
    rng = np.random.default_rng(BOOT_SEED)
    A, rf, rcn = [], [], []
    for k in HELD_SEEDS:
        d = D[("main", k)]
        sc = w[0] * d["T"][0] + w[1] * d["T"][1] + w[2] * d["T"][2]
        r_fit = c1.spearman(sc, d["err"])
        r_ctl = c1.spearman(-d["ts"], d["err"])
        rf.append(r_fit); rcn.append(r_ctl); A.append(r_ctl - r_fit)
    lo, hi = boot_ci(A, rng)
    verdict = "BETTER" if lo > 0 else ("WORSE" if hi < 0 else "INDISTINGUISHABLE")
    agree = float(np.mean([c1.spearman(w[0] * D[("main", k)]["T"][0] + w[1] * D[("main", k)]["T"][1] + w[2] * D[("main", k)]["T"][2],
                                       D[("main", k)]["T"][0]) for k in FIT_SEEDS]))
    # --- thresholds: pooled main + end
    thr, rows = {}, []
    for name, (tier, label, mode) in THR.items():
        s, lab, leg, sd = tier_data(D, FIT_SEEDS, CONFIGS, tier, label, w)
        npos, nneg = int(lab.sum()), int((~lab).sum())
        if len(s) < MIN_CYC:
            # UNEXERCISED: the legacy value rescaled by the scores' median ratio over all fix cycles
            alls = np.concatenate([(w[0] * D[(c, k)]["T"][0] + w[1] * D[(c, k)]["T"][1] + w[2] * D[(c, k)]["T"][2]) for c in CONFIGS for k in FIT_SEEDS])
            allp = np.concatenate([D[(c, k)]["pcs"] for c in CONFIGS for k in FIT_SEEDS])
            t = LEGACY_THR[name] * float(np.median(alls)) / float(np.median(allp))
            thr[name] = t
            rows.append((name, tier, len(s), npos, nneg, "UNEXERCISED", t, float("nan"), float("nan")))
            continue
        if npos < MIN_POS or nneg < MIN_POS:
            legfire = (leg < LEGACY_THR[name]) if mode == "below" else (leg > LEGACY_THR[name])
            q = float(np.mean(legfire))
            t = float(np.quantile(s, q if mode == "below" else 1.0 - q))
            thr[name] = t
            rows.append((name, tier, len(s), npos, nneg, f"UNFITTED (legacy rate {q:.3f})", t, float("nan"), float("nan")))
            continue
        t, fbest = fit_thr(s, lab, mode)
        loso = []
        for k in FIT_SEEDS:
            m = sd != k
            if m.sum() == 0 or (~m).sum() == 0:
                continue
            tk, _ = fit_thr(s[m], lab[m], mode)
            fire = (s[~m] < tk) if mode == "below" else (s[~m] > tk)
            v = f1(fire, lab[~m])
            if not math.isnan(v):
                loso.append(v)
        sh, lh, _, _ = tier_data(D, HELD_SEEDS, CONFIGS, tier, label, w)
        held = f1((sh < t) if mode == "below" else (sh > t), lh) if len(sh) else float("nan")
        thr[name] = t
        rows.append((name, tier, len(s), npos, nneg, "FITTED", t, float(np.mean(loso)) if loso else float("nan"), held))
    raised = []
    for up, dn in (("pcsThrUp2p4", "pcsThrDown2p4"), ("pcsThrUpUwb", "pcsThrDownUwb")):
        if thr[dn] < thr[up]:
            raised.append((dn, thr[dn], thr[up] * HYST))
            thr[dn] = thr[up] * HYST
    flags = {"pcsW1": round(float(w[0]), 6), "pcsW2": round(float(w[1]), 6), "pcsW3": round(float(w[2]), 6)}
    flags.update({k: round(float(v), 6) for k, v in thr.items()})
    out = dict(weights=[float(x) for x in w], direction=[float(x) for x in wdir], clamped=clamped,
               heldA=dict(mean=float(np.mean(A)), lo=lo, hi=hi, verdict=verdict,
                          rhoFit=float(np.mean(rf)), rhoControl=float(np.mean(rcn))),
               rankAgreementWithT1=agree, thresholds=rows, raised=raised, flags=flags)
    json.dump(out, open(os.path.join(root, "fit.json"), "w"), indent=1)
    nogeo = sum(d["nogeo"] for d in D.values())
    nfix = sum(d["n"] + d["nogeo"] for d in D.values())
    md = ["# CAL-03 fit (scripts/cal03_analyse.py fit)", "",
          f"Fix cycles read: {nfix}; excluded with no geometry (gdop 0, PCS 0): {nogeo}.", "",
          f"FIT-R direction {c1.describe(wdir)}; clamped to >= 0 on {clamped or 'none'}; weights (sum 3) "
          f"({w[0]:.4f}, {w[1]:.4f}, {w[2]:.4f}); rank agreement with t1 on the fit seeds {agree:.4f}.", "",
          f"Held-out seeds 21-40 (main): rho FIT {np.mean(rf):.4f}, rho control {np.mean(rcn):.4f}; "
          f"A = control - FIT = {np.mean(A):+.4f} [{lo:+.4f}, {hi:+.4f}] (95 % seed bootstrap) -> {verdict}.", "",
          "| threshold | tier | pooled fit cycles | positive | negative | status | value | LOSO F1 | held-out F1 |",
          "|---|---|---|---|---|---|---|---|---|"]
    for r in rows:
        md.append(f"| {r[0]} | {r[1]} | {r[2]} | {r[3]} | {r[4]} | {r[5]} | {r[6]:.4f} | {r[7]:.3f} | {r[8]:.3f} |")
    md += ["", f"Down thresholds raised to keep hysteresis: {raised or 'none'}.", "",
           "Flags for the calibrated preset: " + " ".join(f"--{k}={v}" for k, v in flags.items())]
    open(os.path.join(root, "FIT.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


def summ(root, phase, cfg, seed, arm):
    p = os.path.join(root, phase, cfg, f"s{seed}")
    a = next(r for r in csv.DictReader(open(os.path.join(p, "arm_summary.csv"))) if r["arm"] == arm)
    ss = [r for r in csv.DictReader(open(os.path.join(p, "subject_summary.csv"))) if r["arm"] == arm]
    steps = float(a["steps"])
    occ = [float(a[k]) / steps for k in ("cycles_868", "cycles_2p4", "cycles_uwb")]
    return dict(avail=float(np.mean([float(r["fixCycles"]) / float(r["steps"]) for r in ss])),
                cep50=float(a["cep50M"]), cep95=float(a["cep95M"]), esc=float(a["escalations"]),
                occ=occ, J=float(np.mean([float(r["totalJ"]) for r in ss])))


def evaluate(root):
    md = ["# CAL-03 evaluation on held-out seeds 21-40 (scripts/cal03_analyse.py eval)", ""]
    pred4 = []
    for cfg in CONFIGS:
        rows = {}
        for name, phase, arm in (("calibrated", "eval", "ca_tbl_full"), ("legacy", "gen", "ca_tbl_full"),
                                 ("fixed_threshold", "gen", "fixed_threshold")):
            rows[name] = [summ(root, phase, cfg, k, arm) for k in HELD_SEEDS]
        md += [f"## {cfg}", "", "| arm | availability | CEP50 (m) | CEP95 (m) | escalations | occupancy 868 / 2.4 / UWB | subject J |",
               "|---|---|---|---|---|---|---|"]
        for name, rs in rows.items():
            occ = np.mean([r["occ"] for r in rs], axis=0)
            md.append(f"| {name} | {np.mean([r['avail'] for r in rs]):.3f} | {np.mean([r['cep50'] for r in rs]):.1f} | "
                      f"{np.mean([r['cep95'] for r in rs]):.1f} | {np.mean([r['esc'] for r in rs]):.1f} | "
                      f"{occ[0]:.3f} / {occ[1]:.3f} / {occ[2]:.3f} | {np.mean([r['J'] for r in rs]):.2f} |")
        dcal = np.mean([np.abs(np.array(a["occ"]) - np.array(b["occ"])).sum() for a, b in zip(rows["calibrated"], rows["fixed_threshold"])])
        dleg = np.mean([np.abs(np.array(a["occ"]) - np.array(b["occ"])).sum() for a, b in zip(rows["legacy"], rows["fixed_threshold"])])
        pred4.append((cfg, dcal, dleg))
        md += ["", f"Mean |occupancy difference| to fixed_threshold: calibrated {dcal:.3f}, legacy {dleg:.3f}.", ""]
    open(os.path.join(root, "EVAL.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 0


if __name__ == "__main__":
    mode, root = sys.argv[1], sys.argv[2]
    sys.exit(fit(root) if mode == "fit" else evaluate(root))
