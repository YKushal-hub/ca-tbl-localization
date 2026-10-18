#!/usr/bin/env bash
# G132 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/acq_gate_selftest.sh [<dir written by acq_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/acq_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g132self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/acq_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G132 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <label that must FAIL, or ->
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G132 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G132 ok    %-64s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G132 FAIL  %-64s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G132 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/acq3"; cp -r "$REAL/base" "$T/f/acq3"; check "N1 a run without the rule offered as the run at 3" 1 ACQ
fresh; f=$(cd "$T/f/zero" && find . -type f ! -name 'run_id.txt' | sort | head -1); echo x >> "$T/f/zero/$f"
check "N2 the default given explicitly changes an output ($f)" 1 DEFAULT
fresh; echo 134 > "$T/f/acq3.rc"; check "N3 a run did not exit 0" 1 RUNS
printf '  G132 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
