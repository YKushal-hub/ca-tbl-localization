#!/usr/bin/env bash
# G40 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/imu_predict_gate_selftest.sh [<dir written by imu_predict_gate.sh>]
#   Without a directory it makes its own real run set first (about 10 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most (the plan's own list):
#   N2  the constant-velocity prediction on a coasted cycle -- the tree before DC-1;
#   N4  u = Delta (no error) claimed as the error model;
#   N8  an inflated sigma;
#   N9  an independent per-step Q in place of the accumulated covariance (fails NEES).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/imu_predict_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g40self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/imu_predict_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G40 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G40 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G40 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G40 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G40 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G40 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv glob...> -- <python body over rows (list of dicts)>
edit() {
    python3 - "$@" <<'PY'
import csv, sys
args = sys.argv[1:]
cut = args.index("--")
files, body = args[:cut], args[cut + 1]
for p in files:
    rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
    exec(body)
    w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}
ON="$T/f/on/imu_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$T/f/off/trace_ca_tbl_full_seed7.csv" -- 'rows[3]["estX"] = "1.0"'
check "N0 --imuPredict=false moves an output" 1 NULL
fresh; grep "DC-1" "$T/f/on.log" >> "$T/f/def.log"
check "N1 the default prints the DC-1 line" 1 NULL
fresh; edit "$ON" -- '
for a, b in zip(rows, rows[1:]):
    if b["hadFix"] == "0":
        b["estX"] = "%.6f" % (float(a["estX"]) + 0.5)
        break'
check "N2 a coasted cycle not moved by u (constant velocity)" 1 MOTION
fresh; edit "$ON" -- 'rows[10]["imuPredicted"] = "0"'
check "N3 a cycle predicted without the control" 1 MOTION
fresh; edit "$ON" -- '
for r in rows:
    r["ux"] = r["dxTrue"]; r["uy"] = r["dyTrue"]'
check "N4 u = Delta (no error) claimed as the error model" 1 MODEL
fresh; edit "$ON" -- '
for r in rows:
    for k in ("sigXX", "sigXY", "sigYY"):
        r[k] = "%.9f" % (4 * float(r[k]))'
check "N5 the accumulated covariance inflated x4" 1 MODEL
fresh; edit "$ON" -- '
for r in rows:
    if float(r["dxTrue"]) != 0.0:
        r["quxx"] = r["sigXX"]; r["quxy"] = r["sigXY"]; r["quyy"] = r["sigYY"]'
check "N6 Q_u the whole accumulation, not its increment" 1 QU
fresh; edit "$ON" -- '
r = next(r for r in rows if r["hadFix"] == "0"); r["reset"] = "1"'
check "N7 a reset on a cycle with no fused fix" 1 QU
fresh; edit "$T"/f/s*/imu_ca_tbl_full_seed*.csv -- '
for r in rows:
    for k in ("sigXX", "sigXY", "sigYY"):
        r[k] = "%.9f" % (4 * float(r[k]))'
check "N8 an inflated sigma in every seed" 1 NEES
fresh; edit "$T"/f/s*/imu_ca_tbl_full_seed*.csv -- '
for r in rows:
    r["sigXX"] = r["quxx"]; r["sigXY"] = r["quxy"]; r["sigYY"] = r["quyy"]'
check "N9 an independent per-step Q in place of the accumulation" 1 NEES
fresh; echo 134 > "$T/f/s3.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS

printf '  G40 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
