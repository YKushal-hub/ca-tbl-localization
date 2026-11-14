#!/usr/bin/env bash
# G168 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/tdoa_timing_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g168self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/tdoa_timing_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/t0" ] || { echo "  G168 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G168 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G168 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G168 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
TD=tdoa_static_868_seed7.csv
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/t0.rc"; check "N1 the scaled run crashed" 1 RUNS
fresh; f=$(ls "$T/f/one" | grep -v run_id.txt | head -1); echo x >> "$T/f/one/$f"; check "N2 a scale of 1 not exact" 1 OFF
fresh; sed -i '/TDOA TOA SCALE/d' "$T/f/t0.log"; check "N3 the probe not announced" 1 LINE
fresh; echo "TDOA TOA SCALE (--tdoaToaScale=1): x" >> "$T/f/one.log"; check "N4 a scale of 1 announced" 1 LINE
fresh; cp "$T/f/abs/$TD" "$T/f/t0/$TD"; check "N5 the scale not reaching the stamps" 1 EFFECT
fresh; head -3 "$T/f/t0/$TD" > "$T/f/x" && mv "$T/f/x" "$T/f/t0/$TD"; check "N6 too few solves to judge" 1 EFFECT
fresh; echo 0 > "$T/f/r0.rc"; check "N7 a zero scale accepted" 1 REFUSE
echo "  G168 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
