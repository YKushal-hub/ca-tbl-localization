#!/usr/bin/env bash
# G56 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/finder_session_gate_selftest.sh [<dir written by finder_session_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N1: a finder's session fed an
# answer that reached a different requester.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/finder_session_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g56self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/finder_session_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/fs/session_ca_tbl_full_seed7.csv" ] || { echo "  G56 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G56 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G56 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G56 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G56 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G56 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv> <python body over rows (list of dicts)>
edit() {
    python3 - "$@" <<'PY'
import csv, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}
S="$T/f/fs/session_ca_tbl_full_seed7.csv"
SC="$T/f/fsc/session_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$S" 'r = next(r for r in rows if r["requesterId"] == "501" and r["event"] == "result"); r["requesterId"] = "502"'
check "N1 a finder fed an answer that reached another requester" 1 FEED
fresh; edit "$S" 'r = next(r for r in rows if r["requesterId"] == "503" and r["event"] == "result"); r["manifest"] = "1"'
check "N2 a finder searching for a subject it was not assigned" 1 TARGET
fresh; edit "$S" '
for r in rows:
    if int(r["requesterId"]) >= 501 and r["stateBefore"] == "SEARCHING":
        r["stateAfter"] = "SEARCHING"'
check "N3 no finder session ever left SEARCHING on an answer" 1 COARSE
fresh; edit "$SC" 'r = next(r for r in rows if r["event"] == "cancel"); r["distM"] = "45.0000"'
check "N4 a cancel beyond the cancel distance" 1 CANCEL
fresh; edit "$SC" '
c = next(r for r in rows if r["event"] == "cancel")
x = dict(c); x["event"] = "result"; x["t"] = "%.6f" % (float(c["t"]) + 30.0)
x["stateBefore"] = "CLOSED"; x["stateAfter"] = "LOCATED_COARSE"
rows.append(x)'
check "N5 a cancelled session moved afterwards" 1 CANCEL
fresh; sed -i '0,/finder 501 target m1, answers \([0-9]*\)/s//finder 501 target m1, answers 9\1/' "$T/f/fs.log"
check "N6 a TA-1b line the log does not support" 1 LINE
fresh; python3 - "$T/f" <<'PY'
import sys
d = sys.argv[1]
src = open(d + "/fs/session_ca_tbl_full_seed7.csv").read().splitlines()
row = next(l for l in src[1:] if l.split(",")[1] == "501")
open(d + "/nofs/session_ca_tbl_full_seed7.csv", "a").write(row + "\n")
PY
check "N7 a finder row without --finderSessions" 1 CONTROL
fresh; echo 0 > "$T/f/r_nobase.rc"
check "N8 --finderSessions accepted without --sessionOverMesh" 1 R
fresh; echo 134 > "$T/f/fs.rc"
check "N9 a run the checks read did not exit 0" 1 RUNS

printf '  G56 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
