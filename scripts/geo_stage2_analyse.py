#!/usr/bin/env python3
"""GEO-01 stage 2 analysis (report/PREREG_GEO01.md, sect 3-4). Reads <outdir>/raw/<cand>/s<seed>/
and writes <outdir>/geo_stage2.csv (one row per candidate, arm and seed), geo_stage2.md (the
per-candidate table, the criterion, the selection) and the abort checks.

usage: geo_stage2_analyse.py <outdir>
"""
import csv
import glob
import json
import os
import statistics as st
import sys

DUR = 1800.0
BOX = 5000.0
AVAIL = 0.95
T100 = 600.0
ORDER = {"cluster": 0, "star": 0, "arc": 1, "ring": 2, "perimeter": 2}  # fewer access points first


def main():
    out = sys.argv[1]
    rows, aborts = [], []
    for cdir in sorted(glob.glob(os.path.join(out, "raw", "*"))):
        cand = os.path.basename(cdir)
        n = int(cand.split("_")[0][1:])
        shape = cand.split("_", 1)[1]
        for sdir in sorted(glob.glob(os.path.join(cdir, "s*"))):
            seed = int(os.path.basename(sdir)[1:])
            rc = open(os.path.join(sdir, "rc")).read().strip() if os.path.exists(os.path.join(sdir, "rc")) else "?"
            if rc != "0":
                aborts.append((cand, seed, f"exit {rc}"))
                continue
            dep = json.load(open(os.path.join(sdir, "deployment.json")))
            anc = dep.get("anchors", {})
            gws = [a for t in (anc.values() if isinstance(anc, dict) else [anc]) for a in t if isinstance(a, dict)]
            for g in gws:
                x, y = float(g.get("x", 0)), float(g.get("y", 0))
                if abs(x) > BOX + 1e-6 or abs(y) > BOX + 1e-6:
                    aborts.append((cand, seed, f"gateway outside the box {x},{y}"))
            arms = {r["arm"]: r for r in csv.DictReader(open(os.path.join(sdir, "arm_summary.csv")))}
            subj = list(csv.DictReader(open(os.path.join(sdir, "subject_summary.csv"))))
            for arm, a in arms.items():
                ss = [r for r in subj if r["arm"] == arm]
                avail = st.mean(float(r["fixCycles"]) / float(r["steps"]) for r in ss) if ss else float("nan")
                t100 = float(a["tFirst100mS"])
                tff = float(a["tFirstFixS"])
                tr = os.path.join(sdir, f"trace_{arm}_seed{seed}.csv")
                uwb_t, r25, ncyc = DUR, 0, 0
                for r in csv.DictReader(open(tr)):
                    ncyc += 1
                    if r["nbrUwb"] not in ("", "0") and uwb_t == DUR:
                        uwb_t = float(r["t"])
                    if float(r["errM"]) <= 25.0:
                        r25 += 1
                rows.append(dict(cand=cand, N=n, shape=shape, arm=arm, seed=seed, availability=round(avail, 6),
                                 cep50M=float(a["cep50M"]), cep95M=float(a["cep95M"]),
                                 tFirstFixS=(tff if tff >= 0 else DUR), tFirst100mS=(t100 if t100 >= 0 else DUR),
                                 t100Censored=int(t100 < 0), firstUwbContactS=uwb_t, fracErrLe25=round(r25 / max(ncyc, 1), 6),
                                 subjMeanJ=round(st.mean(float(r["totalJ"]) for r in ss), 6) if ss else float("nan")))
    with open(os.path.join(out, "geo_stage2.csv"), "w", newline="") as f:
        w = csv.DictWriter(f, fieldnames=list(rows[0].keys()), lineterminator="\n")
        w.writeheader()
        w.writerows(rows)
    agg = {}
    for r in rows:
        agg.setdefault((r["cand"], r["arm"]), []).append(r)
    lines = ["| candidate | arm | seeds | availability (mean) | tFirst100m median (s) | CEP50 / CEP95 mean (m) | first UWB contact median (s) | err <= 25 m | subject J |",
             "|---|---|---|---|---|---|---|---|---|"]
    summ = {}
    for (cand, arm), rs in sorted(agg.items(), key=lambda kv: (int(kv[0][0].split('_')[0][1:]), kv[0][0], kv[0][1])):
        av = st.mean(r["availability"] for r in rs)
        t1 = st.median(r["tFirst100mS"] for r in rs)
        c50 = st.mean(r["cep50M"] for r in rs)
        c95 = st.mean(r["cep95M"] for r in rs)
        uw = st.median(r["firstUwbContactS"] for r in rs)
        e25 = st.mean(r["fracErrLe25"] for r in rs)
        j = st.mean(r["subjMeanJ"] for r in rs)
        summ[(cand, arm)] = dict(av=av, t1=t1, c95=c95, N=rs[0]["N"], shape=rs[0]["shape"], seeds=len(rs))
        lines.append(f"| {cand} | {arm} | {len(rs)} | {av:.3f} | {t1:.0f} | {c50:.1f} / {c95:.1f} | {uw:.0f} | {e25:.3f} | {j:.2f} |")
    meets = {k: v for k, v in summ.items() if k[1] == "ca_tbl_full" and v["av"] >= AVAIL and v["t1"] <= T100 and v["seeds"] == 10}
    sel = ""
    if meets:
        nmin = min(v["N"] for v in meets.values())
        pool = [(k, v) for k, v in meets.items() if v["N"] == nmin]
        pool.sort(key=lambda kv: (kv[1]["c95"], ORDER.get(kv[1]["shape"].rstrip("0123456789"), 9)))
        sel = f"**Winner: {pool[0][0][0]}** (smallest N meeting the criterion is {nmin}; best CEP95 among them {pool[0][1]['c95']:.1f} m)."
        if len(pool) > 1:
            sel += f" Runner-up: {pool[1][0][0]}."
    else:
        full = [(k, v) for k, v in summ.items() if k[1] == "ca_tbl_full"]
        full.sort(key=lambda kv: (-kv[1]["av"], kv[1]["c95"]))
        sel = (f"**No candidate meets the criterion** (availability >= {AVAIL} and median tFirst100m <= {T100:.0f} s). "
               f"Registered fallback: the highest mean availability, {full[0][0][0]} ({full[0][1]['av']:.3f}, CEP95 {full[0][1]['c95']:.1f} m); "
               f"runner-up {full[1][0][0]}.")
    md = ["# GEO-01 stage 2 — generated table (scripts/geo_stage2_analyse.py)", "",
          f"Criterion (ca_tbl_full): mean availability >= {AVAIL} and median tFirst100m <= {T100:.0f} s (never reached = {DUR:.0f} s).", "",
          *lines, "", sel, "", f"Abort checks: {len(aborts)} problems" + (f": {aborts[:10]}" if aborts else " (none)")]
    open(os.path.join(out, "geo_stage2.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))
    return 1 if aborts else 0


if __name__ == "__main__":
    sys.exit(main())
