#!/usr/bin/env bash
# G172 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/height_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g172self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/height_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G172 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G172 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G172 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G172 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
N=trace_static_868_seed7.csv
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the corrected run crashed" 1 RUNS
fresh; sed -i '/DD28/d' "$T/f/on.log"; check "N2 the flag not announced" 1 LINE
fresh; grep DD28 "$T/f/on.log" >> "$T/f/off.log"; check "N3 the flag announced with it off" 1 LINE
fresh; python3 - "$T/f/on/$N" <<'PY'
import csv, sys
p = sys.argv[1]; r = list(csv.reader(open(p))); i = r[0].index("trueX"); r[5][i] = str(float(r[5][i]) + 1.0)
csv.writer(open(p, "w", newline="")).writerows(r)
PY
check "N4 the world moved, not just the estimate" 1 TRUTH
fresh; cp "$T/f/off/$N" "$T/f/on/$N"; check "N5 the flag changed no estimate" 1 MOVES
echo "  G172 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
