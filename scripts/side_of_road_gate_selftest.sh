#!/usr/bin/env bash
# G173 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/side_of_road_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g173self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/side_of_road_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G173 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G173 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G173 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G173 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
N=trace_static_2p4_seed501.csv
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the constrained run crashed" 1 RUNS
fresh; sed -i '/DD29/d' "$T/f/on.log"; check "N2 the flag not announced" 1 LINE
fresh; echo 0 > "$T/f/nor.rc"; check "N3 the flag accepted without a ridge" 1 REFUSE
fresh; cp "$T/f/off/$N" "$T/f/on/$N"; check "N4 the estimate crossing the line with the flag on" 1 SIDE
fresh; cp "$T/f/on/$N" "$T/f/off/$N"; check "N5 a seed with no mirror to remove" 1 SIDE
echo "  G173 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
