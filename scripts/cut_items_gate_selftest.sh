#!/usr/bin/env bash
# G166 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/cut_items_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g166self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/cut_items_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/buck" ] || { echo "  G166 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G166 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G166 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G166 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
col() { head -1 "$1" | tr ',' '\n' | grep -nx "$2" | cut -d: -f1; }
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/apc.rc"; check "N1 a flag's run crashed" 1 RUNS
fresh; f="$T/f/buck/subject_summary.csv"; c=$(col "$f" idle_uwb); cut -d, -f"$c" --complement "$f" > "$f.t" && mv "$f.t" "$f"; check "N2 a bucket column missing" 1 BUCKET
fresh; f="$T/f/buck/subject_summary.csv"; c=$(col "$f" rx_868); awk -F, -v c="$c" 'BEGIN{OFS=","} NR==2{$c=$c+0.5} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N3 a bucket that does not close on j_tier" 1 BUCKET
fresh; sed -i 's/all measured nodes [0-9]* cycles/all measured nodes 0 cycles/' "$T/f/ca_on.log"; check "N4 Case A pre-fusion never taken" 1 CASEA
fresh; cp "$T/f/ca_off/arm_summary.csv" "$T/f/ca_on/arm_summary.csv"; check "N5 Case A pre-fusion without effect" 1 CASEA
fresh; sed -i 's/CAD decisions busy only because of them [0-9]*/CAD decisions busy only because of them 0/' "$T/f/apc.log"; check "N6 anchor-pair sensing never deciding a CAD" 1 APC
fresh; sed -i 's/stationary\/approaching\/receding \([0-9]*\)\/[0-9]*\/[0-9]*/stationary\/approaching\/receding \1\/0\/0/' "$T/f/mob.log"; check "N6b subjects always STATIONARY" 1 MOB
fresh; sed -i 's/RECEDING eager pushes [0-9]*/RECEDING eager pushes 0/' "$T/f/mob.log"; check "N6c the push never run for a subject" 1 MOB
fresh; sed -i 's/: [0-9]* MAC transmissions put back/: 0 MAC transmissions put back/' "$T/f/rs_on.log"; check "N6d the round still meeting no MAC frame" 1 SLICE
fresh; cp "$T/f/rs_off/arm_summary.csv" "$T/f/rs_on/arm_summary.csv"; check "N6e the round's MAC frames without effect" 1 SLICE
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N7 a flag not off by default" 1 OFF
fresh; echo 0 > "$T/f/r_ca.rc"; check "N8 pre-fusion accepted without --sparseInStep" 1 REFUSE
fresh; echo 0 > "$T/f/r_rs.rc"; check "N9 the round fix accepted without a round" 1 REFUSE
echo "  G166 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
