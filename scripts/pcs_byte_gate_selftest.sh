#!/usr/bin/env bash
# G159 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/pcs_byte_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g159self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/pcs_byte_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.log" ] || { echo "  G159 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G159 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G159 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G159 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the scaled run crashed" 1 RUNS
fresh; f=$(ls "$T/f/base"/session_ca_tbl_full_seed*.csv); awk -F, 'BEGIN{OFS=","} NR>1 && $4=="result"{$10="1.5"} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N2 the default byte carrying PCS above 1" 1 DEFECT
fresh; cp "$T/f/base"/session_ca_tbl_full_seed*.csv "$T/f/on/"; check "N3 the flag not taking effect" 1 CARRIES
fresh; f=$(ls "$T/f/on"/session_ca_tbl_full_seed*.csv); awk -F, 'BEGIN{OFS=","} NR>1 && $4=="result" && $10+0>=1.3 {$10="1.2"} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N4 nothing at or above the threshold" 1 REFUSABLE
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N5 the default not being 1" 1 OFF
fresh; echo 0 > "$T/f/r_low.rc"; check "N6 accepted a scale below 1" 1 REFUSE
fresh; echo 0 > "$T/f/r_high.rc"; check "N7 accepted a scale above 64" 1 REFUSE
echo "  G159 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
