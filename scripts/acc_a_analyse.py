#!/usr/bin/env python3
"""ACC-A analysis (report/PREREG_ACC_A_FILTER.md): what --imuQuScale does to every arm and to every
consumer of P, the integrity-trigger ablation, the fair policy comparison at corrected covariance,
and the energy decomposition.

usage: acc_a_analyse.py <outdir written by scripts/acc_a_run.sh>

Writes <outdir>/ACC_A.md and <outdir>/acc_a_table.csv, prints the same, and exits 1 if any
registered abort condition (PREREG sect 6) holds.
"""
import csv
import glob
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import numpy as np  # noqa: E402

ARMS = ("ca_tbl_full", "fixed_threshold", "ca_tbl_no_integrity", "static_2p4", "static_868",
        "lora_fusion", "poucet_fusion")
CONFIGS = {"A": "5 km", "B": "endgame"}
NCELLS = 240
WARMUP_S = 120.0
CHI2_2_95, CHI2_2_99, SUSPECT_CHI2 = 5.991, 9.21, 1992.9


def rows(p):
    try:
        return list(csv.DictReader(open(p)))
    except OSError:
        return None


def spearman(a, b):
    if len(a) < 5:
        return float("nan")
    ra = np.argsort(np.argsort(a)).astype(float)
    rb = np.argsort(np.argsort(b)).astype(float)
    if ra.std() == 0 or rb.std() == 0:
        return float("nan")
    return float(np.corrcoef(ra, rb)[0, 1])


def n26(d, arm):
    """(TDoA refused, TDoA fused) from the run log's N.26 line for <arm>."""
    try:
        t = open(os.path.join(d, "run.log")).read()
    except OSError:
        return None
    ln = next((x for x in t.splitlines() if f"N.26 fixes ({arm})" in x), None)
    if ln is None:
        return None
    return (int(ln.split("TDoA innovation gate refused ")[1].split(",")[0]),
            int(ln.split("TDoA fused ")[1].split(";")[0]))


def seed_metrics(d, arm):
    s = int(d.rsplit("s", 1)[1])
    a = cs.arm_row(d, arm)
    ss = cs.subj_rows(d, arm)
    t0 = rows(os.path.join(d, f"trace_{arm}_seed{s}.csv"))
    if a is None or not ss or t0 is None:
        return None
    err0 = np.array([float(r["errM"]) for r in t0])
    w = [float(np.mean(err0 <= 25.0))]
    for k in range(2, 6):
        st = rows(os.path.join(d, f"subjtrace_{arm}_seed{s}_subj{k}.csv"))
        if st:
            w.append(float(np.mean([float(r["errM"]) <= 25.0 for r in st])))
    fx = [r for r in t0 if r["hadFix"] == "1" and float(r["t"]) >= WARMUP_S]
    nees = []
    for r in fx:
        if "pxx" not in r:
            break
        pxx, pxy, pyy = float(r["pxx"]), float(r["pxy"]), float(r["pyy"])
        det = pxx * pyy - pxy * pxy
        if det > 0:
            ex, ey = float(r["estX"]) - float(r["trueX"]), float(r["estY"]) - float(r["trueY"])
            nees.append((pyy * ex * ex - 2 * pxy * ex * ey + pxx * ey * ey) / det)
    nis = [float(r["mahalanobis"]) for r in fx if float(r["mahalanobis"]) > 0]
    e_fx = [float(r["errM"]) for r in fx]
    tj = float(a["totalJ"])
    fixc = float(a["fixCycles"])
    n25 = float(np.sum(err0 <= 25.0))
    g = n26(d, arm)
    return dict(
        av=float(np.mean([float(r["fixCycles"]) / float(r["steps"]) for r in ss])),
        w25=float(np.mean(w)), c50=float(a["cep50M"]), c95=float(a["cep95M"]),
        J=float(np.mean([float(r["totalJ"]) for r in ss])),
        err=err0, maxerr=float(err0.max()), far=float(np.mean(err0 > 1000.0)),
        j868=float(a["j_868"]), j2p4=float(a["j_2p4"]), juwb=float(a["j_uwb"]), tj=tj,
        jfix=tj / fixc if fixc > 0 else float("nan"), j25=tj / n25 if n25 > 0 else float("nan"),
        esc=float(a["escalations"]), suspects=float(a["suspects"]), cuwb=float(a["cycles_uwb"]),
        uwbfix=float(sum(1 for r in t0 if r["hadFix"] == "1" and r["fixTier"] == "UWB")),
        sus_share=float(np.mean([r["suspect"] == "1" for r in fx])) if fx else float("nan"),
        rej_share=float(np.mean([r["rejected"] == "1" for r in t0])),
        anees=float(np.mean(nees)) if nees else float("nan"),
        nees95=float(np.mean([x > CHI2_2_95 for x in nees])) if nees else float("nan"),
        nnees=len(nees),
        nis921=float(np.mean([x > CHI2_2_99 for x in nis])) if nis else float("nan"),
        nismed=float(np.median(nis)) if nis else float("nan"),
        tdoa_ref=(g[0] / (g[0] + g[1]) if g and (g[0] + g[1]) > 0 else float("nan")),
        rho_pcs=-spearman([float(r["pcs"]) for r in fx], e_fx),
        rho_tr=spearman([float(r["traceSigma"]) for r in fx], e_fx))


def collect(out, cell, arm):
    res = {}
    for d in cs.seeds(out, cell):
        if cs.ok(d):
            m = seed_metrics(d, arm)
            if m is not None:
                res[int(d.rsplit("s", 1)[1])] = m
    return res


def pooled(v, q):
    e = np.concatenate([x["err"] for x in v.values()]) if v else np.array([])
    return float(np.quantile(e, q)) if len(e) else float("nan")


def paired(a, b, k):
    s = sorted(set(a) & set(b))
    return cs.ci([a[i][k] - b[i][k] for i in s])


def above0(iv):
    return not math.isnan(iv[1]) and iv[1] > 0.0


def below0(iv):
    return not math.isnan(iv[2]) and iv[2] < 0.0


def inc0(iv):
    return not math.isnan(iv[1]) and iv[1] <= 0.0 <= iv[2]


def verdict(iv, low_better):
    if math.isnan(iv[1]) or inc0(iv):
        return "tie"
    good = (iv[0] < 0) if low_better else (iv[0] > 0)
    return "WIN" if good else "LOSS"


def main():
    if len(sys.argv) != 2:
        print(__doc__)
        return 2
    out = sys.argv[1]
    problems = []
    rcs = glob.glob(os.path.join(out, "*", "s*", "rc"))
    if len(rcs) != NCELLS:
        problems.append(f"{len(rcs)} cells with an rc, not {NCELLS}")
    for p in rcs:
        if open(p).read().strip() != "0":
            problems.append(f"{os.path.dirname(p)} exit {open(p).read().strip()}")
    srcs = set()
    for p in glob.glob(os.path.join(out, "*", "s*", "run_id.txt")):
        for ln in open(p):
            if ln.startswith("source "):
                srcs.add(" ".join(ln.split()[:4]))
    if len(srcs) != 1 or not next(iter(srcs)).endswith("dirty 0"):
        problems.append(f"run_id source lines {sorted(srcs)}")

    D = {}
    for c in CONFIGS:
        for k in (1, 10):
            for arm in ARMS:
                D[("M", c, k, arm)] = collect(out, f"M_{c}_k{k}", arm)
            D[("E", c, k, "ca_tbl_full")] = collect(out, f"E_{c}_k{k}", "ca_tbl_full")
        for k in (3, 30):
            D[("K", c, k, "ca_tbl_full")] = collect(out, f"K_{c}_k{k}", "ca_tbl_full")
    for key, v in D.items():
        if len(v) != 20:
            problems.append(f"{key}: {len(v)} seeds, not 20")
        elif any(x["nnees"] == 0 for x in v.values()):
            problems.append(f"{key}: a seed with no covariance rows (--traceCovariance did not reach the trace)")
    for c in CONFIGS:
        for k in (1, 10):
            fl = []
            for d in cs.seeds(out, f"E_{c}_k{k}"):
                try:
                    prof = any(ln.startswith("suspectForcesEscalation: explicit")
                               for ln in open(os.path.join(d, "spec_profile.txt")))
                    argv = "--suspectForcesEscalation=true" in open(os.path.join(d, "run_id.txt")).read()
                    fl.append(prof and argv)
                except OSError:
                    fl.append(False)
            if not fl or not all(fl):
                problems.append(f"E_{c}_k{k}: not every seed shows --suspectForcesEscalation=true in its invocation and as explicit in spec_profile.txt")

    md = ["# ACC-A — IMU increment covariance (--imuQuScale) and its consumers (scripts/acc_a_analyse.py)", "",
          "Mean [95 % seed bootstrap] over seeds 1-20; differences paired by seed. CEP50/95 per seed is the "
          "arm summary's; the pooled CEP (all cycles of all seeds) is S2's convention and is given beside it.", ""]
    table = []
    q = {}
    for c, cname in CONFIGS.items():
        md += [f"## {cname} ({c})", "",
               "| arm | k | availability | within 25 m (5 nodes) | CEP50 / CEP95 per seed (m) | pooled CEP50 / CEP95 (m) | J/node |",
               "|---|---|---|---|---|---|---|"]
        for arm in ARMS:
            for k in (1, 10):
                v = D[("M", c, k, arm)]
                if not v:
                    continue
                col = lambda key: [x[key] for x in v.values()]  # noqa: E731
                av = cs.ci(col("av"))
                md.append(f"| {arm} | {k} | {cs.fmt(*av)} | {np.mean(col('w25')):.3f} | {np.mean(col('c50')):.1f} / "
                          f"{np.mean(col('c95')):.1f} | {pooled(v, .5):.1f} / {pooled(v, .95):.1f} | {np.mean(col('J')):.2f} |")
                table.append(dict(config=c, arm=arm, k=k, availability=f"{av[0]:.4f}", within25=f"{np.mean(col('w25')):.4f}",
                                  cep50=f"{np.mean(col('c50')):.2f}", cep95=f"{np.mean(col('c95')):.2f}",
                                  pooled_cep50=f"{pooled(v, .5):.2f}", pooled_cep95=f"{pooled(v, .95):.2f}",
                                  j_node=f"{np.mean(col('J')):.3f}"))
        md += ["", "### 1. Covariance effect: k = 10 − k = 1, per arm", "",
               "| arm | Δ availability | Δ within 25 m | Δ CEP50 | Δ CEP95 | Δ J/node | Δ ANEES | Δ NIS>9.21 share | Δ TDoA refusal rate |",
               "|---|---|---|---|---|---|---|---|---|"]
        for arm in ARMS:
            a, b = D[("M", c, 10, arm)], D[("M", c, 1, arm)]
            r = {key: paired(a, b, key) for key in ("av", "w25", "c50", "c95", "J", "anees", "nis921", "tdoa_ref")}
            q[("cov", c, arm)] = r
            md.append(f"| {arm} | {cs.fmt(*r['av'])} | {cs.fmt(*r['w25'])} | {cs.fmt(*r['c50'], p=1)} | {cs.fmt(*r['c95'], p=1)} | "
                      f"{cs.fmt(*r['J'], p=2)} | {cs.fmt(*r['anees'], p=2)} | {cs.fmt(*r['nis921'])} | {cs.fmt(*r['tdoa_ref'])} |")
        md += ["", "Dose-response, ca_tbl_full (per-seed CEP95 mean, pooled CEP50 / CEP95, ANEES median):", ""]
        for k, key in ((1, ("M", c, 1, "ca_tbl_full")), (3, ("K", c, 3, "ca_tbl_full")),
                       (10, ("M", c, 10, "ca_tbl_full")), (30, ("K", c, 30, "ca_tbl_full"))):
            v = D[key]
            if v:
                md.append(f"- k = {k}: CEP95 {np.mean([x['c95'] for x in v.values()]):.1f} m; pooled {pooled(v, .5):.1f} / "
                          f"{pooled(v, .95):.1f} m; ANEES median {np.median([x['anees'] for x in v.values()]):.2f}; "
                          f"max error over seeds {max(x['maxerr'] for x in v.values()):.0f} m")
        md += ["", "### 2. Consumers of P, ca_tbl_full (tag 0, fix rows after 120 s)", "",
               "| k | ANEES median | NEES > 5.991 | NIS > 9.21 | NIS median | SUSPECT share | TDoA refusal rate | rejected share | PCS informativeness −ρ(pcs,err) | trace ρ(trace,err) | escalations | UWB cycles | UWB fixes |",
               "|---|---|---|---|---|---|---|---|---|---|---|---|---|"]
        for k in (1, 10):
            v = D[("M", c, k, "ca_tbl_full")]
            if not v:
                continue
            m = lambda key: float(np.nanmean([x[key] for x in v.values()]))  # noqa: E731
            md.append(f"| {k} | {np.nanmedian([x['anees'] for x in v.values()]):.2f} | {m('nees95'):.3f} | {m('nis921'):.3f} | "
                      f"{m('nismed'):.2f} | {m('sus_share'):.4f} | {m('tdoa_ref'):.3f} | {m('rej_share'):.4f} | {m('rho_pcs'):.3f} | "
                      f"{m('rho_tr'):.3f} | {m('esc'):.1f} | {m('cuwb'):.1f} | {m('uwbfix'):.1f} |")
        for k in (1, 10):
            v = D[("M", c, k, "ca_tbl_full")]
            s = sorted(v)
            q[("pcs", c, k)] = cs.ci([v[i]["rho_pcs"] - v[i]["rho_tr"] for i in s
                                      if not (math.isnan(v[i]["rho_pcs"]) or math.isnan(v[i]["rho_tr"]))])
            md.append(f"\nPCS − trace informativeness, k = {k}, paired: {cs.fmt(*q[('pcs', c, k)])} (> 0 means PCS predicts error better)")
        r = {key: paired(D[("M", c, 10, "ca_tbl_full")], D[("M", c, 1, "ca_tbl_full")], key)
             for key in ("sus_share", "esc", "cuwb", "uwbfix")}
        q[("cons", c)] = r
        md.append(f"\nk = 10 − k = 1: SUSPECT share {cs.fmt(*r['sus_share'], p=4)}; escalations {cs.fmt(*r['esc'], p=1)}; "
                  f"UWB cycles {cs.fmt(*r['cuwb'], p=1)}; UWB fixes {cs.fmt(*r['uwbfix'], p=1)}.")
        md += ["", "### 3. Integrity trigger (ca_tbl_full)", "",
               "| k | comparison | Δ availability | Δ CEP95 | Δ J/node | Δ escalations | Δ UWB fixes | suspects (mean per seed, ca_tbl_full) |",
               "|---|---|---|---|---|---|---|---|"]
        for k in (1, 10):
            full = D[("M", c, k, "ca_tbl_full")]
            for name, other in (("forced escalation (E) − preset", D[("E", c, k, "ca_tbl_full")]),
                                ("integrity off (no_integrity) − preset", D[("M", c, k, "ca_tbl_no_integrity")])):
                r = {key: paired(other, full, key) for key in ("av", "c95", "J", "esc", "uwbfix")}
                q[("int", c, k, name.split()[0])] = r
                md.append(f"| {k} | {name} | {cs.fmt(*r['av'])} | {cs.fmt(*r['c95'], p=1)} | {cs.fmt(*r['J'], p=2)} | "
                          f"{cs.fmt(*r['esc'], p=1)} | {cs.fmt(*r['uwbfix'], p=1)} | {np.mean([x['suspects'] for x in full.values()]):.1f} |")
        md += ["", "### 4. Scoreboard: ca_tbl_full − each arm, same k, paired", "",
               "| k | arm | availability | within 25 m | CEP50 | CEP95 | J/node (reported) |", "|---|---|---|---|---|---|---|"]
        for k in (1, 10):
            full = D[("M", c, k, "ca_tbl_full")]
            for arm in ARMS[1:]:
                o = D[("M", c, k, arm)]
                r = {key: paired(full, o, key) for key in ("av", "w25", "c50", "c95", "J")}
                q[("sb", c, k, arm)] = r
                md.append(f"| {k} | {arm} | {cs.fmt(*r['av'])} {verdict(r['av'], False)} | {cs.fmt(*r['w25'])} {verdict(r['w25'], False)} | "
                          f"{cs.fmt(*r['c50'], p=1)} {verdict(r['c50'], True)} | {cs.fmt(*r['c95'], p=1)} {verdict(r['c95'], True)} | "
                          f"{cs.fmt(*r['J'], p=2)} |")
        md += ["", "### 5. Energy, tag 0 (J over the run)", "",
               "| arm | k | 868 | 2.4 GHz | UWB | total | J per fix | J per cycle within 25 m | escalations |", "|---|---|---|---|---|---|---|---|---|"]
        for arm in ARMS:
            for k in (1, 10):
                v = D[("M", c, k, arm)]
                if not v:
                    continue
                m = lambda key: float(np.nanmean([x[key] for x in v.values()]))  # noqa: E731
                md.append(f"| {arm} | {k} | {m('j868'):.2f} | {m('j2p4'):.2f} | {m('juwb'):.2f} | {m('tj'):.2f} | {m('jfix'):.4f} | "
                          f"{m('j25'):.4f} | {m('esc'):.1f} |")
        for k in (1, 10):
            f_, t_ = D[("M", c, k, "ca_tbl_full")], D[("M", c, k, "fixed_threshold")]
            dj = np.mean([x["J"] for x in f_.values()]) - np.mean([x["J"] for x in t_.values()])
            dc = np.mean([x["c95"] for x in t_.values()]) - np.mean([x["c95"] for x in f_.values()])
            md.append(f"\nk = {k}, ca_tbl_full against fixed_threshold: {dj:+.2f} J/node for {dc:+.1f} m of CEP95 "
                      f"({'n/a' if abs(dc) < 1e-9 else f'{dj / dc:+.3f} J per metre'}; positive metres = ca_tbl_full more accurate)")
        md.append("")

    V = []
    try:
        for c in CONFIGS:
            pass
        fA, fB = D[("M", "A", 10, "ca_tbl_full")], D[("M", "B", 10, "ca_tbl_full")]
        V.append(("Q1", pooled(fA, .5) <= 13.0 and pooled(fA, .95) <= 70.0 and pooled(fB, .5) <= 3.2 and pooled(fB, .95) <= 10.0))
        V.append(("Q2", all(below0(q[("cov", c, "ca_tbl_full")]["c95"]) for c in CONFIGS)))
        V.append(("Q3", all(np.nanmedian([x["anees"] for x in D[("M", c, 1, "ca_tbl_full")].values()]) > 4.0
                            and below0(q[("cov", c, "ca_tbl_full")]["anees"]) for c in CONFIGS)))
        V.append(("Q4", all(below0(q[("cov", c, "ca_tbl_full")]["nis921"]) for c in CONFIGS)))
        V.append(("Q5", all(np.nanmean([x["nis921"] for x in D[("M", c, 10, "ca_tbl_full")].values()]) > 0.005 for c in CONFIGS)))
        V.append(("Q6", all(abs(q[("cov", c, a)]["av"][0]) <= 0.01 for c in CONFIGS for a in ARMS)))
        V.append(("Q7", all(pooled(D[("M", c, 10, a)], .95) <= 1.10 * pooled(D[("M", c, 1, a)], .95) for c in CONFIGS for a in ARMS)))
        V.append(("Q8", all(not above0(q[("pcs", c, 10)]) for c in CONFIGS)))
        V.append(("Q9", all(np.nanmean([x["sus_share"] for x in D[("M", c, 1, "ca_tbl_full")].values()]) <= 0.02
                            and not above0(q[("cons", c)]["sus_share"]) for c in CONFIGS)))
        V.append(("Q10", all(abs(q[("int", c, 1, "forced")]["av"][0]) <= 0.01 and inc0(q[("int", c, 1, "forced")]["c95"])
                             for c in CONFIGS)))
        V.append(("Q11", all(inc0(q[("int", c, k, "integrity")]["av"]) and inc0(q[("int", c, k, "integrity")]["c95"])
                             for c in CONFIGS for k in (1, 10))))
        V.append(("Q12", all(above0(q[("sb", c, 10, "fixed_threshold")]["av"]) and inc0(q[("sb", c, 10, "fixed_threshold")]["c95"])
                             for c in CONFIGS)))
        V.append(("Q13", all(above0(q[("cons", c)]["esc"]) for c in CONFIGS)))
    except (KeyError, TypeError, ValueError, IndexError) as e:
        problems.append(f"a prediction could not be evaluated: {e!r}")
    md += ["## Registered predictions", ""] + [f"- {k}: {'HELD' if v else 'FAILED'}" for k, v in V] + [""]
    if table:
        with open(os.path.join(out, "acc_a_table.csv"), "w", newline="") as fh:
            w = csv.DictWriter(fh, fieldnames=list(table[0].keys()), lineterminator="\n")
            w.writeheader()
            w.writerows(table)
    md += [f"Problems: {len(problems)}" + ("" if problems else " (none)")] + [f"- {p}" for p in problems]
    open(os.path.join(out, "ACC_A.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
