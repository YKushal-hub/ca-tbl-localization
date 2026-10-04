#!/usr/bin/env bash
# G52 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/queue_gossip_gate_selftest.sh [<dir written by queue_gossip_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The plan's negative is N1: a queue-less gossip claimed as a
# queue gossip.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/queue_gossip_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g52self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/queue_gossip_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/q/arb_ca_tbl_full_seed1.csv" ] || { echo "  G52 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G52 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G52 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G52 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G52 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G52 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
A="$T/f/q/arb_ca_tbl_full_seed1.csv"
E="$T/f/q/events_ca_tbl_full_seed1.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$E" '
import re
for r in rows:
    r["payload"] = re.sub(r";queue=[0-9]+", "", r["payload"])'
check "N1 a queue-less gossip claimed as a queue gossip" 1 GOSSIP
fresh; edit "$A" 'r = next(r for r in rows if r["event"] == "yield"); r["requester"] = "4"'
check "N2 a yield no higher-id anchor justified" 1 YIELD
fresh; edit "$A" '
y = next(r for r in rows if r["event"] == "yield")
g = dict(y); g["event"] = "aged_grant"; g["t"] = "%.6f" % (float(y["t"]) + 30.0)
rows.append(g)'
check "N3 a yielded request granted from the yielder's queue" 1 AFTER
fresh; sed -i 's/yielded to a higher id \([0-9]*\)/yielded to a higher id 9\1/' "$T/f/q.log"
check "N4 an NW-6 line the log does not support" 1 COUNTS
fresh; edit "$T/f/nq/events_ca_tbl_full_seed1.csv" '
r = next(r for r in rows if r["msgType"] == "GRANT_GOSSIP"); r["payload"] += ";queue=1"'
check "N5 grants-only gossip carrying a queue" 1 CONTROL
fresh; echo 0 > "$T/f/r_noarb.rc"
check "N6 --gossipQueue accepted without --arbitrationV2" 1 R
fresh; echo 134 > "$T/f/q.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G52 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
