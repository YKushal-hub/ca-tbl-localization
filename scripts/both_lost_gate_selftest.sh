#!/usr/bin/env bash
# G83 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/both_lost_gate_selftest.sh [<dir written by both_lost_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N1: the defect itself, a flag-on
# run in which no frame is ever lost to a completed one.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/both_lost_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g83self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/both_lost_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/events_ca_tbl_full_seed7.csv" ] || { echo "  G83 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G83 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G83 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G83 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G83 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G83 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
E="$T/f/on/events_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; sed -i '/,collision_late,/d' "$E"
check "N1 the flag on, and no frame lost to a completed one" 1 LATE
fresh; sed -i -E 's/(of them against a completed frame )[0-9]+/\1999/' "$T/f/on.log"
check "N2 an N.22 line the events do not support" 1 LATE
fresh; python3 - "$E" <<'PY'
import re, sys
p = sys.argv[1]
ls = open(p).read().splitlines(True)
k = next(i for i, l in enumerate(ls) if ',collision_late,' in l)
f = ls[k].split(',')
ls[k] = re.sub(r';by=\d+:', ';by=' + f[3] + ':', ls[k])
open(p, 'w').write(''.join(ls))
PY
check "N3 a frame lost to the receiver's own transmission" 1 BY
fresh; python3 - "$E" <<'PY'
import csv, re, sys
p = sys.argv[1]
ls = open(p).read().splitlines(True)
ends = {}
for l in ls[1:]:
    f = l.split(',')
    m = re.search(r'(?:^|;)pkt=(\d+)', ','.join(f[7:]))
    if m:
        ends.setdefault((f[1], f[2], m.group(1)), []).append(float(f[0]))
for i, l in enumerate(ls):
    if ',collision_late,' not in l:
        continue
    f = l.split(',')
    b = re.search(r';by=(\d+):(\d+)', l)
    if (f[1], b.group(1), b.group(2)) in ends:
        f[0] = '%g' % (min(ends[(f[1], b.group(1), b.group(2))]) - 10.0)
        ls[i] = ','.join(f)
        break
open(p, 'w').write(''.join(ls))
PY
check "N4 a frame lost to one that had not yet ended" 1 BY
fresh; sed -i -E '0,/,collision_late,/s/(,collision_late,.*);by=[0-9]+:[0-9]+/\1/' "$E"
check "N5 a late loss that names no frame" 1 BY
fresh; grep -m1 ',collision_late,' "$E" >> "$T/f/off/events_ca_tbl_full_seed7.csv"
check "N6 the control losing a frame to a completed one" 1 CONTROL
fresh; echo 134 > "$T/f/on.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G83 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
