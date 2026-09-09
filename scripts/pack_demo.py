#!/usr/bin/env python3
"""Pack one run into a single JSON the SAR console embeds.

WHAT IS SAMPLED AND WHAT IS COMPLETE, because a viewer will reasonably assume the
picture is the run and it is not:

  packets   COMPLETE per cycle up to a per-cycle cap. The cap is on what is DRAWN.
            Every cycle carries its true counts alongside, so the rail's totals are
            the run's totals even when the map shows a sample of one cycle's frames.
  tables    sampled at whatever --vizTablePeriodS the run used. The period travels
            in the JSON so the console can say so rather than implying continuity.
  tracks    every cycle. They are two floats per node and cost nothing.

Usage: pack_demo.py <run-dir> <arm> <seed> <out.json>
"""
import csv, json, os, sys
from collections import defaultdict

D, ARM, SEED, OUT = sys.argv[1], sys.argv[2], int(sys.argv[3]), sys.argv[4]
PKT_CAP = 40          # drawn packets per cycle
DT = 2.5

def path(p):
    return os.path.join(D, p)

dep = json.load(open(path("deployment.json")))

# ---- node tracks, and the role each node plays -----------------------------
# Role comes from the id range the scenario assigns, which is the only place it is
# expressed: 101-199 gateway, 501+ finder, 1 the subject, 2+ peers.
def role_of(nid):
    if 101 <= nid <= 199: return "gateway"
    if nid >= 501:        return "finder"
    if nid == 1:          return "lost"
    return "peer"

tracks = defaultdict(dict)
for r in csv.DictReader(open(path("nodes_%s_seed%d.csv" % (ARM, SEED)))):
    ci = int(round(float(r["t"]) / DT))
    tracks[int(r["nodeId"])][ci] = [round(float(r["x"]), 1), round(float(r["y"]), 1)]

# ---- per-cycle truth, estimate and tier, from the trace --------------------
cyc = {}
tp = path("trace_%s_seed%d.csv" % (ARM, SEED))
if os.path.exists(tp):
    for r in csv.DictReader(open(tp)):
        ci = int(round(float(r["t"]) / DT))
        cyc[ci] = {
            "t": round(float(r["t"]), 1),
            "at": int(r["activeTier"]) if r["activeTier"].isdigit() else -1,
            "ft": r["fixTier"],
            "tx": round(float(r["trueX"]), 1), "ty": round(float(r["trueY"]), 1),
            "ex": round(float(r["estX"]), 1),  "ey": round(float(r["estY"]), 1),
            "err": round(float(r["errM"]), 1),
            "sig": round(float(r.get("traceSigma", 0) or 0) ** 0.5, 1),
            "pcs": round(float(r["pcs"]), 3),
            "fix": int(r["hadFix"]), "susp": int(r["suspect"]),
            "nb": [int(r["nbr868"]), int(r["nbr2p4"]), int(r["nbrUwb"])],
            "esc": int(r["escalated"]), "de": int(r["deescalated"]),
        }

# ---- packets, bucketed per cycle ------------------------------------------
pk = defaultdict(list)
tot = defaultdict(lambda: [0, 0, 0])   # delivered, capture, collision
OUTC = {"delivered": 0, "capture": 1, "collision": 2}
ep = path("events_%s_seed%d.csv" % (ARM, SEED))
for r in csv.DictReader(open(ep)):
    ci = int(float(r["t"]) / DT)
    o = OUTC.get(r["outcome"], 0)
    tot[ci][o] += 1
    if len(pk[ci]) < PKT_CAP:
        pk[ci].append([int(r["txId"]), int(r["rxId"]), int(r["tier"]),
                       r["msgType"], o, round(float(r["rxPowerDbm"]), 1),
                       r["payload"]])

# ---- the two tables, at their sampled times -------------------------------
nbr = defaultdict(lambda: defaultdict(list))
p = path("nbrtable_%s_seed%d.csv" % (ARM, SEED))
if os.path.exists(p):
    for r in csv.DictReader(open(p)):
        ci = int(round(float(r["t"]) / DT))
        nbr[ci][int(r["nodeId"])].append(
            [int(r["peerId"]), int(r["tier"]), round(float(r["rxPowerDbm"]), 1),
             round(float(r["snrDb"]), 1), int(r["heardCount"])])

loc = defaultdict(lambda: defaultdict(list))
p = path("loctable_%s_seed%d.csv" % (ARM, SEED))
if os.path.exists(p):
    for r in csv.DictReader(open(p)):
        ci = int(round(float(r["t"]) / DT))
        loc[ci][int(r["nodeId"])].append(
            [int(r["entryId"]), round(float(r["x"]), 1), round(float(r["y"]), 1),
             round(float(r["ageS"]), 1), int(r["seqNo"]), int(r["hopCount"]),
             r["freshness"]])

ncyc = (max(cyc) + 1) if cyc else (max(pk) + 1 if pk else 0)
roles = {str(n): role_of(n) for n in tracks}
for a in dep["anchors"]["0"]:
    roles[str(a["id"])] = "gateway"

out = {
    "arm": ARM, "seed": SEED, "dt": DT, "ncyc": ncyc,
    "box": dep["boxHalfM"], "gw": dep["anchors"]["0"],
    "roles": roles,
    "tracks": {str(k): v for k, v in tracks.items()},
    "cyc": cyc,
    "pk": {str(k): v for k, v in pk.items()},
    "tot": {str(k): v for k, v in tot.items()},
    "nbr": {str(k): {str(n): v for n, v in d.items()} for k, d in nbr.items()},
    "loc": {str(k): {str(n): v for n, v in d.items()} for k, d in loc.items()},
    "cap": PKT_CAP,
}
json.dump(out, open(OUT, "w"), separators=(",", ":"))
print("wrote %s  cycles=%d  drawn_packets=%d  size=%.2f MB"
      % (OUT, ncyc, sum(len(v) for v in pk.values()), os.path.getsize(OUT) / 1e6))
