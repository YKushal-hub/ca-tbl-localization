#!/usr/bin/env bash
# G189 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
GATE="$(pwd)/scripts/tdoa_height_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g189self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash scripts/tdoa_height_gate.sh "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/both" ] || { echo "  G189 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; mkdir "$T/f"; cp "$REAL"/*.rc "$REAL"/*.log "$T/f/"; for n in off false rd both n0 n1; do mkdir "$T/f/$n"; cp "$REAL/$n"/subjtrace_* "$REAL/$n"/subjtdoa_* "$REAL/$n"/arm_summary.csv "$T/f/$n/"; done; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G189 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G189 ok    %-58s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G189 FAIL  %-58s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/both.rc"; check "N1 a run crashed" 1 RUNS
fresh; echo x >> "$T/f/false/arm_summary.csv"; check "N2 an explicit default changes an output" 1 OFFSAME
fresh; cp "$T"/f/off/subjtrace_* "$T/f/rd/"; check "N3 the delay changes nothing" 1 WORLD
fresh; sed -i '/DD62 tdoaHeight/d' "$T/f/both.log"; check "N4 the correction not announced" 1 LINES
fresh; echo "DD62 tdoaHeightCorrect" >> "$T/f/rd.log"; check "N5 the correction on in the control" 1 LINES
fresh; rm "$(ls "$T"/f/both/subjtrace_* | head -1)"; check "N6 a crew trace missing" 1 ROWS
fresh; f=$(ls "$T"/f/both/subjtrace_* | head -1); sed -i '$d' "$f"; check "N7 a truncated crew trace" 1 ROWS
fresh; cp "$T"/f/rd/subjtrace_* "$T/f/both/"; check "N8 no effect" 1 EFFECT
fresh; cp "$T"/f/n0/subjtdoa_* "$T/f/n1/"; check "N9 the fix keeps its constant offset" 1 OFFSET
fresh; rm "$T"/f/n1/subjtdoa_*; check "N11 no fixes (vacuous)" 1 OFFSET
fresh; rm "$T"/f/both/subjtrace_*; check "N10 no crew traces (vacuous)" 1 EFFECT
echo "  G189 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
