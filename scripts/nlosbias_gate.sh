#!/usr/bin/env bash
# G139: V2a --nlosRangeBias (master plan v3). usage: bash scripts/nlosbias_gate.sh [<workdir>]
# (1) unit level: scratch/v2_ranging_unit.cc, forced LOS/NLOS states, plus its three negative fixtures;
# (2) scenario: --nlosRangeBias=false given explicitly reproduces the default byte for byte.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
UNIT="$(pwd)/build/scratch/ns3.47-v2_ranging_unit-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g139.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
bad=0
"$UNIT" > "$D/unit.txt" 2>&1 || bad=1
cat "$D/unit.txt"
for f in excess wolf loss868; do
    if "$UNIT" --break=$f > "$D/fix_$f.txt" 2>&1; then echo "G139 FAIL FIXTURE   --break=$f did not turn the unit check red"; bad=1
    else echo "G139 PASS FIXTURE   --break=$f is red"; fi
done
B="--specProfile=true --arms=static_868,static_2p4 --seed=7 --duration=600 --trace=true --traceRanges=true --rssiRangeFromShadowing=true"
mkdir -p "$D/base" "$D/off"
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 & "$BIN" $B --nlosRangeBias=false --outdir="$D/off" > "$D/off.log" 2>&1 & wait
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ] && [ "$(ls "$D/base" | wc -l)" -eq "$(ls "$D/off" | wc -l)" ]; then echo "G139 PASS DEFAULT   explicit false reproduces the default"; else echo "G139 FAIL DEFAULT   $n outputs differ"; bad=1; fi
exit $bad
