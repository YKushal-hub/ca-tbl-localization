#!/usr/bin/env bash
# G43 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/entry_prediction_gate_selftest.sh [<dir written by entry_prediction_gate.sh>]
#   Without a directory it makes its own real run set first (about 20 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N2  every entry used where it was reported -- the tree before DC-13;
#   N5  a substituted entry the node never held.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/entry_prediction_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g43self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/entry_prediction_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G43 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G43 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G43 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G43 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G43 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G43 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
EP="$T/f/on/entrypred_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$T/f/off/trace_ca_tbl_full_seed7.csv" 'rows[3]["estX"] = "1.0"'
check "N0 --entryPrediction=false moves an output" 1 NULL
fresh; grep "DC-13" "$T/f/on.log" >> "$T/f/base.log"
check "N1 the run without the flag prints the DC-13 line" 1 NULL
fresh; edit "$EP" '
for r in rows:
    r["xPred"] = r["x0"]; r["yPred"] = r["y0"]'
check "N2 every entry used where it was reported (the tree before DC-13)" 1 MOVED
fresh; edit "$EP" '
for r in rows:
    r["varAdd"] = r["varAddLegacy"]'
check "N3 the walk-speed bound kept as the added variance" 1 PROPAGATE
fresh; edit "$EP" '
rows[0]["xPred"] = "%.6f" % (float(rows[0]["xPred"]) + 3.0)'
check "N4 a prediction off x0 + v age" 1 PROPAGATE
fresh; edit "$EP" 'rows[0]["seqNo"] = "99999"'
check "N5 a substituted entry the node never held" 1 ENTRIES
fresh; sed -i 's/DC-13 entry prediction: substitutions [0-9]*,/DC-13 entry prediction: substitutions 7,/' "$T/f/on.log"
check "N6 the DC-13 line disagrees with the CSV" 1 LINE
fresh; echo 0 > "$T/f/r_novel.rc"
check "N7 the flag accepted without --entryVelocity" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G43 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
