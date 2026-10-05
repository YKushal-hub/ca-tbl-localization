#!/usr/bin/env bash
# G85 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/fuse_short_gate_selftest.sh [<dir written by fuse_short_gate.sh>]
#   Without a directory it makes its own real run set first (about 3 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N1: a union formed where the
# floor could fix alone, which is the full fuse the flag narrows.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/fuse_short_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g85self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/fuse_short_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/end/fusefloor_ca_tbl_full_seed7.csv" ] || { echo "  G85 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G85 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G85 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G85 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G85 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G85 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
E="$T/f/end/fusefloor_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$E" "r = rows[0]; r['nFloor'] = '3'; r['nFused'] = str(int(r['nActive']) + 3)"
check "N1 a union formed where the floor could fix alone" 1 SHORT
fresh; edit "$E" "r = rows[0]; r['nFused'] = str(int(r['nFused']) + 1)"
check "N2 a set that is not the sum of its parts" 1 SHORT
fresh; edit "$E" "rows[:] = []"
check "N3 the narrowed fuse never fusing" 1 SHORT
fresh; edit "$T/f/full/fusefloor_ca_tbl_full_seed7.csv" "rows[:] = [r for r in rows if int(r['nFloor']) < 3]"
check "N4 a control in which the narrowing removes nothing" 1 FULL
fresh; sed -i -E 's/(N\.14 floor\+active fusion \(\S+\): sets )[0-9]+/\1999/' "$T/f/end.log"
check "N5 an N.14 line the file does not support" 1 COUNTS
fresh; echo 0 > "$T/f/r_nofuse.rc"
check "N6 the flag accepted without --fuseFloorActive" 1 R
fresh; echo 134 > "$T/f/main.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G85 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
