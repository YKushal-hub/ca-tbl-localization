#!/usr/bin/env bash
# G155: T46 --rssiRangeFromRxPower (TECH_DEBT O.9). usage: bash scripts/rssi_rxpower_gate.sh [<workdir>]
# DEFAULT explicit false reproduces the default; FIRES static_868's trace changes; SCOPE static_2p4 byte-identical;
# REQ with --rssiRangeFromShadowing=false the flag is inert (its documented precondition) -- also the fixture that
# shows the FIRES comparison can say "no effect".
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD19: the preset this gate was written against (DESIGN_DECISIONS 19-24)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g155.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
B="--specProfile=true $PIN_PRE_DD19 --arms=static_868,static_2p4 --seed=7 --duration=600 --trace=true --unknownStart=true"
for n in base off on nos nos_on; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --rssiRangeFromRxPower=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --rssiRangeFromRxPower=true --outdir="$D/on" > "$D/on.log" 2>&1 &
"$BIN" $B --rssiRangeFromShadowing=false --outdir="$D/nos" > "$D/nos.log" 2>&1 &
"$BIN" $B --rssiRangeFromShadowing=false --rssiRangeFromRxPower=true --outdir="$D/nos_on" > "$D/nos_on.log" 2>&1 &
wait
bad=0
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G155 PASS DEFAULT  explicit false reproduces the default"; else echo "G155 FAIL DEFAULT  $n outputs differ"; bad=1; fi
if cmp -s "$D/base/trace_static_868_seed7.csv" "$D/on/trace_static_868_seed7.csv"; then echo "G155 FAIL FIRES    static_868 unchanged"; bad=1; else echo "G155 PASS FIRES    static_868 changes"; fi
if cmp -s "$D/base/trace_static_2p4_seed7.csv" "$D/on/trace_static_2p4_seed7.csv"; then echo "G155 PASS SCOPE    static_2p4 untouched"; else echo "G155 FAIL SCOPE    static_2p4 changed"; bad=1; fi
if cmp -s "$D/nos/trace_static_868_seed7.csv" "$D/nos_on/trace_static_868_seed7.csv"; then echo "G155 PASS REQ      inert without --rssiRangeFromShadowing (the comparison can say 'no effect')"; else echo "G155 FAIL REQ      fired without its precondition"; bad=1; fi
exit $bad
