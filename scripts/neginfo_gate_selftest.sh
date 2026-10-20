#!/usr/bin/env bash
# G162 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/neginfo_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g162self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/neginfo_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G162 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G162 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G162 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G162 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
first() { ls "$T/f/on"/neginfo_ca_tbl_full_seed205_subj*.csv | head -1; }
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the traced run crashed" 1 RUNS
fresh; rm "$(first)"; check "N2 a subject's trace missing" 1 FILES
fresh; f=$(first); sed -i '1s/,trueDistM$//' "$f"; check "N3 the header changed" 1 FILES
fresh; f=$(first); awk -F, 'BEGIN{OFS=","} NR==2{$3="868MHz"} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N4 a row on the 868 MHz floor" 1 ROWS
fresh; f=$(first); awk -F, 'BEGIN{OFS=","} NR==2{$5="151.00"} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N5 a row past the 150 m radius" 1 ROWS
fresh; echo x >> "$T/f/on/arm_summary.csv"; check "N6 the trace changing another output" 1 INERT
fresh; cp "$(first)" "$T/f/base/"; check "N7 the default writing the trace" 1 OFF
echo "  G162 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
