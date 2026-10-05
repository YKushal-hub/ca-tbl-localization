#!/usr/bin/env bash
# G79 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_witness_gate_selftest.sh [<dir written by subject_witness_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_witness_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g79self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_witness_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G79 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G79 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G79 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G79 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G79 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G79 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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

fresh
check "P0 the real run set passes" 0 -
fresh; rm "$T"/f/on/subjwitness_*_subj4.csv
check "N1 a subject without its witness file" 1 FILES
fresh; sed -i -E 's/(Row 55 witness \(subject [0-9]+\): IMU evaluations )[0-9]+/\10/; s/(witnesses )[0-9]+(, agreeing)/\10\2/' "$T/f/on.log"; for f in "$T"/f/on/subjwitness_*.csv; do head -1 "$f" > "$f.h"; mv "$f.h" "$f"; done
check "N2 no subject witness ever evaluated" 1 EVAL
fresh; sed -i -E '0,/(Row 55 witness \(subject [0-9]+\): IMU evaluations )[0-9]+/s//\199999/' "$T/f/on.log"
check "N3 a witness line its file does not support" 1 COUNTS
fresh; f=$(ls "$T"/f/on/subjwitness_*_subj2.csv); edit "$f" "r = next(r for r in rows if r['evaluated'] == '1'); r['anchored'] = '0'"
check "N4 an evaluation with no anchored track" 1 LOGIC
fresh; cp "$T"/f/on/subjwitness_*_subj2.csv "$T/f/off/"
check "N5 the control writing subject witnesses" 1 CONTROL
fresh; echo 0 > "$T/f/r_shs.rc"
check "N6 the flag accepted with the SHS witness source" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G79 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
