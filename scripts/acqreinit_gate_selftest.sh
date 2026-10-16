#!/usr/bin/env bash
# G133 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/acqreinit_gate_selftest.sh [<dir written by acqreinit_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/acqreinit_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g133self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/acqreinit_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G133 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <label that must FAIL, or ->
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G133 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G133 ok    %-64s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G133 FAIL  %-64s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G133 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/on" "$T/f/on.log"; cp -r "$REAL/base" "$T/f/on"; cp "$REAL/base.log" "$T/f/on.log"; check "N1 a run without the flag offered as the flag-on run" 1 FIRES
fresh; f=$(cd "$T/f/off" && find . -type f ! -name 'run_id.txt' | sort | head -1); echo x >> "$T/f/off/$f"
check "N2 the default given explicitly changes an output ($f)" 1 DEFAULT
fresh; echo 134 > "$T/f/on.rc"; check "N3 a run did not exit 0" 1 RUNS
fresh; echo "ACQREINIT x=0 y=0 pv=1 n=3 res=0" >> "$T/f/base.log"; check "N4 the control prints a firing" 1 FIRES
fresh; cp "$REAL/base/trace_ca_tbl_full_seed7.csv" "$T/f/on/trace_ca_tbl_full_seed7.csv"; check "N5 the flag-on trace equals the control's" 1 CHANGES
printf '  G133 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
