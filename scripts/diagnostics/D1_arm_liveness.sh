#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
OUTD="$DIAG_ROOT/d1_runs"; mkdir -p "$OUTD"
# config: name|boxHalf|envFacility|nlosSource|dwell|extra
CONFIGS=(
"A|2000|0|legacy|90|"
"B|2000|0|propagation|90|"
"C|1500|0|legacy|800|"
"D|2000|1|legacy|800|"
"E|2000|0|propagation|90|--suspectForcesEscalation=false"
"F|2000|4|propagation|90|"
)
run_one() {
  IFS='|' read -r name box ef ns dw extra <<< "$1"
  d="$OUTD/$name"
  if [ -s "$d/arm_summary.csv" ] && [ "$(ls "$d"/trace_*.csv 2>/dev/null | wc -l)" = "6" ]; then return 0; fi
  rm -rf "$d"; mkdir -p "$d"
  timeout 40m $BIN --seed=1 --duration=3600 --boxHalf=$box --gwGrid=5 \
     --escalationRequiresEvidence=true --floorOnDemand=true --trace=true \
     --envFacility=$ef --nlosSource=$ns --dwell=$dw $extra \
     --outdir="$d" > "$d/stdout.log" 2>&1
  echo "$name rc=$?" >> "$OUTD/rc.txt"
}
export -f run_one; export OUTD BIN
printf '%s\n' "${CONFIGS[@]}" | xargs -P 6 -I{} bash -c 'run_one "{}"'
python3 scripts/diagnostics/d1_matrix.py "$OUTD" > "$DIAG_ROOT/D1.md" 2>&1
exit 0
