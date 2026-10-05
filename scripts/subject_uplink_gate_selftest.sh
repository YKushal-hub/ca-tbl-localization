#!/usr/bin/env bash
# G65 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_uplink_gate_selftest.sh [<dir written by subject_uplink_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N2 and N3: a bill that is not
# the airtime at the radio's TX power, and a subject billed on a tier it never used (sect 6).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_uplink_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g65self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_uplink_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/subject_summary.csv" ] || { echo "  G65 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G65 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G65 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G65 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G65 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G65 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
S="$T/f/on/subject_summary.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$S" "r = next(r for r in rows if r['subjectId'] != '1'); r['escalations'] = str(int(r['escalations']) + 1)"
check "N1 a subject's escalations moved with the bill" 1 ONLY
fresh; edit "$S" "r = next(r for r in rows if r['subjectId'] != '1' and float(r['tx_2p4']) > 0); r['tx_2p4'] = '%.9f' % (float(r['tx_2p4']) * 1.01)"
check "N2 a bill that is not the airtime at the TX power" 1 EXACT
fresh; python3 - "$T/f/off/subject_summary.csv" "$S" <<'PY'
import csv, sys
off = list(csv.DictReader(open(sys.argv[1])))
rows = list(csv.DictReader(open(sys.argv[2])))
cols = list(csv.reader(open(sys.argv[2])))[0]
for a, b in zip(rows, off):
    if a['subjectId'] != '1' and float(b['tx_uwb']) == 0.0:
        a['tx_uwb'] = '0.000001000'
        break
w = csv.DictWriter(open(sys.argv[2], 'w', newline=''), fieldnames=cols, lineterminator='\n'); w.writeheader(); w.writerows(rows)
PY
check "N3 a subject billed on UWB, never used" 1 SIX
fresh; f=$(ls "$T"/f/on/trace_*.csv | head -1); edit "$f" "rows[3]['errM'] = '999'"
check "N4 tag 0's trace moved" 1 ONLY
fresh; sed -i -E 's/(subject [0-9]+ 2\.4 GHz frames )[0-9]+/\10/g' "$T/f/on.log"
check "N5 no subject uplink frame" 1 REACH
fresh; grep 'SY-1 edge uplink (subjects' "$T/f/on.log" >> "$T/f/off.log"
check "N6 the control billing the subjects" 1 CONTROL
fresh; echo 0 > "$T/f/r_noup.rc"
check "N7 the flag accepted without --edgeUplinkRanges" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G65 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
