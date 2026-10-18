#!/usr/bin/env bash
# G97 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/imu_stride_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g97self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/imu_stride_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.log" ] || { echo "  G97 selftest: no real run set in $REAL"; exit 2; }
pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G97 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G97 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G97 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 3 > "$T/f/on.rc"; check "N1 the stride run crashed" 1 RUNS
fresh; sed -i '/stride noise/d' "$T/f/on.log"; check "N2 the flag silently not taking effect" 1 LINE
fresh; f=$(ls "$T/f/on"/imu_ca_tbl_full_seed*.csv); python3 - "$f" <<'PY'
import csv, sys
p = sys.argv[1]; rows = list(csv.DictReader(open(p))); h = rows[0].keys()
for r in rows:
    r["ux"], r["uy"] = r["dxTrue"], r["dyTrue"]
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=list(h)); w.writeheader(); w.writerows(rows)
PY
check "N3 stride noise absent (u = d exactly)" 1 NOISE
fresh; f=$(ls "$T/f/on"/imu_ca_tbl_full_seed*.csv); python3 - "$f" <<'PY'
import csv, sys, math
p = sys.argv[1]; rows = list(csv.DictReader(open(p))); h = rows[0].keys()
for r in rows:
    dx, dy, ux, uy = (float(r[k]) for k in ("dxTrue", "dyTrue", "ux", "uy"))
    r["ux"], r["uy"] = str(dx + 2.0 * (ux - dx)), str(dy + 2.0 * (uy - dy))
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=list(h)); w.writeheader(); w.writerows(rows)
PY
check "N4 stride noise twice the stated size" 1 NOISE
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N5 the default not being 0" 1 OFF
fresh; echo "  DC-1: stride noise 5 % per step" >> "$T/f/def.log"; check "N6 the noise leaking into the default" 1 CONTROL
fresh; echo 0 > "$T/f/r_noimu.rc"; check "N7 accepted without --imuPredict" 1 REFUSE
fresh; echo 0 > "$T/f/r_neg.rc"; check "N8 a negative value accepted" 1 REFUSE
echo "  G97 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
