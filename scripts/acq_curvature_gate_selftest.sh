#!/usr/bin/env bash
# G98 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/acq_curvature_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g98self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/acq_curvature_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.log" ] || { echo "  G98 selftest: no real run set in $REAL"; exit 2; }
pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G98 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G98 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G98 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the guarded run crashed" 1 RUNS
fresh; sed -i -E 's/(kept the cloud )[0-9]+/\10/' "$T/f/on.log"; check "N2 a guard that never held" 1 REACHED
fresh; sed -i '/curvature guard/d' "$T/f/on.log"; check "N3 the flag silently not taking effect" 1 REACHED
fresh; cp "$T/f/base/arm_summary.csv" "$T/f/on/arm_summary.csv"; check "N4 a guard that held but changed nothing" 1 DIFFERS
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N5 the default not being false" 1 OFF
fresh; grep "curvature guard" "$T/f/on.log" >> "$T/f/base.log"; check "N6 the guard leaking into the default" 1 CONTROL
fresh; echo 0 > "$T/f/r_nopf.rc"; check "N7 accepted without --acquireParticles" 1 REFUSE
echo "  G98 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
