#!/usr/bin/env bash
# G151 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/iterate_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g151self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/iterate_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.log" ] || { echo "  G151 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G151 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G151 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G151 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the iterated run crashed" 1 RUNS
fresh; sed -i '/iterated update/d' "$T/f/on.log"; check "N2 the flag silently not taking effect" 1 REACHED
fresh; sed -i -E 's/(updates )[0-9]+/\10/' "$T/f/on.log"; check "N3 no update was iterated" 1 REACHED
fresh; sed -i -E 's/updates ([0-9]+), passes [0-9]+/updates \1, passes \1/' "$T/f/on.log"; check "N4 one pass per update: never re-linearised" 1 ITERATES
fresh; cp "$T/f/base"/trace_ca_tbl_full_seed*.csv "$T/f/on/"; check "N5 iterating changed nothing" 1 EFFECT
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N6 the default not being 0" 1 OFF
fresh; grep "iterated update" "$T/f/on.log" >> "$T/f/def.log"; check "N7 the flag leaking into the default" 1 CONTROL
fresh; echo 0 > "$T/f/r_cv.rc"; check "N8 accepted without ekf5" 1 REFUSE
fresh; echo 0 > "$T/f/r_rob.rc"; check "N9 accepted with a robust mode" 1 REFUSE
fresh; echo 0 > "$T/f/r_neg.rc"; check "N10 accepted a negative pass count" 1 REFUSE
echo "  G151 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
