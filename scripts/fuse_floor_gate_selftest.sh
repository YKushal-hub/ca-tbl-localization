#!/usr/bin/env bash
# G63 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/fuse_floor_gate_selftest.sh [<dir written by fuse_floor_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N1-N4: a union formed where
# the old path applies, or one whose label hides which ranges it fused -- the failures that
# would let a fused fix pass as a single-tier one, or the reverse.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/fuse_floor_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g63self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/fuse_floor_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/end/fusefloor_ca_tbl_full_seed7.csv" ] || { echo "  G63 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G63 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G63 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G63 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G63 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G63 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
edit() {  # edit <csv> <python statement over rows>
    python3 - "$1" "$2" <<'PY'
import csv, sys
p, stmt = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
exec(stmt)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=cols, lineterminator="\n")
w.writeheader()
w.writerows(rows)
PY
}
E="$T/f/end/fusefloor_ca_tbl_full_seed7.csv"
M="$T/f/main/fusefloor_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$E" "r = next(r for r in rows if r['nActive'] == '2'); r['nActive'] = '3'; r['nFused'] = str(int(r['nFloor']) + 3)"
check "N1 a union formed when the active tier had three" 1 UNION
fresh; edit "$M" "r = next(r for r in rows if r['nActive'] == '0'); r['nFloor'] = '3'; r['nFused'] = '3'"
check "N2 a union formed when the floor alone had three" 1 UNION
fresh; edit "$E" "r = rows[0]; r['nFused'] = str(int(r['nFused']) + 1)"
check "N3 a set that is not the sum of its parts" 1 UNION
fresh; edit "$E" "r = next(r for r in rows if r['activeTier'] == 'UWB' and r['nActive'] != '0'); r['fixTier'] = '868MHz'"
check "N4 a fused UWB set labelled 868 MHz" 1 UNION
fresh; edit "$E" "r = next(r for r in rows if int(r['nFused']) >= 3); r['fixKind'] = '4'"
check "N5 Case D from three ranges or more" 1 KIND
fresh; edit "$E" "r = next(r for r in rows if r['fixKind'] == '4'); r['hadFix'] = '1'"
check "N6 a Case D cycle called a fix" 1 KIND
fresh; python3 - "$E" "$T/f/end/trace_ca_tbl_full_seed7.csv" <<'PY'
import csv, sys
r = next(r for r in csv.DictReader(open(sys.argv[1])) if r['nodeId'] == '1')
p = sys.argv[2]
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
for x in rows:
    if abs(float(x['t']) - float(r['t'])) < 1e-3:
        x['nRangesEkf'] = str(int(x['nRangesEkf']) + 1)
w = csv.DictWriter(open(p, 'w', newline=''), fieldnames=cols, lineterminator='\n'); w.writeheader(); w.writerows(rows)
PY
check "N7 tag 0's trace fused a different set" 1 TRACE
fresh; for f in "$T"/f/end/trace_*.csv "$T"/f/end/subjtrace_*.csv; do edit "$f" "[r.update(fixTier='2.4GHz') for r in rows if r['fixTier'] == 'UWB']"; done
check "N8 no UWB-delivered fix in the endgame" 1 UWBFIX
fresh; sed -i -E '0,/(N\.14 floor\+active fusion \(\S+\): sets )([0-9]+)/s//\199999/' "$T/f/end.log"
check "N9 an N.14 line the file does not support" 1 COUNTS
fresh; cp "$E" "$T/f/off/"
check "N10 the control writing a fusefloor file" 1 CONTROL
fresh; echo 0 > "$T/f/r_nostep.rc"
check "N11 the flag accepted without --sparseInStep" 1 R
fresh; echo 134 > "$T/f/end.rc"
check "N12 a run the checks read did not exit 0" 1 RUNS
fresh; edit "$M" "rows[:] = [r for r in rows if r['nActive'] != '0']"
check "N13 the short-floor case never formed" 1 ROWS

printf '  G63 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
