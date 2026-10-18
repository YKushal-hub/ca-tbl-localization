#!/usr/bin/env bash
# G146: V2a-2 --nlos3gppRMa (master plan v3). usage: bash scripts/nlos3gpp_gate.sh [<workdir>]
# (1) unit (scratch/v2a2_nlos_unit.cc): 3GPP TR 38.901 RMa LOS probability and NLOS excess, fallback below 10 m,
#     and its --break=far fixture (red); (2) explicit false reproduces the default; (3) the flag raises the share
#     of tag 0's 868 MHz gateway ranges that are NLOS (RMa: P_NLOS 0.63 at 1 km against the old 0.12).
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
UNIT="$(pwd)/build/scratch/ns3.47-v2a2_nlos_unit-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g146.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
bad=0
"$UNIT" || bad=1
if "$UNIT" --break=far > "$D/fix.txt" 2>&1; then echo "G146 FAIL FIXTURE  --break=far did not turn the unit checks red"; bad=1; else echo "G146 PASS FIXTURE  --break=far is red"; fi
B="--specProfile=true --arms=static_868 --seed=7 --duration=600 --trace=true --traceRanges=true"
for n in base off on; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --nlos3gppRMa=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --nlos3gppRMa=true --outdir="$D/on" > "$D/on.log" 2>&1 &
wait
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G146 PASS DEFAULT  explicit false reproduces the default"; else echo "G146 FAIL DEFAULT  $n outputs differ"; bad=1; fi
python3 - "$D" <<'PY' || bad=1
import csv, sys
d = sys.argv[1]
def share(run):
    r = [x for x in csv.DictReader(open(f"{d}/{run}/ranges_static_868_seed7.csv")) if x["sourceKind"] == "1"]
    return (sum(x["nlosTruth"] == "1" for x in r) / len(r) if r else float("nan")), len(r)
(b, nb), (o, no) = share("base"), share("on")
ok = nb > 20 and no >= 5 and o > b + 0.1  # on: RMa NLOS removes most 868 gateway links (241 -> 15 rows in dev), so few remain
print(f"G146 {'PASS' if ok else 'FAIL'} FIRES    NLOS share of gateway ranges {b:.3f} (n {nb}) -> {o:.3f} (n {no})")
sys.exit(0 if ok else 1)
PY
exit $bad
