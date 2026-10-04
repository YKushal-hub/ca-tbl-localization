#!/usr/bin/env bash
# G69 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/finder_strict_gate_selftest.sh [<dir written by finder_strict_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N3: a used position that is the
# truth -- the oracle this flag exists to remove.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/finder_strict_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g69self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/finder_strict_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/pseudo_ca_tbl_full_seed7.csv" ] || { echo "  G69 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G69 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G69 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G69 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G69 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G69 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
F="$T/f/on/pseudo_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$F" "rows[:] = [r for r in rows if r['kind'] != 'dropped']"
check "N1 no range ever reaching the drop path" 1 ROWS
fresh; sed -i -E 's/(TABLE\(reverse\): )[0-9]+( pseudo-anchor substitutions)/\199999\2/' "$T/f/on.log"
check "N2 substitutions the file does not log" 1 SUBS
fresh; edit "$F" "r = next(r for r in rows if r['kind'] == 'report'); r.update(usedX=r['trueX'], usedY=r['trueY'])"
check "N3 a used position that is the truth" 1 NOTTRUE
fresh; sed -i -E 's/(dropped for want of a position )[0-9]+(, reports used)/\199999\2/' "$T/f/on.log"
check "N4 an N.21 strict line the file does not support" 1 COUNTS
fresh; cp "$F" "$T/f/off/"
check "N5 the control writing strict rows" 1 CONTROL
fresh; echo 0 > "$T/f/r_nopa.rc"
check "N6 the flag accepted without --finderPseudoAnchor" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G69 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
