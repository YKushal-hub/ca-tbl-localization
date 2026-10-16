#!/usr/bin/env bash
# G138 negative fixtures (rule 6). usage: bash scripts/rngkeyed_gate_selftest.sh [<dir written by rngkeyed_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/rngkeyed_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g138self.XXXXXX") || exit 2; trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/rngkeyed_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.rc" ] || { echo "  G138 selftest: no real run set"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() { python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G138 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then printf '  G138 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else printf '  G138 FAIL  %-60s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G138 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1)); fi; }
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/onp"; cp -r "$REAL/offp" "$T/f/onp"; cp "$REAL/on.log" "$T/f/onp.log"; check "N1 an unkeyed perturbed run offered as the keyed one" 1 INVARIANT
fresh; sed -i 's/RNGKEYED unkeyed_draws=0/RNGKEYED unkeyed_draws=17/' "$T/f/onp.log"; check "N2 draws outside any link context" 1 RESIDUE
fresh; grep -v RNGKEYED "$REAL/on.log" > "$T/f/on.log"; check "N3 the keyed run prints no residue line" 1 RESIDUE
fresh; rm -rf "$T/f/offp"; cp -r "$REAL/off" "$T/f/offp"; check "N4 the perturbation moves nothing even unkeyed" 1 REAL
fresh; cp "$REAL/off/arm_summary.csv" "$T/f/on/arm_summary.csv"; cp "$REAL/off/arm_summary.csv" "$T/f/onp/arm_summary.csv"; check "N5 keyed mode changed nothing" 1 EFFECT
fresh; echo 1 > "$T/f/onp.rc"; check "N6 a run did not exit 0" 1 RUNS
printf '  G138 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
