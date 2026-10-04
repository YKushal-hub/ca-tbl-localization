#!/usr/bin/env bash
# G73 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/viz_digest_gate_selftest.sh [<dir written by viz_digest_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/viz_digest_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g73self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/viz_digest_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/run_id.txt" ] || { echo "  G73 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G73 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G73 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G73 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G73 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G73 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set passes" 0 -
fresh; sed -i '/  events_/d' "$T/f/on/run_id.txt"
check "N1 a visualiser file without a digest line" 1 DIGEST
fresh; f=$(ls "$T"/f/on/nodes_*.csv | head -1); echo "0,1,0,0" >> "$f"; cp "$f" "$T/f/off/"
check "N2 a digest line the file no longer matches" 1 DIGEST
fresh; f=$(ls "$T"/f/on/trace_*.csv | head -1); sed -i '2s/,/,9/' "$f"
check "N3 the flag changing an output, not just the sidecar" 1 SAME
fresh; grep '  deployment.json' "$T/f/on/run_id.txt" >> "$T/f/off/run_id.txt"
check "N4 the control digesting a visualiser file" 1 CONTROL
fresh; echo 0 > "$T/f/r_noviz.rc"
check "N5 the flag accepted without --vizEvents" 1 R
fresh; echo 134 > "$T/f/off.rc"
check "N6 a run the checks read did not exit 0" 1 RUNS

printf '  G73 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
