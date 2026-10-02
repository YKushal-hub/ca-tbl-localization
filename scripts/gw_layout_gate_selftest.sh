#!/usr/bin/env bash
# G38 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/gw_layout_gate_selftest.sh [<dir written by gw_layout_gate.sh>]
#   Without a directory it makes its own real run set first (about 20 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N3  a cluster that ignores the base camp;
#   N4  a shape applied to one tier's list and not the others (not one tri-band node).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/gw_layout_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g38self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/gw_layout_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/file.log" ] || { echo "  G38 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G38 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G38 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G38 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G38 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G38 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set" 0 -

fresh; sed -i '3s/[0-9],/7,/' "$T/f/legacy/trace_static_868_seed7.csv"
check "N0 --gwLayout=legacy moves an output" 1 NULL
fresh; grep "MS-2" "$T/f/perimeter.log" >> "$T/f/def.log"
check "N1 the default prints an MS-2 line" 1 NULL
fresh; sed -i 's/ 102 -1800 -4500 / 102 -1750 -4500 /' "$T/f/perimeter.log"
check "N2 a perimeter gateway 50 m off its place" 1 SHAPES
fresh; python3 - "$T/f/cluster_bc.log" "$T/f/cluster.log" <<'PY'
import re, sys
bc, plain = sys.argv[1:3]
line = re.search(r"  MS-2 layout cluster:.*", open(plain).read()).group(0)
s = open(bc).read()
s = re.sub(r"  MS-2 layout cluster:.*", line, s)
open(bc, "w").write(s)
PY
check "N3 a cluster that ignores the base camp" 1 SHAPES
fresh; python3 - "$T/f/star/deployment.json" "$T/f/def/deployment.json" <<'PY'
import json, sys
p, legacy = sys.argv[1:3]
d = json.load(open(p)); l = json.load(open(legacy))
d["anchors"]["2"] = l["anchors"]["2"]
json.dump(d, open(p, "w"))
PY
check "N4 the shape on two tier lists and the legacy ring on UWB" 1 BUILT
fresh; sed -i 's/ 104 5000 0/ 104 7000 0/' "$T/f/file.log"
check "N5 a file row outside the box, not clamped" 1 FILE
fresh; echo 0 > "$T/f/r_topo.rc"
check "N6 --gwLayout accepted under the legacy topology" 1 R
fresh; echo 134 > "$T/f/star.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G38 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
