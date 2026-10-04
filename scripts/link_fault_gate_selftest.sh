#!/usr/bin/env bash
# G37 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/link_fault_gate_selftest.sh [<dir written by link_fault_gate.sh>]
#   Without a directory it makes its own real run set first (about 40 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N3  the fault acting before its window opens;
#   N5  the RSSI skew computed but never applied;
#   N6  a transmit-power drop attenuating what the node receives.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/link_fault_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g37self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/link_fault_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/loss.log" ] || { echo "  G37 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G37 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G37 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G37 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G37 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G37 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set" 0 -

fresh; sed -i '3s/[0-9]$/9/' "$T/f/zero/trace_ca_tbl_full_seed7.csv"
check "N0 magnitudes of 0 move an output" 1 NULL
fresh; grep "SY-5" "$T/f/drop.log" >> "$T/f/def.log"
check "N1 the no-fault run prints the SY-5 line" 1 NULL
fresh; sed -i 's/window \[225, 495) s/window [200, 470) s/' "$T/f/drop.log"
check "N2 the window not the seeded one" 1 WINDOW
fresh; python3 - "$T/f/drop/trace_ca_tbl_full_seed7.csv" <<'PY'
import sys
p = sys.argv[1]
lines = open(p).read().split("\n")
for i, l in enumerate(lines[1:], 1):
    if l.startswith("100.0000,"):
        f = l.split(","); f[9] = "%.4f" % (float(f[9]) + 5.0); lines[i] = ",".join(f); break
open(p, "w").write("\n".join(lines))
PY
check "N3 the fault acting before its window opens" 1 WINDOW
fresh; sed -i 's/tier-1 skew x2.72133877/tier-1 skew x1/' "$T/f/loss.log"
check "N4 a skew factor that is not 10^(dB/(10 n))" 1 SKEW
fresh; sed -i 's/tier-1 ranges skewed [0-9]*;/tier-1 ranges skewed 0;/' "$T/f/drop.log"
check "N5 the RSSI skew computed but never applied" 1 SKEW
fresh; sed -i 's/MAC links attenuated tx \([0-9]*\) rx 0/MAC links attenuated tx \1 rx 17/' "$T/f/drop.log"
check "N6 a transmit-power drop attenuating what the node receives" 1 DIRECT
fresh; sed -i 's/MAC links attenuated tx \([0-9]*\) rx [0-9]*/MAC links attenuated tx \1 rx 0/' "$T/f/loss.log"
check "N7 an antenna loss that spares reception" 1 DIRECT
fresh; echo 0 > "$T/f/r_nowin.rc"
check "N8 a magnitude accepted without its window" 1 R
fresh; echo 134 > "$T/f/loss.rc"
check "N9 a run the checks read did not exit 0" 1 RUNS

printf '  G37 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
