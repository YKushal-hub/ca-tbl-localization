#!/usr/bin/env bash
# G60 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/case_c_gate_selftest.sh [<dir written by case_c_gate.sh>]
#   Without a directory it makes its own real run set first (about 3 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N1, a peer range taken on a
# cycle the node had an anchor (Case C is the 0-anchor case and nothing else), and N3, a set
# credited to the base station that no answer could have carried there.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/case_c_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g60self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/case_c_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/case_c_ranges_ca_tbl_full_seed7.csv" ] || { echo "  G60 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G60 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G60 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G60 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G60 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G60 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
edit() {
    python3 - "$@" <<'PY'
import csv, glob, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}
RG="$T/f/on/case_c_ranges_ca_tbl_full_seed7.csv"
SH="$T/f/on/case_c_relative_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

# N1: the node of a peer range had an anchor that cycle.
fresh
python3 - "$RG" "$T/f/on" <<'PY'
import csv, glob, sys
rg, on = sys.argv[1:3]
r = next(r for r in csv.DictReader(open(rg)) if r["nodeId"] != "1")
p = glob.glob(f"{on}/subjtrace_*_subj{r['nodeId']}.csv")[0]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
for x in rows:
    if x["t"] == r["t"]:
        x["nAnchors"] = "2"
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N1 a peer range on a cycle its node had an anchor" 1 ZERO
fresh; edit "$RG" 'r = rows[0]; r["tier"] = "2" if r["tier"] != "2" else "1"'
check "N2 a peer range on a tier its node did not range on" 1 ZERO
fresh; edit "$RG" '
key = next((r["t"], r["nodeId"]) for r in rows if r["deliveredAt"] == "-1.0000")
for r in rows:
    if (r["t"], r["nodeId"]) == key:
        r["deliveredAt"] = "%.4f" % (float(r["t"]) + 2.5)'
check "N3 a set credited to the base station that no answer carried" 1 DELIVER
fresh; edit "$SH" 'r = next(r for r in rows if int(r["k"]) >= 3); r["stressM"] = "%.6f" % (float(r["stressM"]) + 5.0)'
check "N4 a shape that does not reproduce its distances as stated" 1 SHAPE
fresh; edit "$SH" 'r = next(r for r in rows if int(r["k"]) >= 3); r["completedPairs"] = str(int(r["completedPairs"]) + 1)'
check "N5 a set whose pair counts the ranges do not give" 1 SHAPE
fresh; sed -i -E 's/fused into a filter 0/fused into a filter 3/' "$T/f/on.log"
check "N6 Case C output fused into a filter" 1 NOFUSE
fresh; edit "$SH" 'rows[:] = [r for r in rows if r["k"] != "2"]'
check "N7 no pair ever assembled (and the line not matching)" 1 REACH
fresh; sed -i -E 's/reached the base station ([0-9]+)/reached the base station 9\1/' "$T/f/on.log"
check "N8 a DC-3 line the files do not support" 1 COUNTS
fresh; cp "$RG" "$T/f/off/"
check "N9 the control writing Case C ranges" 1 CONTROL
fresh; echo 0 > "$T/f/r_nosess.rc"
check "N10 --caseCRelative accepted without --sessionOverMesh" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N11 a run the checks read did not exit 0" 1 RUNS

printf '  G60 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
