#!/usr/bin/env bash
# G74 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/half_duplex_gate_selftest.sh [<dir written by half_duplex_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/half_duplex_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g74self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/half_duplex_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/enforce.log" ] || { echo "  G74 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G74 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G74 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G74 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G74 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G74 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set passes" 0 -
fresh; f=$(ls "$T"/f/count/trace_*.csv | head -1); sed -i '2s/,/,9/' "$f"
check "N1 count-only mode changing an output" 1 COUNT
fresh; sed -i -E 's/(receptions while transmitting )[0-9]+( \(kept\))/\10\2/' "$T/f/count.log"
check "N2 the defect never occurring at defaults" 1 COUNT
fresh; sed -i -E 's/(sent back to CAD )[0-9]+(, receptions while transmitting [0-9]+ \(lost\))/\10\2/' "$T/f/enforce.log"
check "N3 an enforced frame started over the radio's own" 1 ENFORCE
fresh; sed -i '/,halfduplex,/d' "$T/f/enforce/events_ca_tbl_full_seed7.csv"
check "N4 deaf receptions with no events row" 1 ENFORCE
fresh; grep 'N.6 half-duplex' "$T/f/count.log" >> "$T/f/off.log"
check "N5 the control reporting half-duplex" 1 CONTROL
fresh; echo 0 > "$T/f/r_bad.rc"
check "N6 --halfDuplex=3 accepted" 1 R
fresh; echo 134 > "$T/f/enforce.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G74 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
