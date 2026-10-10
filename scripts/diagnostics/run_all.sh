#!/usr/bin/env bash
# Orchestrator. Deliberately no `set -e`: no module outcome may stop the batch.
cd "$(dirname "$0")/../.." || exit 1   # repository root
export DIAG_ROOT="${DIAG_ROOT:?}"
mkdir -p "$DIAG_ROOT/raw"
STATUS="$DIAG_ROOT/STATUS.txt"; : > "$STATUS"
run_mod() {
  name=$1; script=$2; mins=$3
  s=$(date +%s)
  if [ ! -x "$script" ]; then
    echo "$name ERROR module absent 0s" >> "$STATUS"; return 0
  fi
  timeout "${mins}m" "$script" > "$DIAG_ROOT/raw/$name.log" 2>&1
  rc=$?
  e=$(( $(date +%s) - s ))
  if [ $rc -eq 124 ]; then st=TIMEOUT
  elif [ $rc -ne 0 ]; then st=ERROR
  else
    f="$DIAG_ROOT/$name.md"
    if [ -f "$f" ]; then st=$(grep -m1 '^STATUS:' "$f" | awk '{print $2}'); st=${st:-OK}
    else st=ERROR; fi
  fi
  echo "$name $st rc=$rc ${e}s" >> "$STATUS"
}
run_mod D0 scripts/diagnostics/D0_preflight.sh      5
run_mod D2 scripts/diagnostics/D2_flag_confound.sh 10
run_mod D4 scripts/diagnostics/D4_provenance.sh    30
run_mod D6 scripts/diagnostics/D6_docs.sh          15
run_mod D8 scripts/diagnostics/D8_build_hygiene.sh 15
run_mod D5 scripts/diagnostics/D5_gates.sh         60
run_mod D1 scripts/diagnostics/D1_arm_liveness.sh 120
run_mod D3 scripts/diagnostics/D3_degeneracy.sh    60
run_mod D7 scripts/diagnostics/D7_clean_integrity.sh 420
git status --porcelain --untracked-files=no > "$DIAG_ROOT/tree_end.txt" 2>&1
python3 scripts/diagnostics/summarise.py "$DIAG_ROOT" > "$DIAG_ROOT/SUMMARY.md" 2>&1
echo "BATCH_DONE" >> "$STATUS"
