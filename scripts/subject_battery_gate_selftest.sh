#!/usr/bin/env bash
# G67 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_battery_gate_selftest.sh [<dir written by subject_battery_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N3 and N10: a dead subject
# that still fixes, or still transmits -- the "dead node still ranging" defect the tag-0
# fault once had (FAULT sect 49's comment).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_battery_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g67self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_battery_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/subjtriggered_ca_tbl_full_seed7.csv" ] || { echo "  G67 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G67 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G67 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G67 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G67 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G67 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
G="$T/f/on/subjtriggered_ca_tbl_full_seed7.csv"
SID=$(grep -m1 -oE 'FAULT sect 49 \(subject [0-9]+' "$REAL/on.log" | grep -oE '[0-9]+$')
TD=$(grep -m1 -oE "FAULT sect 49 \(subject $SID\): battery exhausted at t=[0-9.]+" "$REAL/on.log" | grep -oE '[0-9.]+$')

fresh
check "P0 the real run set passes" 0 -
fresh; sed -i -E "s/(FAULT sect 49 \(subject $SID\): battery exhausted at t=)[0-9.]+/\1$(python3 -c "print($TD + 10)")/" "$T/f/on.log"
check "N1 a death time the trace does not support" 1 DEAD
fresh; sed -i -E "/FAULT sect 49 \(subject $SID\)/d" "$T/f/on.log"
check "N2 a subject past its budget with no FAULT line" 1 DEAD
fresh; edit "$T/f/on/subjtrace_ca_tbl_full_seed7_subj$SID.csv" "r = next(r for r in rows if float(r['t']) > $TD + 20); r['hadFix'] = '1'"
check "N3 a dead subject still fixing" 1 SILENT
fresh; edit "$G" "r = next(r for r in rows if r['kind'] == 'last_gasp'); rows.append(dict(r))"
check "N4 two last gasps" 1 GASP
fresh; edit "$G" "r = next(r for r in rows if r['kind'] == 'last_gasp'); r['t'] = '%.4f' % (float(r['t']) - 50.0)"
check "N5 a last gasp before the death" 1 GASP
fresh; sed -i -E 's/(MC-2 triggered updates \(subjects, \w+\): [^\n]*last gasp )[0-9]+/\199/' "$T/f/on.log"
check "N6 an MC-2 subjects line the rows do not support" 1 COUNTS
fresh; grep -m1 'FAULT sect 49 (subject' "$REAL/on.log" >> "$T/f/off.log"
check "N7 the control killing a subject" 1 CONTROL
fresh; echo 0 > "$T/f/r_nobat.rc"
check "N8 the flag accepted without --batteryDepleteJ" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N9 a run the checks read did not exit 0" 1 RUNS
fresh; python3 - "$T/f/on/events_ca_tbl_full_seed7.csv" "$SID" "$TD" <<'PY'
import csv, re, sys
p, sid, td = sys.argv[1], int(sys.argv[2]), float(sys.argv[3])
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
def pkt(r):
    m = re.search(r"(?:^|;)pkt=(\d+)", r["payload"])
    return int(m.group(1)) if m else None
src = next(r for r in rows if pkt(r) is not None and pkt(r) >> 20 == sid)
new = dict(src)
new["t"] = "%.6f" % (td + 60.0)
new["payload"] = re.sub(r"pkt=\d+", "pkt=%d" % ((sid << 20) + 999999), new["payload"])
rows.append(new)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=cols, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N10 a dead subject still transmitting" 1 SILENT

printf '  G67 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
