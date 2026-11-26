#!/usr/bin/env bash
# G169 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/benefit_rule_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g169self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/benefit_rule_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { fresh; s=$(grep -o "skipped [0-9]* (all" "$T/f/on.log" | grep -o "[0-9]*"); sed -i "s/skipped [0-9]* (all measured nodes)/skipped $s (all measured nodes)/" "$T/f/min3.log"; check "N9 the TII-2 condition never keeping a slot" 1 MIN3
fresh; sed -i '/TII-2 benefitSkip868Min3/d' "$T/f/min3.log"; check "N10 TII-2 not announced" 1 MIN3
echo "  G169 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G169 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G169 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G169 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
TR=trace_fixed_threshold_seed7.csv
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the flag-on run crashed" 1 RUNS
fresh; f=$(ls "$T/f/off" | grep -v run_id.txt | head -1); echo x >> "$T/f/off/$f"; check "N2 --benefitLambda=0 not the default" 1 OFF
fresh; sed -i '/TII-1 benefitSkip868/d' "$T/f/on.log"; check "N3 the mechanism not reported" 1 LINE
fresh; grep 'TII-1 benefitSkip868' "$T/f/on.log" >> "$T/f/abs.log"; check "N4 the mechanism reported with the flags absent" 1 LINE
fresh; f=$(ls "$T/f/on" | grep ca_tbl_full | head -1); echo x >> "$T/f/on/$f"; check "N5 the flags reaching ca_tbl_full" 1 ARMS
fresh; sed -i 's/skipped [0-9]* (all measured nodes)/skipped 0 (all measured nodes)/' "$T/f/on.log"; check "N6 no 868 MHz slot ever skipped" 1 SKIP
fresh; cp "$T/f/abs/arm_summary.csv" "$T/f/on/arm_summary.csv"; check "N7 868 MHz energy not reduced" 1 SKIP
fresh; cp "$T/f/abs/$TR" "$T/f/on/$TR"; check "N8 the trigger not changing behaviour" 1 RULE
fresh; s=$(grep -o "skipped [0-9]* (all" "$T/f/on.log" | grep -o "[0-9]*"); sed -i "s/skipped [0-9]* (all measured nodes)/skipped $s (all measured nodes)/" "$T/f/min3.log"; check "N9 the TII-2 condition never keeping a slot" 1 MIN3
fresh; sed -i '/TII-2 benefitSkip868Min3/d' "$T/f/min3.log"; check "N10 TII-2 not announced" 1 MIN3
echo "  G169 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
