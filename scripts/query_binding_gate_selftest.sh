#!/usr/bin/env bash
# G32 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/query_binding_gate_selftest.sh [<dir written by query_binding_gate.sh>]
#   Without a directory it makes its own real run set first (about 30 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N0  per-tier that binds nothing -- every driver still on radio 0, the tree before NW-3;
#   N3  a 2.4 GHz driver on its own radio but billed at 868 MHz airtime;
#   N2  a `first` record whose airtime no longer shows the defect it is there to show.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/query_binding_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g32self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/query_binding_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/per.log" ] || { echo "  G32 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G32 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G32 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G32 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G32 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G32 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# relog <log> <python expression over the line m (a re.Match) giving the replacement line>
relog() {
    python3 - "$@" <<'PY'
import re, sys
p, expr = sys.argv[1:3]
pat = re.compile(r"(    NW-3 query driver tier (\d): node (\d+) radio (-?\d+), QUERY sent (\d+) airtime ([0-9.eE+-]+) s \((first|per-tier)\))")
s = open(p).read()
out, hit = [], 0
for line in s.split("\n"):
    m = pat.fullmatch(line)
    if m:
        hit += 1
        line = eval(expr)
    out.append(line)
assert hit, "fixture: no NW-3 line"
open(p, "w").write("\n".join(out))
PY
}
L="'    NW-3 query driver tier %s: node %s radio %s, QUERY sent %s airtime %s s (%s)'"

fresh
check "P0 the real run set" 0 -

fresh; relog "$T/f/per.log" "$L % (m[2], m[3], '0', m[5], m[6], m[7])"
check "N0 per-tier that binds nothing (the tree before NW-3)" 1 PER
fresh; relog "$T/f/first.log" "$L % (m[2], m[3], m[2], m[5], m[6], m[7])"
check "N1 the first record reports each tier on its own radio" 1 FIRST
fresh; relog "$T/f/first.log" "$L % (m[2], m[3], m[4], m[5], '%.9f' % (27*0.051456 + 26*0.00804 + 27*0.000186591), m[7])"
check "N2 the first record billed correctly (no defect left to show)" 1 FIRST
fresh; relog "$T/f/per.log" "$L % (m[2], m[3], m[4], m[5], ('%.9f' % (int(m[5]) * 0.051456)) if m[2] == '1' else m[6], m[7])"
check "N3 the 2.4 GHz driver on its radio but at 868 MHz airtime" 1 PER
fresh; grep "NW-3" "$T/f/first.log" >> "$T/f/def.log"
check "N4 the default prints the binding" 1 NULL
fresh; sed -i '2s/,/;/' "$T/f/first/arm_summary.csv"
check "N5 explicit first moves an output" 1 NULL
fresh; echo 0 > "$T/f/r_noq.rc"
check "N6 the flag accepted without queries" 1 R
fresh; echo 134 > "$T/f/per.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G32 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
