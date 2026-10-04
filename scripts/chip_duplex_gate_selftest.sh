#!/usr/bin/env bash
# G84 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/chip_duplex_gate_selftest.sh [<dir written by chip_duplex_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N2: the two bands of one chip
# never seeing each other, which is the per-radio model the flag replaces.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/chip_duplex_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g84self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/chip_duplex_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/countchip.log" ] || { echo "  G84 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G84 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G84 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G84 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G84 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G84 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set passes" 0 -
fresh; f=$(ls "$T"/f/countchip/trace_*.csv | head -1); sed -i '2s/,/,9/' "$f"
check "N1 count-only per chip changing an output" 1 COUNT
fresh; sed -i -E 's/(receptions while the other band transmitted )[0-9]+( \(kept\))/\10\2/' "$T/f/countchip.log"
check "N2 the two bands of one chip never seeing each other" 1 COUNT
fresh; sed -i -E 's/(N\.6 half-duplex \(count, \S+\): frames due over the radio.s own [0-9]+, sent back to CAD 0, receptions while transmitting )[0-9]+/\11/' "$T/f/countchip.log"
check "N3 a chip line the N.6 line does not add up to" 1 COUNT
fresh; sed -i -E 's/(frames due over the other band.s )[0-9]+(, receptions while the other band transmitted [0-9]+ \(lost\))/\1999\2/' "$T/f/enforcechip.log"
check "N4 an enforced frame started over the other band's" 1 ENFORCE
fresh; sed -i '/,halfduplex,/d' "$T/f/enforcechip/events_ca_tbl_full_seed7.csv"
check "N5 lost receptions with no events row" 1 ENFORCE
fresh; grep 'N.6 chip' "$T/f/countchip.log" >> "$T/f/count.log"
check "N6 the control reporting a chip" 1 CONTROL
fresh; echo 0 > "$T/f/r_sx.rc"
check "N7 the flag accepted on the SX basis" 1 R
fresh; echo 134 > "$T/f/countchip.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G84 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
