#!/usr/bin/env bash
# G36 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/phy_flags_gate_selftest.sh [<dir written by phy_flags_gate.sh>]
#   Without a directory it makes its own real run set first (about 30 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N3  the offset computed and printed but never applied to the engine;
#   N4  --sf868 not reaching the energy model (SF9 run billed at SF7 airtime).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/phy_flags_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g36self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/phy_flags_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/bw.log" ] || { echo "  G36 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G36 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G36 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G36 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G36 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G36 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# setcol <csv> <arm> <column> <value>
setcol() {
    python3 - "$@" <<'PY'
import csv, sys
p, arm, col, val = sys.argv[1:5]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
for r in rows:
    if r["arm"] == arm:
        r[col] = val
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}

fresh
check "P0 the real run set" 0 -

fresh; setcol "$T/f/expl/arm_summary.csv" ca_tbl_full cep50M 1.0
check "N0 the flags at their defaults move an output" 1 NULL
fresh; grep "SY-3 PHY" "$T/f/expl.log" >> "$T/f/def.log"
check "N1 the default prints the SY-3 line" 1 NULL
fresh; sed -i 's/sensitivity offset 3.01029996 dB/sensitivity offset 0 dB/' "$T/f/bw.log"
check "N2 a 250 kHz run reporting no sensitivity offset" 1 LINE
fresh; sed -i 's/868 MHz SF7 -119.9897 dBm/868 MHz SF7 -123 dBm/' "$T/f/bw.log"
check "N3 the offset printed but never applied to the engine" 1 ENGINE
fresh; setcol "$T/f/sf9/arm_summary.csv" static_868 airtime_868 "$(grep static_868 "$T/f/def/arm_summary.csv" | cut -d, -f19)"
check "N4 --sf868=9 billed at SF7 airtime" 1 AIR
fresh; setcol "$T/f/bw/arm_summary.csv" static_2p4 airtime_2p4 24.3
check "N5 2.4 GHz airtime that is no whole number of frames" 1 AIR
fresh; echo 0 > "$T/f/r_sf.rc"
check "N6 an SF outside SF7-SF12 accepted" 1 R
fresh; echo 134 > "$T/f/sf9.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G36 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
