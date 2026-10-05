#!/usr/bin/env bash
# G53 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/hop_provenance_gate_selftest.sh [<dir written by hop_provenance_gate.sh>]
#   Without a directory it makes its own real run set first (about 20 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The plan's negatives: N1 (the literal-0 hop, the behaviour
# before the flag) and N3 (depth capped at 1).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/hop_provenance_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g53self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/hop_provenance_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/hops_static_2p4_seed7.csv" ] || { echo "  G53 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G53 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G53 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G53 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G53 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G53 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
H="$T/f/on/hops_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$H" '
for r in rows:
    r["hopCountAtFix"] = "0"'
check "N1 the literal-0 hop (the behaviour before the flag)" 1 RECOUNT
fresh; edit "$H" 'r = next(r for r in rows if r["maxHop"] == "1"); r["maxHop"] = "2"'
check "N2 a depth the chain log does not show" 1 RECOUNT
fresh; for a in static_2p4 ca_tbl_full; do edit "$T/f/on/hops_${a}_seed7.csv" '
for r in rows:
    if r["hopCountAtFix"] == "2":
        r["hopCountAtFix"] = "1"; r["maxHop"] = "1"'; done
check "N3 depth capped at 1" 1 DEPTH
fresh; sed -i 's/(ca_tbl_full): fixes by depth of the fused set 0:\([0-9]*\)/(ca_tbl_full): fixes by depth of the fused set 0:9\1/' "$T/f/on.log"
check "N4 an NW-1 line the hops log does not support" 1 LINE
fresh; cp "$H" "$T/f/off/"
check "N5 the default writes a hops log" 1 CONTROL
fresh; echo 0 > "$T/f/r_norelay.rc"
check "N6 --hopProvenance accepted without relays" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G53 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
