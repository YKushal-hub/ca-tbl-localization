#!/usr/bin/env python3
"""D1 - arm liveness matrix. Does each arm EXERCISE its mechanism, not just run."""
import csv, glob, os, sys

OUTD = sys.argv[1]
CFG = {
 "A": "boxHalf=2000 RURAL_OPEN legacy dwell=90",
 "B": "boxHalf=2000 RURAL_OPEN propagation dwell=90",
 "C": "boxHalf=1500 RURAL_OPEN legacy dwell=800   [H_cost PASS cell]",
 "D": "boxHalf=2000 RURAL_FOREST legacy dwell=800 [H_reliability cell]",
 "E": "boxHalf=2000 RURAL_OPEN propagation dwell=90 + suspectForcesEscalation=false",
 "F": "boxHalf=2000 URBAN_DENSE propagation dwell=90 [collapse region]",
}
HYBRID = {"ca_tbl_full", "ca_tbl_no_integrity", "fixed_threshold"}
STATIC = {"static_868": "cycles_868", "static_2p4": "cycles_2p4", "static_uwb": "cycles_uwb"}

flags = []
rows = []
for name in sorted(CFG):
    d = os.path.join(OUTD, name)
    f = os.path.join(d, "arm_summary.csv")
    if not os.path.exists(f) or os.path.getsize(f) == 0:
        rows.append((name, "(all)", "ERROR", "no arm_summary — run failed or timed out", {}))
        flags.append((name, "(all)", "ERROR: no arm_summary"))
        continue
    for r in csv.DictReader(open(f)):
        arm = r["arm"]
        g = lambda k: float(r.get(k, 0) or 0)
        c8, c24, cu = g("cycles_868"), g("cycles_2p4"), g("cycles_uwb")
        tot = c8 + c24 + cu
        esc, dem = g("escalations"), g("deescalations")
        sus = g("suspects")
        fix, nofix = g("fixCycles"), g("noFixCycles")
        had = fix / (fix + nofix) if (fix + nofix) else 0.0
        tp, fp, fn = g("trig_tp"), g("trig_fp"), g("trig_fn")
        verdicts = []
        if arm in HYBRID and tot > 0:
            frac = max(c8, c24, cu) / tot
            if frac > 0.99:
                which = "868" if c8 == max(c8, c24, cu) else ("2p4" if c24 == max(c8, c24, cu) else "uwb")
                verdicts.append("DEGENERATE: %.1f%% on %s" % (100 * frac, which))
            if esc == 0:
                verdicts.append("DEGENERATE: 0 escalations")
        if arm == "ca_tbl_full" and sus == 0:
            verdicts.append("GATE INERT: 0 suspects")
        if had < 0.90:
            verdicts.append("FINDING: hadFix %.3f" % had)
        if tp + fp == 0:
            verdicts.append("FINDING: trig tp+fp=0, metric has no population")
        if arm in STATIC:
            own = STATIC[arm]
            other = sum(g(k) for k in ("cycles_868", "cycles_2p4", "cycles_uwb") if k != own)
            if other > 0:
                verdicts.append("BROKEN: static arm shows %d cycles on other tiers" % other)
        rows.append((name, arm, "; ".join(verdicts) if verdicts else "ok",
                     "cyc %d/%d/%d esc %d sus %d cep50 %s" % (c8, c24, cu, esc, sus, r.get("cep50M", "?")), r))
        for v in verdicts:
            if v.split(":")[0] in ("DEGENERATE", "GATE INERT", "BROKEN"):
                flags.append((name, arm, v))

print("# D1 — arm liveness matrix\n")
print("Does each arm **exercise its mechanism**, not merely run to completion.\n")
print("## Flagged cells — read this block first\n")
if flags:
    print("| config | arm | verdict | configuration |")
    print("|---|---|---|---|")
    for n, a, v in flags:
        print("| %s | `%s` | **%s** | %s |" % (n, a, v, CFG.get(n, "?")))
else:
    print("none")

print("\n## Full matrix\n")
print("| config | arm | verdict | cycles 868/2p4/uwb, escalations, suspects, cep50 |")
print("|---|---|---|---|")
for n, a, v, ctx, _ in rows:
    print("| %s | `%s` | %s | %s |" % (n, a, v if v != "ok" else "ok", ctx))

print("\n## Configurations\n")
for k in sorted(CFG):
    print("- **%s** — %s" % (k, CFG[k]))

print("\n## D7 pre-check (config E)\n")
e = [r for r in rows if r[0] == "E" and r[1] == "ca_tbl_full"]
if not e:
    print("Config E did not produce a result — **D7 cannot be pre-checked**; D7 skipped.")
else:
    esc = float(e[0][4].get("escalations", 0) or 0)
    if esc == 0:
        print("`ca_tbl_full` shows **0 escalations** with `suspectForcesEscalation=false`. "
              "Both arms are then degenerate and the D7 comparison is void in the other "
              "direction. **D7 should be SKIPPED**, and this is the reason.")
    else:
        print("`ca_tbl_full` shows **%d escalations** with `suspectForcesEscalation=false`, "
              "so it still escalates without the suspect trigger. **D7 is worth running.**" % esc)

st = "FINDING" if flags else "OK"
print("\nSTATUS: %s" % st)
