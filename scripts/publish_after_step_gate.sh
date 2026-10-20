#!/usr/bin/env bash
# G156: T2 --publishAfterStep (TECH_DEBT L.2 note). usage: bash scripts/publish_after_step_gate.sh [<workdir>]
# DEFAULT  explicit false reproduces the default byte for byte;
# STALE    flag off: held copies of tag 0's entry carry the PREVIOUS cycle's trace estimate (the defect, >= 90 %)
#          -- this is also the negative fixture for FRESH: the same check FRESH passes must fail here;
# FRESH    flag on: held copies carry the SAME cycle's trace estimate (>= 90 %).
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g156.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
B="--arms=ca_tbl_full --seed=7 --duration=600 --trace=true --nFinders=4 --finderMode=1 --boxHalf=750 --sendHandshakeFrames=true --bidirectionalDigest=true --entryVelocity=true"
for n in base off on; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --publishAfterStep=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --publishAfterStep=true --outdir="$D/on" > "$D/on.log" 2>&1 &
wait
bad=0
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G156 PASS DEFAULT  explicit false reproduces the default"; else echo "G156 FAIL DEFAULT  $n outputs differ"; bad=1; fi
python3 - "$D" <<'PY' || bad=1
import csv, sys
d = sys.argv[1]
def frac(run):
    tr = {round(float(r["t"]), 3): (float(r["estX"]), float(r["estY"])) for r in csv.DictReader(open(f"{d}/{run}/trace_ca_tbl_full_seed7.csv"))}
    ts = sorted(tr)
    same = prev = n = 0
    for r in csv.DictReader(open(f"{d}/{run}/entries_ca_tbl_full_seed7.csv")):
        if r["kind"] != "held" or r["entryId"] != "1":
            continue
        st = round(float(r["timestampS"]), 3)
        if st not in tr:
            continue
        i = ts.index(st)
        x, y = float(r["x"]), float(r["y"])
        n += 1
        if abs(x - tr[st][0]) < 2e-3 and abs(y - tr[st][1]) < 2e-3:
            same += 1
        elif i > 0 and abs(x - tr[ts[i - 1]][0]) < 2e-3 and abs(y - tr[ts[i - 1]][1]) < 2e-3:
            prev += 1
    return n, same, prev
no, so, po = frac("off"); nn, sn, pn = frac("on")
ok1 = no > 20 and po >= 0.9 * no and so < 0.1 * no
ok2 = nn > 20 and sn >= 0.9 * nn
print(f"G156 {'PASS' if ok1 else 'FAIL'} STALE   flag off: {po}/{no} held entries carry the previous cycle's estimate, {so} the same cycle's")
print(f"G156 {'PASS' if ok2 else 'FAIL'} FRESH   flag on: {sn}/{nn} held entries carry the same cycle's estimate")
sys.exit(0 if ok1 and ok2 else 1)
PY
exit $bad
