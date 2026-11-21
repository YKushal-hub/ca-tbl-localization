#!/usr/bin/env bash
# G174 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/search_doctrine_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g174self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/search_doctrine_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G174 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G174 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G174 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G174 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
N=nodes_static_2p4_seed501.csv
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the fixed run crashed" 1 RUNS
fresh; sed -i '/DD32/d' "$T/f/on.log"; check "N2 one fix not announced" 1 LINES
fresh; cp "$T/f/off/$N" "$T/f/on/$N"; check "N3 the jump still there with the fixes on" 1 JUMP
fresh; cp "$T/f/on/$N" "$T/f/off/$N"; check "N4 no jump to remove (off already fine)" 1 JUMP
fresh; awk -F, -v OFS=, 'NR==1 || $2<500 || $1+0<3000 {print; next} {print $1,$2,$3+0+40,$4}' "$REAL/on/$N" > "$T/f/on/$N"; check "N5 the party never closing in" 1 STEER
echo "  G174 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
