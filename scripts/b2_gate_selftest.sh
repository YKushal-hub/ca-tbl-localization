#!/usr/bin/env bash
# G136 negative fixtures (rule 6). usage: bash scripts/b2_gate_selftest.sh [<dir written by b2_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/b2_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g136self.XXXXXX") || exit 2; trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/b2_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G136 selftest: no real run set"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() { python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G136 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then printf '  G136 ok    %-56s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else printf '  G136 FAIL  %-56s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G136 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1)); fi; }
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/on" "$T/f/on.log"; cp -r "$REAL/base" "$T/f/on"; cp "$REAL/base.log" "$T/f/on.log"; check "N1 the control offered as the flag-on run" 1 FIRES
fresh; echo x >> "$T/f/off/arm_summary.csv"; check "N2 the explicit default changes an output" 1 DEFAULT
fresh; echo "    B2 dualBaseOnDemand: tag-0 cycles decided 1, 2.4 GHz radio asleep 1" >> "$T/f/base.log"; check "N3 the control prints a firing" 1 FIRES
fresh; cp "$REAL/base/arm_summary.csv" "$T/f/on/arm_summary.csv"; check "N4 no energy saved" 1 ENERGY
fresh; echo 1 > "$T/f/on.rc"; check "N5 a run did not exit 0" 1 RUNS
printf '  G136 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
