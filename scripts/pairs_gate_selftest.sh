#!/usr/bin/env bash
# G160 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/pairs_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g160self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/pairs_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.log" ] || { echo "  G160 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G160 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G160 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G160 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the paired run crashed" 1 RUNS
fresh; sed -i '/MS-1 tasking (pairs):/s/-> subject \([0-9]*\) sweep/-> subject 9\1 sweep/2' "$T/f/on.log"; check "N2 a pair split across subjects" 1 PAIRS
fresh; sed -i '/MS-1 tasking (pairs):/d' "$T/f/on.log"; check "N3 the pairs line missing" 1 PAIRS
fresh; sed -i '/MS-1 tasking (roundrobin):/s/-> subject \([0-9]*\) sweep \(.*\)-> subject [0-9]* sweep/-> subject \1 sweep \2-> subject \1 sweep/' "$T/f/base.log"; check "N4 roundrobin sending two finders to one subject" 1 ROBIN
fresh; cp "$T/f/base"/trace_ca_tbl_full_seed*.csv "$T/f/on/"; check "N5 the flag not acting" 1 EFFECT
fresh; f=$(ls "$T/f/zero" | grep -v -e run_id.txt -e spec_profile.txt | head -1); echo x >> "$T/f/zero/$f"; check "N6 the preset default not roundrobin" 1 OFF
fresh; sed -i 's/^trace_ma/x/; 1s/^/x/' "$T/f/zero/spec_profile.txt"; check "N6b spec_profile.txt differing beyond the finderTasking line" 1 OFF
fresh; echo 0 > "$T/f/r_name.rc"; check "N7 accepted an unknown tasking" 1 REFUSE
fresh; echo 0 > "$T/f/r_none.rc"; check "N8 accepted pairs with no finders" 1 REFUSE
echo "  G160 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
