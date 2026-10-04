#!/usr/bin/env bash
# G66 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_trig_gate_selftest.sh [<dir written by subject_trig_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N1: a "large correction" that
# did not exceed its threshold -- a trigger firing on nothing.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_trig_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g66self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_trig_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/subjtriggered_ca_tbl_full_seed7.csv" ] || { echo "  G66 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G66 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G66 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G66 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G66 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G66 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
F="$T/f/on/subjtriggered_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$F" "r = rows[0]; r['threshM'] = '%.4f' % (float(r['moveM']) + 1.0)"
check "N1 a correction that did not exceed its threshold" 1 LARGE
fresh; edit "$F" "r = rows[0]; r['estX'] = '%.4f' % (float(r['estX']) + 5.0)"
check "N2 a row the subject's trace does not support" 1 LARGE
fresh; edit "$F" "r = rows[0]; r['tier'] = str((int(r['tier']) + 1) % 3)"
check "N3 a correction on a tier the subject was not on" 1 LARGE
fresh; edit "$F" "rows[:] = rows[1:]"
check "N4 a subject frame with no trigger behind it" 1 FRAMES
fresh; sed -i -E 's/(MC-2 triggered updates \(subjects, \w+\): large correction )[0-9]+/\199/' "$T/f/on.log"
check "N5 an MC-2 subjects line the file does not support" 1 COUNTS
fresh; cp "$F" "$T/f/off/"
check "N6 the control writing subject triggers" 1 CONTROL
fresh; echo 0 > "$T/f/r_notu.rc"
check "N7 the flag accepted without --triggeredUpdates" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS
fresh; edit "$F" "rows[:] = []"
check "N9 no subject large correction" 1 ROWS

printf '  G66 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
