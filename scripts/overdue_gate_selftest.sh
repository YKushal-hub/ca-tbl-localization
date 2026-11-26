#!/usr/bin/env bash
# G171 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/overdue_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g171self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/overdue_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/od" ] || { echo "  G171 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G171 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G171 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G171 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
N=nodes_static_2p4_seed7.csv
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/od.rc"; check "N1 the overdue run crashed" 1 RUNS
fresh; sed -i '/reported overdue/d' "$T/f/od.log"; check "N2 the dispatch not reported" 1 LINE
fresh; echo "    ALERT: reported overdue -- party deployed at t=1.0 s" >> "$T/f/none.log"; check "N3 a dispatch with no alarm set" 1 LINE
fresh; cp "$T/f/none/$N" "$T/f/od/$N"; check "N4 the searchers never leaving after the alarm" 1 MOVE
fresh; cp "$T/f/od/$N" "$T/f/none/$N"; check "N5 the party waiting with no alarm configured" 1 MOVE
echo "  G171 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
