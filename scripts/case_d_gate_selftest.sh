#!/usr/bin/env bash
# G41 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/case_d_gate_selftest.sh [<dir written by case_d_gate.sh>]
#   Without a directory it makes its own real run set first (about 15 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N2  a range applied although its NIS failed the guard;
#   N6  a Case D cycle that never reached the decision (the coast, PCS 0);
#   N5  a Case D cycle that demoted the node.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/case_d_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g41self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/case_d_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G41 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G41 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G41 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G41 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G41 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G41 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
CD="$T/f/on/cased_ca_tbl_full_seed7.csv"
# at <file> <column> <value>: set column on the trace/imu row of the first Case D cycle
at() {
    python3 - "$CD" "$@" <<'PY'
import csv, sys
cd, p, col, val = sys.argv[1:5]
t0 = float(next(csv.DictReader(open(cd)))["t"])
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
for r in rows:
    if abs(float(r["t"]) - t0) < 1e-6:
        r[col] = val
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}

fresh
check "P0 the real run set" 0 -

fresh; edit "$T/f/off/trace_ca_tbl_full_seed7.csv" 'rows[3]["estX"] = "1.0"'
check "N0 --caseD=false moves an output" 1 NULL
fresh; grep "DC-4" "$T/f/on.log" >> "$T/f/base.log"
check "N1 the run without the flag prints the DC-4 line" 1 NULL
fresh; edit "$CD" 'rows[0]["nis"] = "9.5"'
check "N2 a range applied although its NIS failed the guard" 1 GUARD
fresh; at "$T/f/on/trace_ca_tbl_full_seed7.csv" hadFix 1
check "N3 a Case D cycle recorded as a position fix" 1 TRACE
fresh; at "$T/f/on/imu_ca_tbl_full_seed7.csv" imuPredicted 0
check "N4 a Case D cycle predicted without the IMU" 1 TRACE
fresh; edit "$CD" 'rows[0]["deescalated"] = "1"'
check "N5 a Case D cycle that demoted the node" 1 TRACE
fresh; edit "$CD" 'rows[0]["pcs"] = "0.000000"'
check "N6 a Case D cycle that never reached the decision (PCS 0)" 1 DECISION
fresh; edit "$CD" '
for r in rows:
    r["wantedEscalation"] = "0"'
check "N7 no Case D cycle ever wanted to escalate" 1 DECISION
fresh; sed -i 's/DC-4 Case D: cycles \([0-9]*\),/DC-4 Case D: cycles 999,/' "$T/f/on.log"
check "N8 the DC-4 line disagrees with the CSV" 1 LINE
fresh; echo 0 > "$T/f/r_nosparse.rc"
check "N9 --caseD accepted without --sparseInStep" 1 R
fresh; sed -i '/WARNING: --caseD without --imuPredict/d' "$T/f/w_noimu.log"
check "N10 no warning for Case D on a constant-velocity prediction" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N11 a run the checks read did not exit 0" 1 RUNS

printf '  G41 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
