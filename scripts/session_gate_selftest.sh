#!/usr/bin/env bash
# G54 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/session_gate_selftest.sh [<dir written by session_gate.sh>]
#   Without a directory it makes its own real run set first (about 30 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The plan's negative is N1: the legacy policy feed offered
# as over-mesh -- a result no gateway ever received.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/session_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g54self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/session_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/mesh/session_ca_tbl_full_seed7.csv" ] || { echo "  G54 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G54 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G54 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G54 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G54 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G54 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
S="$T/f/mesh/session_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$S" '
x = dict(rows[-1]); x["t"] = "%.6f" % (float(x["t"]) + 2.5); x["queryId"] = "0"; x["answererId"] = "1"
x["stateBefore"] = rows[-1]["stateAfter"]; x["stateAfter"] = rows[-1]["stateAfter"]
rows.append(x)'
check "N1 the legacy policy feed offered as over-mesh (a result no gateway got)" 1 FEED
fresh; edit "$S" 'del rows[next(i for i, r in enumerate(rows) if r["event"] == "result" and i > 3)]'
check "N2 a delivered answer never fed to the session" 1 FEED
fresh; edit "$S" 'r = next(r for r in rows if r["event"] == "result"); r["pcs"] = "%.4f" % (float(r["pcs"]) + 0.2)'
check "N3 a RESULT that is not the delivered entry" 1 CONTENT
fresh; edit "$S" 'r = next(r for r in rows if r["stateBefore"] == "SEARCHING" and r["stateAfter"] != "SEARCHING"); r["event"] = "tick"'
check "N4 the session leaves SEARCHING on a tick, not an answer" 1 STATE
fresh; edit "$S" 'rows[2]["stateBefore"] = "TRACKING"'
check "N5 a broken state chain" 1 STATE
fresh; sed -i 's/first coarse\/precise (s): m1 \([0-9.]*\)/first coarse\/precise (s): m1 9\1/' "$T/f/mesh.log"
check "N6 a time-to-first-fix that is not the first delivered result" 1 TTFF
fresh; sed -i 's/answers delivered \([0-9]*\)/answers delivered 9\1/' "$T/f/mesh.log"
check "N7 a TA-1 line the log does not support" 1 LINE
fresh; cp "$S" "$T/f/legacy/"
check "N8 the legacy feed writes a session log" 1 CONTROL
fresh; echo 0 > "$T/f/r_nosess.rc"
check "N9 --sessionOverMesh accepted without --sessionLayer" 1 R
fresh; echo 134 > "$T/f/mesh.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS

printf '  G54 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
