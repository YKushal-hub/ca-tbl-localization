#!/usr/bin/env bash
# G96 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/ekf5_consider_gate_selftest.sh [<dir written by ekf5_consider_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The filter-level fixture (CONSIDER's gain-zeroing removed:
# the two mode-8 sparse cases of ca_tbl_imu_test fail, 17 of 19) is recorded in TECH_DEBT N.33.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/ekf5_consider_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g96self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/ekf5_consider_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G96 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G96 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G96 ok    %-60s exit=%d\n' "$1" "$rc"
        pass=$((pass + 1))
    else
        printf '  G96 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G96 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the mode-8 run crashed" 1 RUNS
fresh; sed -i '/N.33 ekf5 mode/d' "$T/f/on.log"; check "N2 the flag silently not taking effect" 1 LINE
fresh; sed -i -E 's/considered [0-9]+/considered 0/' "$T/f/on.log"; check "N3 a mode that never fired" 1 REACHED
fresh; sed -i -E 's/gated [0-9]+,/gated 7,/' "$T/f/on.log"; check "N4 the gate bit set alongside CONSIDER" 1 REACHED
fresh; sed -i -E 's/heading offset est \S+ \(true (\S+)\)/heading offset est \1 (true \1)/' "$T/f/base.log"; check "N5 a base run that never went wrong" 1 DECIDES
fresh; sed -i -E 's/heading offset est \S+ \(true/heading offset est 9.0 (true/' "$T/f/on.log"; check "N6 the mode not changing the outcome" 1 DECIDES
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N7 the default not being mode 0" 1 OFF
fresh; grep "N.33 ekf5 mode" "$T/f/on.log" >> "$T/f/def.log"; check "N8 the mode leaking into the default" 1 CONTROL
fresh; echo 0 > "$T/f/r_cv.rc"; check "N9 the mode accepted without ekf5" 1 REFUSE
fresh; echo 0 > "$T/f/r_range.rc"; check "N10 an out-of-range mode accepted" 1 REFUSE
echo "  G96 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
