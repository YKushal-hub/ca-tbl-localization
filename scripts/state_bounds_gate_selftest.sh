#!/usr/bin/env bash
# G102 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/state_bounds_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g102self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/state_bounds_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.log" ] || { echo "  G102 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G102 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G102 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G102 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the bounded run crashed" 1 RUNS
fresh; sed -i -E 's/(projections )[0-9]+/\10/' "$T/f/on.log"; check "N2 a projection that never fired" 1 REACHED
fresh; sed -i -E 's/scale est \S+ \(true/scale est -0.34 (true/' "$T/f/on.log"; check "N3 a bound that did not hold" 1 BOUNDED
fresh; sed -i -E 's/scale est \S+ \(true/scale est 0.001 (true/' "$T/f/base.log"; check "N4 a cell where the bound had nothing to do" 1 BOUNDED
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N5 the default not being 0" 1 OFF
fresh; grep "state bounds" "$T/f/on.log" >> "$T/f/def.log"; check "N6 the projection leaking into the default" 1 CONTROL
fresh; echo 0 > "$T/f/r_cv.rc"; check "N7 accepted without ekf5" 1 REFUSE
fresh; echo 0 > "$T/f/r_neg.rc"; check "N8 a negative value accepted" 1 REFUSE
echo "  G102 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
