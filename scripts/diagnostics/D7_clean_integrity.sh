#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
OUT="$DIAG_ROOT/D7.md"
D7DIR=results/diag_d7_clean_integrity
# Pre-check from D1 config E: if ca_tbl_full has 0 escalations there, D7 is pointless.
ESC=$(python3 - <<'PY'
import csv,os
f="results/diagnostic_20260829/d1_runs/E/arm_summary.csv"
if not os.path.exists(f) or os.path.getsize(f)==0: print("NODATA")
else:
    v="NODATA"
    for r in csv.DictReader(open(f)):
        if r["arm"]=="ca_tbl_full": v=r.get("escalations","0")
    print(v)
PY
)
ESC=$(echo "$ESC" | tail -1)
if [ "$ESC" = "NODATA" ]; then
  { echo "# D7 — clean integrity test"; echo; echo "**SKIPPED.** D1 config E produced no arm_summary, so the pre-check could not be run."; echo; echo "STATUS: SKIPPED"; } > "$OUT"; exit 0
fi
if [ "$ESC" = "0" ]; then
  { echo "# D7 — clean integrity test"; echo; echo "**SKIPPED**, and this is the registered reason."; echo;
    echo "D1 config E measured \`ca_tbl_full\` with **0 escalations** under"
    echo "\`--suspectForcesEscalation=false\`. Without the suspect trigger the arm does not"
    echo "escalate either, so both arms are degenerate and the comparison is void in the"
    echo "other direction. Running it would produce two tier-1-locked arms and a"
    echo "meaningless equality."; echo; echo "STATUS: SKIPPED"; } > "$OUT"; exit 0
fi
mkdir -p "$D7DIR"
cat > "$D7DIR/PREDICTION.md" <<'PRED'
# D7 — clean integrity test: pre-registration

Written before the first run. Source: HEAD at the time of the diagnostic batch.

## What this isolates

`--suspectForcesEscalation=false` removes the side effect documented at
`scratch/ca_tbl_causal_sim.cc:3439-3449`, under which a suspect fix also forces an
escalation. With the flag false, a suspect fix is REJECTED and nothing else
happens — which is the mechanism the report actually claims.

The B2 observation (`ca_tbl_no_integrity` 0/20 under propagation) was produced with
this flag at its default TRUE and therefore measured escalation, not rejection.

## Registered prediction

With the escalation side effect removed, the `ca_tbl_full` - `ca_tbl_no_integrity`
availability gap will be SMALL (< 0.05 mean) at every cell, because rejection alone
subtracts a fix without supplying a better one. If the gap is large, rejection
alone is load-bearing and that is a genuine result.

## Degeneracy guard

If `ca_tbl_full` also fails to escalate under this flag, both arms are tier-1
locked and the comparison is void in the other direction. D1 config E is the
pre-check; this file only exists because that pre-check passed.
PRED
run_one() {
  set -- $1
  ef=$1; ns=$2; dw=$3; b=$4; s=$5
  d=$D7DIR/c_ef${ef}_${ns}_dw${dw}_b${b}/seed_$s
  if [ -s "$d/arm_summary.csv" ] && [ "$(ls "$d"/trace_*.csv 2>/dev/null | wc -l)" = "3" ]; then return 0; fi
  rm -rf "$d"; mkdir -p "$d"
  timeout 40m $BIN --seed=$s --duration=3600 --boxHalf=$b --gwGrid=5 \
    --escalationRequiresEvidence=true --floorOnDemand=true --suspectForcesEscalation=false \
    --envFacility=$ef --nlosSource=$ns --dwell=$dw \
    --arms=ca_tbl_full,ca_tbl_no_integrity,static_2p4 --trace=true \
    --outdir="$d" > "$d/stdout.log" 2>&1
  echo "ef${ef}_${ns}_dw${dw}_b${b}_s${s} rc=$?" >> $D7DIR/rc.txt
}
export -f run_one; export BIN D7DIR
for combo in "0 legacy 90" "0 propagation 90" "1 legacy 90" "1 legacy 800" "1 propagation 90" "1 propagation 800"; do
  for b in 2000 1500; do for s in 1 2 3 4 5; do echo "$combo $b $s"; done; done
done | xargs -P 11 -I{} bash -c 'run_one "{}"'
python3 scripts/diagnostics/d7_report.py "$D7DIR" > "$OUT" 2>&1
exit 0
