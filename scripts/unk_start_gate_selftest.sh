#!/usr/bin/env bash
# G130 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/unk_start_gate_selftest.sh [<dir written by unk_start_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/unk_start_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g130self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/unk_start_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G130 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <label that must FAIL, or ->
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G130 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G130 ok    %-64s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G130 FAIL  %-64s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G130 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/unk"; cp -r "$REAL/base" "$T/f/unk"; check "N1 a known-start run offered as the unknown-start run" 1 UNK
fresh; f=$(cd "$T/f/dflt" && find . -type f ! -name 'run_id.txt' | sort | head -1); echo x >> "$T/f/dflt/$f"
check "N2 the defaults given explicitly change an output ($f)" 1 DEFAULT
fresh; for p in "$REAL"/base/subjtrace_*; do cp "$p" "$T/f/cons/"; done; check "N3 subject traces without the consistency columns" 1 CONS
fresh; rm -rf "$T/f/mvs"; cp -r "$REAL/cons" "$T/f/mvs"; check "N4 a run at measVarScale 1 offered as the run at 4" 1 MVS
fresh; echo 134 > "$T/f/unk.rc"; check "N5 a run did not exit 0" 1 RUNS
printf '  G130 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
