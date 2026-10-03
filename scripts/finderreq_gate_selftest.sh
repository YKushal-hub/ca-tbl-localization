#!/usr/bin/env bash
# G64 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/finderreq_gate_selftest.sh [<dir written by finderreq_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N3 and N4: a finder scoring a
# request in its own queue without evidence it can reach the target, or "naming" itself --
# the nearest-anchor shortcut the plan's review ruled out, in another form.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/finderreq_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g64self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/finderreq_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/finderreq_ca_tbl_full_seed7.csv" ] || { echo "  G64 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G64 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G64 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G64 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G64 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G64 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
F="$T/f/on/finderreq_ca_tbl_full_seed7.csv"
A="$T/f/on/arb_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$F" "r = rows[0]; r['pcs'] = '%.4f' % (float(r['threshold']) + 0.5)"
check "N1 a request on a result above the threshold" 1 RULE
fresh; edit "$F" "r = dict(rows[0]); r['t'] = '%.6f' % (float(r['t']) + 5.0); rows.insert(1, r)"
check "N2 two requests for one target 5 s apart" 1 RULE
fresh; edit "$F" "r = next(r for r in rows if r['route'] == 'own'); r['arbiter'] = '101'"
check "N3 an own request scored by another node" 1 ROUTE
fresh; edit "$F" "r = next(r for r in rows if r['route'] == 'named'); r['arbiter'] = r['finderId']"
check "N4 a named request naming the finder itself" 1 ROUTE
fresh; edit "$F" "r = next(r for r in rows if r['route'] == 'unroutable'); r['arbiter'] = '101'"
check "N5 an unroutable request with an arbiter" 1 ROUTE
fresh; python3 - "$F" "$A" <<'PY'
import csv, sys
r = next(r for r in csv.DictReader(open(sys.argv[1])) if r['route'] != 'unroutable')
p = sys.argv[2]
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
rows = [a for a in rows if not (a['t'] == r['t'] and a['arbiter'] == r['arbiter'] and a['requester'] == r['target'] and a['event'] == 'obo_submit')]
w = csv.DictWriter(open(p, 'w', newline=''), fieldnames=cols, lineterminator='\n'); w.writeheader(); w.writerows(rows)
PY
check "N6 a sent request its arbiter never scored" 1 ARB
fresh; edit "$F" "r = next(r for r in rows if r['outcome'] == 'granted'); r['outcome'] = 'queued'"
check "N7 an outcome the arbiter did not log" 1 ARB
fresh; sed -i -E 's/(TA-1b finder requester escalation \(\S+\): requests own-arbiter )([0-9]+)/\1999/' "$T/f/on.log"
check "N8 a TA-1b line the file does not support" 1 COUNTS
fresh; cp "$F" "$T/f/off/"
check "N9 the control writing finder requests" 1 CONTROL
fresh; echo 0 > "$T/f/r_nofs.rc"
check "N10 the flag accepted without --finderSessions" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N11 a run the checks read did not exit 0" 1 RUNS
fresh; edit "$F" "rows[:] = [r for r in rows if r['route'] == 'unroutable']"
check "N12 no request ever sent" 1 ROWS

printf '  G64 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
