#!/usr/bin/env bash
# G51 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/arbitration_v2_gate_selftest.sh [<dir written by arbitration_v2_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The plan's two negatives are N2 (the constant T_query) and
# N3 (a queue that never re-scores).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/arbitration_v2_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g51self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/arbitration_v2_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/v2/arb_ca_tbl_full_seed7.csv" ] || { echo "  G51 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G51 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G51 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G51 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G51 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G51 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
A="$T/f/v2/arb_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$A" 'r = next(r for r in rows if r["event"] == "grant"); r["u"] = "%.6f" % (float(r["u"]) + 0.5)'
check "N1 a score that is not l.T" 1 SCORE
fresh; edit "$A" 'r = next(r for r in rows if r["qt"] == "1" and r["event"] not in ("out_of_range", "expire")); r["qt"] = "0"'
check "N2 the constant T_query (a query-triggered request scored as if it were not)" 1 SCORE
fresh; edit "$A" '
w = next(r for r in rows if r["event"] == "aged_grant")
x = dict(w); x["event"] = "score"; x["requester"] = "99"; x["u"] = "%.6f" % (float(w["u"]) + 1.0)
rows.insert(rows.index(w), x)'
check "N3 a queue that never re-scores (the grant is not the argmax)" 1 ARGMAX
fresh; edit "$A" 'r = next(r for r in rows if r["event"] in ("grant", "queue")); r["evidenceAgeS"] = "-1.000000"'
check "N4 a grant or queueing without UWB-range evidence" 1 RANGE
fresh; edit "$A" 'r = next(r for r in rows if r["event"] == "out_of_range"); r["evidenceAgeS"] = "1.000000"'
check "N5 an out-of-range refusal that had evidence" 1 RANGE
fresh; edit "$A" 'rows[:] = [r for r in rows if r["event"] != "aged_grant"]'
check "N6 nothing ever granted from the queue" 1 QUEUE
fresh; edit "$T/f/v2/grants_ca_tbl_full_seed7.csv" '
for r in rows:
    if r["event"] == "queued":
        r["event"] = "deny"'
check "N7 no queued request ever committed at its subject" 1 QUEUE
fresh; edit "$A" '
for r in rows:
    if int(r["arbiter"]) >= 501:
        r["arbiter"] = "105"'
check "N8 finders never arbitrate" 1 FINDERS
fresh; edit "$T/f/v2/events_ca_tbl_full_seed7.csv" '
for r in rows:
    r["payload"] = r["payload"].replace(";qt=1", "")'
check "N9 no request carries is_query_triggered" 1 QUERY
fresh; sed -i 's/NW-5 arbitration (ca_tbl_full): grants \([0-9]*\)/NW-5 arbitration (ca_tbl_full): grants 9\1/' "$T/f/v2.log"
check "N10 an NW-5 line the log does not support" 1 COUNTS
fresh; cp "$A" "$T/f/c4/"
check "N11 the C4 control writes an arbitration log" 1 CONTROL
fresh; echo 0 > "$T/f/r_arb.rc"
check "N12 --arbitrationV2 accepted without --handshakeFixes" 1 R
fresh; echo 134 > "$T/f/v2.rc"
check "N13 a run the checks read did not exit 0" 1 RUNS

printf '  G51 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
