#!/usr/bin/env bash
# G176 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/incident_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g176self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/incident_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on601" ] || { echo "  G176 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G176 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G176 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G176 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on602.rc"; check "N1 a randomised run crashed" 1 RUNS
fresh; sed -i '/DD35/d' "$T/f/on601.log"; check "N2 the incident not announced" 1 LINE
fresh; cp "$T/f/on601/nodes_static_868_seed601.csv" "$T/f/on602/nodes_static_868_seed602.csv"; check "N3 the same incident on two seeds" 1 VARY
fresh; awk -F, -v OFS=, 'NR==1 || $2!=1 {print; next} {print $1,$2,5000,$4}' "$REAL/on601/nodes_static_868_seed601.csv" > "$T/f/on601/nodes_static_868_seed601.csv"; check "N4 the casualty pinned to the box edge" 1 INSIDE
fresh; grep DD35 "$T/f/on601.log" >> "$T/f/off601.log"; check "N5 the incident announced with it off" 1 LINE
echo "  G176 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
