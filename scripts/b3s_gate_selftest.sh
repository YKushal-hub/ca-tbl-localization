#!/usr/bin/env bash
# G135 negative fixtures (rule 6). usage: bash scripts/b3s_gate_selftest.sh [<dir written by b3s_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/b3s_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g135self.XXXXXX") || exit 2; trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/b3s_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G135 selftest: no real run set"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() { python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G135 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then printf '  G135 ok    %-56s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else printf '  G135 FAIL  %-56s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G135 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1)); fi; }
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/s1"; cp -r "$REAL/base" "$T/f/s1"; check "N1 the control offered as the S1 run" 1 S1
fresh; rm -rf "$T/f/wolf"; cp -r "$REAL/base" "$T/f/wolf"; check "N2 the control offered as the wolf preset" 1 S2
fresh; rm -rf "$T/f/gottschalk"; cp -r "$REAL/muller" "$T/f/gottschalk"; check "N3 the muller run offered as gottschalk" 1 S2
fresh; echo x >> "$T/f/off/probe_range_sweep.csv"; check "N4 the explicit defaults change an output" 1 DEFAULT
fresh; echo 1 > "$T/f/s1.rc"; check "N5 a run did not exit 0" 1 RUNS
printf '  G135 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
