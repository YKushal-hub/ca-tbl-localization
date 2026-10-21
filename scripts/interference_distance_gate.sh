#!/usr/bin/env bash
# G153: T29 --interferenceDistance (TECH_DEBT M.1). usage: bash scripts/interference_distance_gate.sh [<workdir>]
# (1) unit (scratch/t29_interf_unit.cc) and its --break=ignore fixture; (2) explicit false reproduces the default;
# (3) on the preset's endgame the flag changes the run (the registry is exercised there).
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_DD19_22: the preset this gate was written against (DESIGN_DECISIONS 19-22)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
UNIT="$(pwd)/build/scratch/ns3.47-t29_interf_unit-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g153.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
bad=0
"$UNIT" || bad=1
if "$UNIT" --break=ignore > "$D/fix.txt" 2>&1; then echo "G153 FAIL FIXTURE  --break=ignore did not turn ON red"; bad=1; else echo "G153 PASS FIXTURE  --break=ignore is red"; fi
B="--specProfile=true $PIN_DD19_22 --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --boxHalf=750 --finderMode=1"
for n in base off on; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --interferenceDistance=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --interferenceDistance=true --outdir="$D/on" > "$D/on.log" 2>&1 &
wait
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G153 PASS DEFAULT  explicit false reproduces the default"; else echo "G153 FAIL DEFAULT  $n outputs differ"; bad=1; fi
if cmp -s "$D/base/trace_ca_tbl_full_seed7.csv" "$D/on/trace_ca_tbl_full_seed7.csv"; then echo "G153 FAIL FIRES    the run is unchanged with the flag on"; bad=1; else echo "G153 PASS FIRES    the run changes with the flag on"; fi
exit $bad
