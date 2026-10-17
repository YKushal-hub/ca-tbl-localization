#!/usr/bin/env bash
# G95 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/ekf5_gate_selftest.sh [<dir written by ekf5_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/ekf5_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g95self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/ekf5_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G95 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G95 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G95 ok    %-60s exit=%d\n' "$1" "$rc"
        pass=$((pass + 1))
    else
        printf '  G95 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G95 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the ekf5 run crashed" 1 RUNS
fresh; sed -i '/N.28 ekf5:/d' "$T/f/on.log"; check "N2 the flag silently not taking effect" 1 LINE
fresh; sed -i -E 's/N\.28 ekf5: moving ([0-9]+)/N.28 ekf5: moving 1/' "$T/f/on.log"; check "N3 predictions that bypassed ekf5" 1 WIRED
fresh; sed -i -E 's/bias est \S+ \(true/bias est 0 (true/' "$T/f/on.log"; check "N4 a bias never learned" 1 LEARNS
fresh; sed -i -E 's/heading offset est \S+ \(true/heading offset est 3.0 (true/' "$T/f/on.log"; check "N5 a heading offset never learned" 1 LEARNS
fresh; f=$(ls "$T/f/cv" | grep -v run_id.txt | head -1); echo x >> "$T/f/cv/$f"; check "N6 the default not being cv" 1 OFF
fresh; grep "N.28 ekf5:" "$T/f/on.log" >> "$T/f/def.log"; check "N7 ekf5 leaking into the default" 1 CONTROL
fresh; echo 0 > "$T/f/r_noimu.rc"; check "N8 ekf5 accepted without --imuPredict" 1 REFUSE
fresh; echo 0 > "$T/f/r_bogus.rc"; check "N9 an unknown --imuFilter accepted" 1 REFUSE
echo "  G95 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
