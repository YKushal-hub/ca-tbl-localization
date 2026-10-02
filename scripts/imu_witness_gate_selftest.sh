#!/usr/bin/env bash
# G44 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/imu_witness_gate_selftest.sh [<dir written by imu_witness_gate.sh>]
#   Without a directory it makes its own real run set first (about 15 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N3  the raw fix replaced by the estimate -- the IMU checking itself;
#   N7  an absorbed refusal reported as a pass.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/imu_witness_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g44self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/imu_witness_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/model.log" ] || { echo "  G44 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G44 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G44 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G44 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G44 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G44 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
WM="$T/f/model/witness_ca_tbl_full_seed1.csv"
WE="$T/f/model_ev/witness_ca_tbl_full_seed1.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$T/f/shs/trace_ca_tbl_full_seed1.csv" 'rows[3]["estX"] = "1.0"'
check "N0 --imuWitnessSource=shs moves an output" 1 NULL
fresh; grep "DC-9" "$T/f/model.log" >> "$T/f/def.log"
check "N1 the default prints the DC-9 line" 1 NULL
fresh; edit "$WM" '
for r in rows[5:15]:
    r["wx"] = "%.6f" % (float(r["wx"]) + 40.0)'
check "N2 a track that is not the anchor + u" 1 TRACK
fresh; edit "$WM" '
for r in rows:
    r["fixX"] = r["estX"]; r["fixY"] = r["estY"]'
check "N3 the raw fix replaced by the estimate (the IMU checking itself)" 1 TEST
fresh; edit "$WM" '
r = next(r for r in rows if r["haveFix"] == "1"); r["tol"] = "%.6f" % (float(r["tol"]) * 2)'
check "N4 a tolerance that is not 3 sqrt(trace R + trace Sigma)" 1 TEST
fresh; edit "$WM" '
r = next(r for r in rows if r["refused"] == "1"); r["disagree"] = "0.000001"'
check "N5 a refusal with the disagreement inside the tolerance" 1 COUNTS
fresh; edit "$WM" '
r = next(r for r in rows if r["refused"] == "1"); r["absorbed"] = "1"'
sed -i 's/absorbed by the evidence gate 0/absorbed by the evidence gate 1/' "$T/f/model.log"
check "N6 a refusal absorbed with the evidence gate off" 1 COUNTS
fresh; edit "$WE" '
r = next(r for r in rows if r["absorbed"] == "1"); r["refused"] = "0"'
check "N7 an absorbed refusal reported as a pass" 1 COUNTS
fresh; echo 0 > "$T/f/r_noimu.rc"
check "N8 model accepted without --imuPredict" 1 R
fresh; echo 134 > "$T/f/model.rc"
check "N9 a run the checks read did not exit 0" 1 RUNS

printf '  G44 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
