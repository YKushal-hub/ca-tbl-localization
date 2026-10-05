#!/usr/bin/env bash
# G86 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/airtime_gate_selftest.sh [<dir written by airtime_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N2: a report whose rows do not
# add up to the frames the channel carried.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/airtime_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g86self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/airtime_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/airtime_ca_tbl_full_seed7.csv" ] || { echo "  G86 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G86 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G86 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G86 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G86 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G86 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
A="$T/f/on/airtime_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; f=$(ls "$T"/f/on/trace_*.csv | head -1); sed -i '2s/,/,9/' "$f"
check "N1 the report changing an output" 1 SAME
fresh; sed -i -E '0,/^0,DISCOVERY,[0-9]+/s/^0,DISCOVERY,([0-9]+)/0,DISCOVERY,1\1/' "$A"
check "N2 rows that do not add up to the frames the channel carried" 1 TALLY
fresh; sed -i -E 's/^0,FULL_PUSH_CF,0,0,.*/0,FULL_PUSH_CF,0,0,0/' "$A"
check "N3 digests sent with no full-push counterfactual" 1 CF
fresh; sed -i -E 's/responses unicast [0-9]+, flooded [0-9]+/responses unicast 0, flooded 5/' "$T/f/on.log"
check "N4 the reverse-path run flooding its responses" 1 FLOOD
fresh; cp "$A" "$T/f/off/"
check "N5 the control writing an airtime report" 1 CONTROL
fresh; echo 134 > "$T/f/flood.rc"
check "N6 a run the checks read did not exit 0" 1 RUNS

printf '  G86 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
