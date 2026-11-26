#!/usr/bin/env bash
# G170 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/tii3_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g170self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/tii3_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/f4" ] || { echo "  G170 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G170 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G170 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G170 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
TR=trace_fixed_threshold_seed7.csv
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/f2.rc"; check "N1 a flag-on run crashed" 1 RUNS
fresh; f=$(ls "$T/f/off" | grep -v run_id.txt | head -1); echo x >> "$T/f/off/$f"; check "N2 explicit off not the default" 1 OFF
fresh; sed -i '/TII-3 (fixed_threshold)/d' "$T/f/f1.log"; check "N3 F1 not announced" 1 LINE
fresh; grep 'TII-3 (fixed_threshold)' "$T/f/f1.log" >> "$T/f/base.log"; check "N4 announced with the flags absent" 1 LINE
fresh; f=$(ls "$T/f/f4" | grep ca_tbl_full | head -1); echo x >> "$T/f/f4/$f"; check "N5 a flag reaching ca_tbl_full" 1 ARMS
fresh; s=$(grep -o "skipped [0-9]* (all" "$T/f/b01.log" | grep -o "[0-9]*"); sed -i "s/skipped [0-9]* (all measured nodes)/skipped $s (all measured nodes)/" "$T/f/f1.log"; check "N6 F1 keeping no slot" 1 F1
fresh; sed -i "s/tag-0 mean NIS [0-9.]*/tag-0 mean NIS 1.0/" "$T/f/f1.log"; check "N6b F1 with nothing to scale" 1 F1
fresh; sed -i 's/forced [0-9]*/forced 0/' "$T/f/f2.log"; check "N7 F2 never forcing a slot" 1 F2
fresh; sed -i 's/vetoes [0-9]*/vetoes 0/' "$T/f/f4.log"; check "N8 F4 never vetoing" 1 F4
fresh; s=$(grep -o "skipped [0-9]* (all" "$T/f/base.log" | grep -o "[0-9]*"); sed -i "s/skipped [0-9]* (all measured nodes)/skipped $s (all measured nodes)/" "$T/f/f2.log"; check "N9 F2 not reducing skips" 1 F2
echo "  G170 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
