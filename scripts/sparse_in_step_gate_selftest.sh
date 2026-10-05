#!/usr/bin/env bash
# G39 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/sparse_in_step_gate_selftest.sh [<dir written by sparse_in_step_gate.sh>]
#   Without a directory it makes its own real run set first (about 40 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N3  an accepted sparse cycle that also range-updates -- the double count;
#   N4  an accepted sparse cycle still recorded as a no-fix -- the tree before DC-11;
#   N6  every solve against the previous posterior -- the old prior.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/sparse_in_step_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g39self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/sparse_in_step_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/new.log" ] || { echo "  G39 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G39 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G39 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G39 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G39 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G39 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
SP="$T/f/new/sparse_ca_tbl_full_seed7.csv"
TR="$T/f/new/trace_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$T/f/off/trace_ca_tbl_full_seed7.csv" 'rows[3]["estX"] = "1.0"'
check "N0 --sparseInStep=false moves an output" 1 NULL
fresh; grep "DC-11" "$T/f/new.log" >> "$T/f/def.log"
check "N1 the default prints the DC-11 line" 1 NULL
fresh; edit "$T/f/old/trace_ca_tbl_full_seed7.csv" '
for r in rows:
    if r["nAnchors"] == "2":
        r["hadFix"] = "1"'
check "N2 the old path's solved cycles recorded as fixes (no defect to show)" 1 OLD
fresh; edit "$SP" '
next(r for r in rows if r["verdict"] == "0")["rangeUpdates"] = "1"'
check "N3 an accepted sparse cycle that also range-updates" 1 STEP
fresh; python3 - "$SP" "$TR" <<'PY'
import csv, sys
sp, tr = sys.argv[1:3]
t0 = next(r["t"] for r in csv.DictReader(open(sp)) if r["verdict"] == "0")
rd = csv.DictReader(open(tr)); h = rd.fieldnames; rows = list(rd)
for r in rows:
    if r["t"] == t0:
        r["hadFix"] = "0"
w = csv.DictWriter(open(tr, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N4 an accepted sparse cycle still a no-fix in the trace" 1 TRACE
fresh; python3 - "$SP" "$TR" <<'PY'
import csv, sys
sp, tr = sys.argv[1:3]
t0 = next(r["t"] for r in csv.DictReader(open(sp)) if r["verdict"] == "0")
rd = csv.DictReader(open(tr)); h = rd.fieldnames; rows = list(rd)
for r in rows:
    if r["t"] == t0:
        r["deescalated"] = "1"
w = csv.DictWriter(open(tr, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N5 an accepted sparse cycle that demoted the node" 1 TRACE
fresh; edit "$SP" '
for r in rows:
    r["priorX"] = r["prevX"]; r["priorY"] = r["prevY"]'
check "N6 every solve against the previous posterior (the old prior)" 1 PRIOR
fresh; sed -i 's/DC-11 sparse in Step: attempts \([0-9]*\), case A \([0-9]*\),/DC-11 sparse in Step: attempts \1, case A 999,/' "$T/f/new.log"
check "N7 the DC-11 line disagrees with the CSV" 1 CENSUS
fresh; edit "$T/f/relay/sparse_static_2p4_seed7.csv" '
rows[0]["rangeUpdates"] = "1"; rows[0]["positionFuses"] = "1"'
check "N8 a relayed cycle that fused a sparse point and range-updated" 1 RELAY
fresh; echo 0 > "$T/f/r_both.rc"
check "N9 --sparseCaseA with --sparseInStep accepted" 1 R
fresh; echo 134 > "$T/f/new.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS

printf '  G39 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
