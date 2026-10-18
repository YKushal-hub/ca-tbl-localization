#!/usr/bin/env bash
# G143: V6 --traceOnDemand and --onDemandCalibrate (master plan v3). usage: bash scripts/ondemand_cal_gate.sh [<workdir>]
#  DEFAULT  explicit false for --traceOnDemand and --onDemandCalibrate reproduces the default byte for byte
#  TRACE    --traceOnDemand writes ondemand_<arm>_seed7.csv with the pCal column, digested in run_id.txt
#  FIRES    with --dualBaseOnDemand --onDemandCalibrate the V6 line reports SGD updates > 0 and a, b moved off (1, 0)
#  NEG      without --onDemandCalibrate no V6 line is printed and pCal is -1 throughout
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g143.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
B="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --rngKeyed=true"
for n in base off raw cal; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --traceOnDemand=false --onDemandCalibrate=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --traceOnDemand=true --dualBaseOnDemand=true --outdir="$D/raw" > "$D/raw.log" 2>&1 &
"$BIN" $B --traceOnDemand=true --dualBaseOnDemand=true --onDemandCalibrate=true --onDemandPSkip=0.25 --outdir="$D/cal" > "$D/cal.log" 2>&1 &
wait
bad=0
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G143 PASS DEFAULT  explicit false reproduces the default"; else echo "G143 FAIL DEFAULT  $n outputs differ"; bad=1; fi
f="$D/cal/ondemand_ca_tbl_full_seed7.csv"
if [ -f "$f" ] && head -1 "$f" | grep -q pCal && grep -q "ondemand_ca_tbl_full_seed7.csv" "$D/cal/run_id.txt"; then echo "G143 PASS TRACE   ondemand csv with pCal, digested"; else echo "G143 FAIL TRACE   ondemand csv missing, without pCal, or not digested"; bad=1; fi
l=$(grep "V6 onDemandCalibrate" "$D/cal.log")
if echo "$l" | awk '{a=$4; b=$6; u=$8; gsub(",","",u); exit !(u > 0 && (a != 1 || b != 0))}'; then echo "G143 PASS FIRES    $l"; else echo "G143 FAIL FIRES    '$l'"; bad=1; fi
if ! grep -q "V6 onDemandCalibrate" "$D/raw.log" && awk -F, 'NR > 1 && $5 != "-1.000000" {bad = 1} END {exit bad}' "$D/raw/ondemand_ca_tbl_full_seed7.csv"; then echo "G143 PASS NEG     no calibration without the flag"; else echo "G143 FAIL NEG     calibration output without the flag"; bad=1; fi
exit $bad
