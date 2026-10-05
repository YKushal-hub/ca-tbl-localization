#!/usr/bin/env bash
# G57 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/reqesc_gate_selftest.sh [<dir written by reqesc_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N3: a request sent to "the
# nearest gateway" -- an arbiter with no UWB reach of the target -- which the plan's review
# ruled out.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/reqesc_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g57self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/reqesc_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/req/reqesc_ca_tbl_full_seed7.csv" ] || { echo "  G57 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G57 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G57 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G57 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G57 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G57 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
Q="$T/f/req/reqesc_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$Q" 'rows[0]["pcs"] = "%.4f" % (float(rows[0]["threshold"]) + 0.1)'
check "N1 a request on a result whose PCS was not below the threshold" 1 RULE
fresh; edit "$Q" '
x = dict(rows[0]); x["t"] = "%.6f" % (float(rows[0]["t"]) + 5.0); rows.insert(1, x)'
check "N2 two requests for one target inside a slot hold" 1 RULE
fresh; edit "$Q" 'r = next(r for r in rows if r["outcome"] == "granted"); r["arbiter"] = "105"'
check "N3 a request sent to the nearest gateway, which has no reach of the target" 1 ROUTE
fresh; edit "$Q" 'r = next(r for r in rows if r["outcome"] == "granted"); r["outcome"] = "queued"'
check "N4 an outcome the arbiter did not decide" 1 ARB
fresh; edit "$T/f/req/events_ca_tbl_full_seed7.csv" '
for r in rows:
    r["payload"] = r["payload"].replace(";obo=1", "")'
check "N5 no requester-obtained grant ever delivered" 1 COMMIT
fresh; sed -i 's/TA-2 requester escalation (ca_tbl_full): granted \([0-9]*\)/TA-2 requester escalation (ca_tbl_full): granted 9\1/' "$T/f/req.log"
check "N6 a TA-2 line the logs do not support" 1 COUNTS
fresh; edit "$T/f/noreq/events_ca_tbl_full_seed7.csv" '
r = next(r for r in rows if r["msgType"] == "ESC_GRANT"); r["payload"] += ";obo=1"'
check "N7 an on-behalf grant without the flag" 1 CONTROL
fresh; echo 0 > "$T/f/r_noarb.rc"
check "N8 --requesterEscalation accepted without --arbitrationV2" 1 R
fresh; echo 134 > "$T/f/req.rc"
check "N9 a run the checks read did not exit 0" 1 RUNS

printf '  G57 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
