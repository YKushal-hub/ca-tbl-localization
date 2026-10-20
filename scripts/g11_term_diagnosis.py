#!/usr/bin/env python3
"""T18 / TECH_DEBT J.1: decompose G11's 2.4 GHz margin gap term by term (diagnosis only; nothing is tuned).
usage: g11_term_diagnosis.py <probe outdir>
The probe must be run at probe_v7's configuration with per-term output:
  ca_tbl_causal_sim --probe=true --probeTerms=true --envFacility=0 --vegFacility=1 --outdir=<dir>
(it reproduces results/probe_v7's margins exactly). Reads the frozen corpus (src/catbl-propagation-study/results/corpus.json).
Prints, per distance: path loss, NLOS, foliage, fading (engine / model), engine-only terrain and other terms, the
margins, and the effective sensitivity the engine used (rebuilt receive power minus its margin)."""
import csv, os, sys
H = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "src", "catbl-propagation-study")
sys.path.insert(0, H)
import linkbudget as lb, validate as va  # noqa: E402
d0 = sys.argv[1]
draws, _, _ = va.load_corpus()
t = "2.4GHz"; phy = lb.TIER_PHY[t]; env = lb.TIER_ENV[t]; n = lb.ENV[env]["n"]; ht = lb.ANCHOR_HEIGHT_M[t]; hr = lb.NODE_HEIGHT_M
sens_m = lb.SENSITIVITY_DBM[t] + lb.CODING_RATE_OFFSET_DB
eng = {float(r["distance_m"]): r for r in csv.DictReader(open(os.path.join(d0, "probe_terms.csv"))) if r["tier"] == t}
print("dist | PL eng/mdl | NLOS eng/mdl | foliage eng/mdl | fade eng/mdl | terrain eng | other eng | margin eng/mdl | eff.sens eng (model %.1f)" % sens_m)
for d in sorted(eng):
    d3 = lb.slant_distance_m(d, ht, hr); pl = []; ex = []; fo = []; mg = []
    for (_, u_nl, u_at, u_df) in draws:
        is_n = u_nl < lb.nlos_probability(d3, lb.ENV[env]["nlos_scale_m"])
        pl.append(lb.two_ray_path_loss_db(phy["f"], d3, ht, hr) if (env == lb.TWO_RAY_ENV and not is_n) else lb.path_loss_db(phy["f"], d3, n))
        ex.append(lb.nlos_excess_db(u_nl, u_at, u_df, d3, env)); fo.append(lb.foliage_loss_db(t, u_df)); mg.append(lb.margin_db(t, d, u_nl, u_at, u_df, u_df))
    m = lambda v: sum(v) / len(v); e = eng[d]; g = lambda k: float(e["mean_" + k])
    other = sum(g(k) for k in ("shadowing", "atmospheric", "interference", "hwtx", "frontend"))
    L = sum(g(k) for k in ("pathloss", "shadowing", "fading", "nlos", "foliage", "terrain", "atmospheric", "interference", "hwtx", "frontend"))
    eff = (phy["tx"] - L) - g("margin")
    print(f"{d:5.0f} | {g('pathloss'):6.2f}/{m(pl):6.2f} | {g('nlos'):5.2f}/{m(ex):5.2f} | {g('foliage'):5.2f}/{m(fo):5.2f} | {g('fading'):4.2f}/{lb.mean_fading_loss_db(env):4.2f} | "
          f"{g('terrain'):5.2f} | {other:5.2f} | {g('margin'):7.2f}/{m(mg):7.2f} | {eff:8.2f}")
