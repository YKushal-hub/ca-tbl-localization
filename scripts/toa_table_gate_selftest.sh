#!/usr/bin/env bash
# G80 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/toa_table_gate_selftest.sh [<dir written by toa_table_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/toa_table_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g80self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/toa_table_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/toa_table.csv" ] || { echo "  G80 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G80 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G80 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G80 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G80 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G80 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
edit() {  # edit <csv> <python statement over rows>
    python3 - "$1" "$2" <<'PY'
import csv, sys
p, stmt = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
exec(stmt)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=cols, lineterminator="\n")
w.writeheader()
w.writerows(rows)
PY
}
F="$T/f/on/toa_table.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$F" "rows.pop()"
check "N1 a missing row" 1 ROWS
fresh; edit "$F" "r = rows[5]; r['toaS'] = '%.9f' % (float(r['toaS']) * 1.01)"
check "N2 a ToA off the Semtech formula" 1 FORMULA
fresh; edit "$F" "[r.update(lowDataRateOpt='1') for r in rows if r['sf'] == '11' and r['bwHz'] == '250000']"
check "N3 the low-data-rate flag on a short symbol" 1 DE
fresh; edit "$F" "r = rows[0]; r['frameBytes'] = str(int(r['frameBytes']) + 1)"
check "N4 a frame not 9 bytes plus its payload" 1 FRAME
fresh; sed -i '/  toa_table.csv$/d' "$T/f/on/run_id.txt"
check "N5 the table not digested" 1 DIGEST
fresh; cp "$F" "$T/f/off/"
check "N6 the control writing the table" 1 CONTROL
fresh; echo 134 > "$T/f/on.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G80 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
