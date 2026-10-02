#!/usr/bin/env bash
# G48 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/reverse_path_gate_selftest.sh [<dir written by reverse_path_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N2: a hop sent to the query's
# ORIGINATOR rather than the previous hop -- the pre-NW-8 cache.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/reverse_path_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g48self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/reverse_path_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/hop/events_ca_tbl_full_seed7.csv" ] || { echo "  G48 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G48 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G48 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G48 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G48 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G48 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# evedit <events csv> <python body over rows (list of dicts) with pl(r) -> payload dict, ps(r, d)>
evedit() {
    python3 - "$@" <<'PY'
import csv, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
def pl(r):
    return dict(kv.split("=", 1) for kv in r["payload"].split(";") if "=" in kv)
def ps(r, d):
    r["payload"] = ";".join(f"{k}={v}" for k, v in d.items())
def hop(r):
    p = pl(r)
    return r["msgType"] == "RESPONSE" and r["outcome"] in ("delivered", "capture") and p.get("flood") == "0"
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}
EH="$T/f/hop/events_ca_tbl_full_seed7.csv"
EL="$T/f/legacy/events_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; evedit "$EH" '
r = next(r for r in rows if hop(r))
r["rxId"] = "9999"'
check "N1 a unicast hop to a node that is not the previous hop" 1 PATH
fresh; evedit "$EH" '
r = next(r for r in rows if hop(r) and r["rxId"] != pl(r)["req"])
r["rxId"] = pl(r)["req"]'
check "N2 a hop sent to the originator, not the previous hop (the old cache)" 1 PATH
fresh; evedit "$EH" '
i = next(i for i, r in enumerate(rows) if hop(r) and r["rxId"] != pl(r)["req"])
del rows[i]'
check "N3 a forwarded hop missing from the event log" 1 COUNTS
fresh; sed -i 's/delivered to the requester \([0-9]*\)/delivered to the requester 1\1/' "$T/f/hop.log"
check "N4 an NW-8 line whose deliveries the log does not show" 1 COUNTS
fresh; evedit "$EH" '
rows[:] = [r for r in rows if not (hop(r) and r["rxId"] != pl(r)["req"])]'
sed -i 's/, forwarded [0-9]*,/, forwarded 0,/' "$T/f/hop.log"
check "N5 every path one hop (nothing forwarded)" 1 MULTI
fresh; evedit "$EL" '
r = next(r for r in rows if r["msgType"] == "RESPONSE")
d = pl(r); d["rq"] = "1"; d["req"] = "101"; d["flood"] = "0"; ps(r, d)'
check "N6 --reversePath alone carrying route fields" 1 LEGACY
fresh; echo 0 > "$T/f/r_norp.rc"
check "N7 --reversePathHop accepted without --reversePath" 1 R
fresh; echo 134 > "$T/f/hop.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G48 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
