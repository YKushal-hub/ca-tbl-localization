#!/usr/bin/env bash
# G77 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_tdoa_gate_selftest.sh [<dir written by subject_tdoa_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_tdoa_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g77self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_tdoa_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G77 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G77 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G77 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G77 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G77 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G77 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set passes" 0 -
fresh; rm "$T"/f/on/subjtdoa_*_subj5.csv
check "N1 a subject without its TDoA file" 1 FILES
fresh; sed -i -E 's/(TDoA tier 1 \(subject [0-9]+\): [^;]*; solves OK )[0-9]+/\10/' "$T/f/on.log"
check "N2 no subject solving (or a line the file does not support)" 1 SOLVES
fresh; sed -i -E '0,/(TDoA tier 1 \(subject [0-9]+\): .*; fused )([0-9]+) \(policy/s//\199999 (policy/' "$T/f/on.log"
check "N3 a subject's fused count its policy does not hold" 1 FUSED
fresh; sed -i -E '0,/(TDoA billing \(subject [0-9]+\): .*stamps beyond a billed reply )0/s//\13/' "$T/f/on.log"
check "N4 a subject stamp with no billed reply" 1 BILL
fresh; sed -i -E 's/TDoA tier 1: fix cycles/TDoA tier 1 (subject 1): fix cycles/' "$T/f/on.log"
check "N5 tag 0's line relabelled" 1 TAG0
fresh; cp "$T"/f/on/subjtdoa_*_subj2.csv "$T/f/off/"
check "N6 the control writing subject TDoA" 1 CONTROL
fresh; echo 0 > "$T/f/r_notdoa.rc"
check "N7 the flag accepted without --tier1Tdoa" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G77 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
