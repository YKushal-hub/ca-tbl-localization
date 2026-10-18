#!/usr/bin/env bash
# G150 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/finder_consider_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g150self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/finder_consider_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.log" ] || { echo "  G150 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G150 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G150 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G150 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the consider run crashed" 1 RUNS
fresh; sed -i '/consider parameters/d' "$T/f/on.log"; check "N2 the flag silently not taking effect" 1 LINE
fresh; sed -i -E 's/(ranges to finders )[0-9]+/\10/' "$T/f/on.log"; check "N3 consider never applied" 1 REACHED
fresh; cp "$T/f/base"/ranges_ca_tbl_full_seed*.csv "$T/f/on/"; check "N4 the R still carrying the GNSS part" 1 RSPLIT
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N5 the default not being false" 1 OFF
fresh; grep "consider parameters" "$T/f/on.log" >> "$T/f/def.log"; check "N6 the flag leaking into the default" 1 CONTROL
fresh; echo 0 > "$T/f/r_cv.rc"; check "N7 accepted without ekf5" 1 REFUSE
fresh; echo 0 > "$T/f/r_rob.rc"; check "N8 accepted with a robust mode" 1 REFUSE
echo "  G150 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
