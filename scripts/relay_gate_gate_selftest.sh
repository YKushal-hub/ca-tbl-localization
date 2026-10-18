#!/usr/bin/env bash
# G101 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/relay_gate_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g101self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/relay_gate_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.log" ] || { echo "  G101 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G101 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G101 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G101 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the gated run crashed" 1 RUNS
fresh; sed -i -E 's/(gated )[0-9]+/\10/' "$T/f/on.log"; check "N2 a gate that never fired" 1 REACHED
fresh; sed -i -E 's/relayed ranges ([0-9]+), gated [0-9]+/relayed ranges \1, gated \1/' "$T/f/on.log"; check "N3 a gate that dropped every relay" 1 REACHED
fresh; cp "$T/f/base/arm_summary.csv" "$T/f/on/arm_summary.csv"; check "N4 a gate that fired but changed nothing" 1 DIFFERS
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N5 the default not being false" 1 OFF
fresh; grep "relay gate" "$T/f/on.log" >> "$T/f/base.log"; check "N6 the gate leaking into the default" 1 CONTROL
fresh; echo 0 > "$T/f/r_cv.rc"; check "N7 accepted without ekf5" 1 REFUSE
fresh; echo 0 > "$T/f/r_rob.rc"; check "N8 accepted with a robust mode" 1 REFUSE
echo "  G101 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
