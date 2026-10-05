#!/usr/bin/env bash
# G76 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/binary_stamp_gate_selftest.sh [<dir written by binary_stamp_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/binary_stamp_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g76self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/binary_stamp_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/run_id.txt" ] || { echo "  G76 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G76 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G76 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G76 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G76 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G76 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set passes (outside the repository)" 0 -
fresh; sed -i -E 's/^(binary sha256 )[0-9a-f]{8}/\100000000/' "$T/f/on/run_id.txt"
check "N1 a stamp that is not the binary's hash" 1 STAMP
fresh; sed -i '/^binary sha256 /d' "$T/f/copy/run_id.txt"
check "N2 the copy run without a stamp" 1 COPY
fresh; f=$(ls "$T"/f/on/trace_*.csv | head -1); sed -i '2s/,/,9/' "$f"
check "N3 the flag changing an output" 1 SAME
fresh; grep '^binary sha256 ' "$T/f/on/run_id.txt" >> "$T/f/off/run_id.txt"
check "N4 the control stamping" 1 CONTROL
fresh; echo 134 > "$T/f/copy.rc"
check "N5 a run the checks read did not exit 0" 1 RUNS

printf '  G76 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
