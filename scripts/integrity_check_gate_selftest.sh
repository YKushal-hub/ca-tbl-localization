#!/usr/bin/env bash
# G163 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/integrity_check_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g163self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/integrity_check_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/off" ] || { echo "  G163 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G163 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G163 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G163 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/off.rc"; check "N1 the switched-off run crashed" 1 RUNS
fresh; f="$T/f/off/trace_ca_tbl_full_seed7.csv"; awk -F, 'BEGIN{OFS=","} NR==2{$5=$5+1} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N2 the switched-off full arm differing from no-integrity" 1 EQUIV
fresh; f="$T/f/off/arm_summary.csv"; awk -F, 'BEGIN{OFS=","} $1=="ca_tbl_full"{$3=$3+1} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N3 the summary rows differing" 1 EQUIV
fresh; f="$T/f/on/arm_summary.csv"; c=$(head -1 "$f" | tr ',' '\n' | grep -nx suspects | cut -d: -f1); awk -F, -v c="$c" 'BEGIN{OFS=","} NR>1{$c=0} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N4 the check never flagging" 1 EFFECT
fresh; f="$T/f/off/arm_summary.csv"; c=$(head -1 "$f" | tr ',' '\n' | grep -nx suspects | cut -d: -f1); awk -F, -v c="$c" 'BEGIN{OFS=","} $1=="ca_tbl_full"{$c=5} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N5 the check still flagging when off" 1 EFFECT
fresh; f=$(ls "$T/f/tru" | grep -v -e run_id.txt -e spec_profile.txt | head -1); echo x >> "$T/f/tru/$f"; check "N6 true not being the check as it was" 1 OFF
echo "  G163 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
