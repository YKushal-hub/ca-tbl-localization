#!/usr/bin/env bash
# G75 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/receding_push_gate_selftest.sh [<dir written by receding_push_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/receding_push_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g75self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/receding_push_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G75 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G75 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G75 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G75 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G75 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G75 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set passes" 0 -
fresh; sed -i -E 's/(push frames transmitted )[0-9]+/\10/' "$T/f/on.log"
check "N1 the report counting no push" 1 COUNT
fresh; sed -i -E 's/(push frames transmitted )[0-9]+/\199999/' "$T/f/on.log"
check "N2 more than three frames a push" 1 COUNT
fresh; sed -i '/,1,[0-9]*,TRIGGERED_UPDATE,/d' "$T/f/on/events_ca_tbl_full_seed7.csv"
check "N3 no push from node 1 heard" 1 HEARD
fresh; f=$(ls "$T"/f/on/trace_*.csv | head -1); sed -i '2s/,/,9/' "$f"
check "N4 the report changing an output" 1 SAME
fresh; grep 'M.9 RECEDING' "$T/f/on.log" >> "$T/f/off.log"
check "N5 the control reporting" 1 CONTROL
fresh; echo 0 > "$T/f/r_nomob.rc"
check "N6 the flag accepted without --mobilityFromRange" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G75 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
