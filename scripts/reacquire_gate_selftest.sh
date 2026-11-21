#!/usr/bin/env bash
# G175 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/reacquire_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g175self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/reacquire_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G175 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G175 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G175 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G175 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the re-acquiring run crashed" 1 RUNS
fresh; sed -i '/DD34/d' "$T/f/on.log"; check "N2 the flag not announced" 1 LINE
fresh; rm -f "$T"/f/on/*.csv; cp "$T"/f/off/*.csv "$T/f/on/"; check "N3 the filters still confidently wrong with it on" 1 CONSIST
fresh; rm -f "$T"/f/off/*.csv; cp "$T"/f/on/*.csv "$T/f/off/"; check "N4 no failure to remove (off already consistent)" 1 CONSIST
fresh; grep DD34 "$T/f/on.log" >> "$T/f/off.log"; check "N5 the flag announced with it off" 1 LINE
echo "  G175 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
