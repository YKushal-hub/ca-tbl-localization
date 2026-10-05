#!/usr/bin/env bash
# G82 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/build_stamp_gate_selftest.sh [<dir written by build_stamp_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N3: a binary built from another
# commit than the repository's, which is the M.3 hazard itself.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/build_stamp_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g82self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/build_stamp_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/run_id.txt" ] || { echo "  G82 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G82 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G82 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G82 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G82 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G82 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
R="$T/f/on/run_id.txt"

fresh
check "P0 the real run set passes" 0 -
fresh; sed -i '/^build source /d' "$R"
check "N1 no build line" 1 STAMP
fresh; sed -i -E 's/^(build source \S+ dirty )(-?[0-9]+)$/\1x\2/' "$R"
check "N2 a malformed build line" 1 STAMP
fresh; echo 0000000 > "$T/f/head.txt"
check "N3 a binary built from another commit than the repository's" 1 FRESH
fresh; echo 99 > "$T/f/dirty.txt"
check "N4 a binary built before the tree's last edit" 1 FRESH
fresh; sed -i -E 's/^source unknown /source deadbee /' "$R"
check "N5 a launch the source line could name" 1 UNKNOWN
fresh; echo x >> "$T/f/on/$(ls "$T/f/on" | grep -v run_id | head -1)"
check "N6 the flag changing an output" 1 SAME
fresh; grep '^build source ' "$R" >> "$T/f/off/run_id.txt"
check "N7 the control writing a build line" 1 CONTROL
fresh; echo 134 > "$T/f/on.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G82 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
