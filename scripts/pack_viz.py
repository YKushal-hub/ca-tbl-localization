#!/usr/bin/env python3
"""Pack a recorded run into the JSON the visualiser replays.

Nothing here computes anything: it reformats what the run wrote. Positions, tiers,
errors and packet outcomes all come from the trace and the event log as recorded, so
the animation is a replay of that run and not a re-enactment of it.

Events are BUCKETED PER CYCLE and capped, because a 600 s run emits ~40k frames and an
animation cannot show them individually anyway. The cap is on what is DRAWN; the per
cycle counts are totalled over every event, so the numbers on screen stay honest even
where the lines on screen are a sample. That distinction is surfaced in the UI.
"""
import csv
import json
import os
import sys
from collections import defaultdict

D = os.path.expanduser("~/catbl_work/demo")
ARM = sys.argv[1] if len(sys.argv) > 1 else "ca_tbl_full"
SEED = 7
DRAW_CAP = 28  # per cycle, per the note above

dep = json.load(open(os.path.join(D, "deployment.json")))

tr = os.path.join(D, "trace_%s_seed%d.csv" % (ARM, SEED))
ev = os.path.join(D, "events_%s_seed%d.csv" % (ARM, SEED))
for f in (tr, ev):
    if not os.path.isfile(f):
        sys.exit("missing %s" % f)

TIER_OF_NAME = {"868MHz": 0, "2.4GHz": 1, "UWB": 2}

cycles = []
for r in csv.DictReader(open(tr)):
    try:
        cycles.append({
            "t": round(float(r["t"]), 2),
            "tx": round(float(r["trueX"]), 1),
            "ty": round(float(r["trueY"]), 1),
            "ex": round(float(r["estX"]), 1),
            "ey": round(float(r["estY"]), 1),
            "err": round(float(r["errM"]), 2),
            "tier": TIER_OF_NAME.get(r["activeTier"].strip(), 0),
            "fixTier": TIER_OF_NAME.get(r["fixTier"].strip(), -1),
            "pcs": round(float(r["pcs"]), 3),
            "sig": round(float(r["traceSigma"]), 1),
            "fix": int(r["hadFix"]),
            "esc": int(r["escalated"]),
            "des": int(r["deescalated"]),
            "sus": int(r["suspect"]),
            "nb": [int(r["nbr868"]), int(r["nbr2p4"]), int(r["nbrUwb"])],
            "vis": [int(r["vis868"]), int(r["vis2p4"]), int(r["visUwb"])],
            "j": round(float(r["cumJ"]), 3),
            "na": int(r["nAnchors"]),
            "nlos": int(r["nNlos"]),
        })
    except (KeyError, ValueError, TypeError):
        continue

if not cycles:
    sys.exit("no cycles parsed")

dt = round(cycles[1]["t"] - cycles[0]["t"], 3) if len(cycles) > 1 else 2.5
t0 = cycles[0]["t"]

drawn = defaultdict(list)
counts = defaultdict(lambda: defaultdict(int))
total = defaultdict(int)
with open(ev) as fh:
    for r in csv.DictReader(fh):
        try:
            t = float(r["t"])
            tier = int(r["tier"])
            out = r["outcome"].strip()
        except (KeyError, ValueError, TypeError):
            continue
        idx = int((t - t0 + dt) / dt)
        if idx < 0 or idx >= len(cycles):
            continue
        counts[idx][out] += 1
        counts[idx]["tier%d" % tier] += 1
        total[out] += 1
        if len(drawn[idx]) < DRAW_CAP:
            try:
                drawn[idx].append([int(r["txId"]), int(r["rxId"]), tier,
                                   {"delivered": 0, "capture": 1, "collision": 2}.get(out, 0),
                                   r["msgType"].strip()])
            except (KeyError, ValueError):
                pass

for i, c in enumerate(cycles):
    c["ev"] = drawn.get(i, [])
    cc = counts.get(i, {})
    c["ec"] = [cc.get("delivered", 0), cc.get("capture", 0), cc.get("collision", 0)]

out = {
    "arm": ARM,
    "seed": SEED,
    "dt": dt,
    "deployment": dep,
    "cycles": cycles,
    "totals": dict(total),
    "drawCap": DRAW_CAP,
}
path = os.path.expanduser("~/catbl_work/demo/viz_%s.json" % ARM)
json.dump(out, open(path, "w"), separators=(",", ":"))
print("wrote %s  cycles=%d  events_total=%s  size=%.2f MB"
      % (path, len(cycles), dict(total), os.path.getsize(path) / 1e6))
