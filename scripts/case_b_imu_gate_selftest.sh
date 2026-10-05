#!/usr/bin/env bash
# G42 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/case_b_imu_gate_selftest.sh [<dir written by case_b_imu_gate.sh>]
#   Without a directory it makes its own real run set first (about 30 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most:
#   N3  a Case B prior that is NOT the previous posterior + u -- the heading not carried in.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/case_b_imu_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g42self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/case_b_imu_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/imu1.log" ] || { echo "  G42 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G42 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G42 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G42 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G42 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G42 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv...> -- <python body over rows (list of dicts)>
edit() {
    python3 - "$@" <<'PY'
import csv, sys
args = sys.argv[1:]
cut = args.index("--")
for p in args[:cut]:
    rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
    exec(args[cut + 1])
    w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}

fresh
check "P0 the real run set" 0 -

fresh; edit "$T"/f/imu1/sparse_ca_tbl_full_seed1.csv -- 'rows[0]["priorSource"] = "0"'
check "N0 an IMU-run solve labelled constant-velocity" 1 LABEL
fresh; edit "$T"/f/cv2/sparse_ca_tbl_full_seed2.csv -- 'rows[0]["priorSource"] = "1"'
check "N1 a constant-velocity solve labelled IMU" 1 LABEL
fresh; edit "$T"/f/imu*/sparse_ca_tbl_full_seed*.csv -- '
for r in rows:
    if r["fixKind"] == "3":
        r["fixKind"] = "2"'
check "N2 no Case B under --imuPredict" 1 CASEB
fresh; edit "$T"/f/imu1/sparse_ca_tbl_full_seed1.csv -- '
rows[0]["priorX"] = rows[0]["prevX"]; rows[0]["priorY"] = rows[0]["prevY"]'
check "N3 a prior that is not the previous posterior + u" 1 PRIOR
fresh; echo 134 > "$T/f/imu3.rc"
check "N4 a run the checks read did not exit 0" 1 RUNS

printf '  G42 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
