#!/usr/bin/env python3
"""D7 report: rejection isolated from escalation."""
import csv, glob, os, statistics as st, sys
A, R, CYC = 0.95, 25.0, 1440
D = sys.argv[1]
def avail(p, arm, s):
    f = os.path.join(p, "trace_%s_seed%d.csv" % (arm, s))
    if not os.path.exists(f): return None
    a = 0
    for r in csv.DictReader(open(f)):
        if r.get("hadFix") in ("1","true","True"):
            try:
                if float(r["errM"]) <= R: a += 1
            except (ValueError, KeyError): pass
    return a / CYC
def sm(p, arm, col):
    f = os.path.join(p, "arm_summary.csv")
    if not os.path.exists(f) or os.path.getsize(f)==0: return None
    for r in csv.DictReader(open(f)):
        if r["arm"] == arm: return float(r.get(col, 0) or 0)
    return None
print("# D7 — the clean integrity test (`--suspectForcesEscalation=false`)\n")
print("Rejection isolated from escalation. Pre-registration: `%s/PREDICTION.md`.\n" % D)
cells = sorted(glob.glob(os.path.join(D, "c_*")))
if not cells:
    print("No cells produced.\n\nSTATUS: ERROR"); sys.exit(0)
print("| cell | seeds | full avail | noint avail | gap | full esc | noint esc | full suspects |")
print("|---|---|---|---|---|---|---|---|")
big = 0; ndone = 0
for c in cells:
    seeds = sorted(int(os.path.basename(x).split("_")[1]) for x in glob.glob(os.path.join(c,"seed_*")))
    fa, na, fe, ne, fs = [], [], [], [], []
    for s in seeds:
        p = os.path.join(c, "seed_%d" % s)
        a1, a2 = avail(p,"ca_tbl_full",s), avail(p,"ca_tbl_no_integrity",s)
        if a1 is None or a2 is None: continue
        fa.append(a1); na.append(a2)
        fe.append(sm(p,"ca_tbl_full","escalations") or 0)
        ne.append(sm(p,"ca_tbl_no_integrity","escalations") or 0)
        fs.append(sm(p,"ca_tbl_full","suspects") or 0)
    if not fa: continue
    ndone += len(fa)
    gap = st.mean(fa) - st.mean(na)
    if abs(gap) >= 0.05: big += 1
    print("| `%s` | %d | %.4f | %.4f | **%+.4f** | %.0f | %.0f | %.0f |"
          % (os.path.basename(c), len(fa), st.mean(fa), st.mean(na), gap,
             st.mean(fe), st.mean(ne), st.mean(fs)))
print("\n## Registered prediction vs outcome\n")
print("Predicted: gap < 0.05 at every cell — rejection alone subtracts a fix without")
print("supplying a better one.\n")
print("- cells with |gap| >= 0.05: **%d of %d**" % (big, len(cells)))
print("- seed-runs completed: %d" % ndone)
print("\n%s" % ("**Prediction MISSED** — rejection alone is load-bearing somewhere; that is a genuine result and it belongs in the record."
                if big else "**Prediction HELD** — with escalation removed, the integrity gate's rejection does not by itself change availability materially."))
print("\nSTATUS: %s" % ("FINDING" if big else "OK"))
