#!/usr/bin/env bash
# G35 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/edge_uplink_gate_selftest.sh [<dir written by edge_uplink_gate.sh>]
#   Without a directory it makes its own real run set first (about 40 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N2  the uplink moved accuracy, not only energy;
#   N5  a tier that never transmitted is charged (CLAUDE.md sect 6);
#   N6  static_2p4 not billed at all -- the tree before SY-1.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/edge_uplink_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g35self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/edge_uplink_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G35 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G35 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G35 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G35 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G35 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G35 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv> <python body over rows (list of dicts)>
edit() {
    python3 - "$@" <<'PY'
import csv, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p))
h = rd.fieldnames
rows = list(rd)
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n")
w.writeheader()
w.writerows(rows)
PY
}
ON="$T/f/on/arm_summary.csv"
DEF="$T/f/def/arm_summary.csv"

fresh
check "P0 the real run set" 0 -

fresh; sed -i '2s/,/;/' "$T/f/off/arm_summary.csv"
check "N0 --edgeUplinkRanges=false moves an output" 1 NULL
fresh; grep "SY-1" "$T/f/on.log" | head -1 >> "$T/f/def.log"
check "N1 the default prints the SY-1 line" 1 NULL
fresh; edit "$ON" '
for r in rows:
    if r["arm"] == "static_2p4":
        r["rmseM"] = "%.6f" % (float(r["rmseM"]) + 1.0)'
check "N2 the uplink moved accuracy, not only energy" 1 ONLY
fresh; edit "$T/f/on/trace_static_2p4_seed7.csv" '
rows[5]["estX"] = "%.4f" % (float(rows[5]["estX"]) + 1.0)'
check "N3 a trace cell other than cumJ moved" 1 ONLY
fresh; edit "$ON" '
for r in rows:
    if r["arm"] == "static_2p4":
        r["tx_2p4"] = "0.1"'
check "N4 a tier-2 transmission fell" 1 TIERS
fresh; edit "$ON" '
for r in rows:
    if r["arm"] == "static_868":
        r["tx_uwb"] = "0.01"; r["j_uwb"] = "0.01"'
check "N5 a tier that never transmitted is charged" 1 TIERS
fresh; python3 - "$DEF" "$ON" <<'PY'
import csv, sys
d, o = sys.argv[1:3]
a = list(csv.DictReader(open(d)))
rd = csv.DictReader(open(o)); h = rd.fieldnames; b = list(rd)
for ra, rb in zip(a, b):
    if ra["arm"] == "static_2p4":
        rb.update(ra)
w = csv.DictWriter(open(o, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(b)
PY
check "N6 static_2p4 not billed (the tree before SY-1)" 1 REACH
fresh; python3 - "$T/f/on.log" <<'PY'
import re, sys
p = sys.argv[1]
s = open(p).read()
s = re.sub(r"(SY-1 edge uplink: 2\.4 GHz frames \d+ records \d+ airtime )([0-9.]+)",
           lambda m: m.group(1) + "%.9f" % (float(m.group(2)) + 0.5), s, count=2)
open(p, "w").write(s)
PY
check "N7 the stdout airtime disagrees with the bill" 1 POWER
fresh; echo 0 > "$T/f/r_node.rc"
check "N8 the flag accepted with the estimator on the node" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N9 a run the checks read did not exit 0" 1 RUNS

printf '  G35 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
