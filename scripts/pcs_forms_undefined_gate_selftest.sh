#!/usr/bin/env bash
# G72 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/pcs_forms_undefined_gate_selftest.sh
#
# Each fixture is a copy of the analysis script, outside the tree, with ONE thing changed,
# and the gate run against it must fail the check it names.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/pcs_forms_undefined_gate.py"
SRC="$ROOT/scratch/python_handoff/analyse_pcs_forms.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g72self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT

pass=0
fail=0
check() {  # check <name> <script> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$2" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$4" != - ]; then
        grep -Eq "G72 FAIL +$4 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$3" ] && [ "$reason" -eq 1 ]; then
        printf '  G72 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$3" -ne 0 ] && grep -m1 -E "G72 FAIL +$4 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G72 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$3" "$4"
        grep -E "G72 FAIL" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}

check "P0 the repo's script passes" "$SRC" 0 -
sed 's/if STRICT_UNDEFINED and nundef > 0:/if False:/' "$SRC" > "$T/n1.py"
check "N1 --strict-undefined still reading undefined as indistinguishable" "$T/n1.py" 1 STRICT
sed 's/if STRICT_UNDEFINED and nundef > 0:/if nundef > 0:/' "$SRC" > "$T/n2.py"
check "N2 the default output changed (the registrations would not reproduce)" "$T/n2.py" 1 DEFAULT
sed 's/^        if STRICT_UNDEFINED:$/        if False:/' "$SRC" > "$T/n3.py"
check "N3 no fix-cycle count per cell" "$T/n3.py" 1 STRICT

printf '  G72 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
