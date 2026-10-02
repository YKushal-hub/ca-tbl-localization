#!/usr/bin/env bash
# G45 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/mesh_links_gate_selftest.sh [<dir written by mesh_links_gate.sh>]
#   Without a directory it makes its own real run set first (about 60 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The plan's own negatives: star offered as all (N2), a
# DISCOVERY rebroadcast counted (N3), a peer polled as an anchor (N8), and a sleeping-tier
# rebroadcast (N5).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/mesh_links_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g45self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/mesh_links_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/all.log" ] || { echo "  G45 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G45 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G45 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G45 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G45 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G45 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
EV="$T/f/all/events_ca_tbl_full_seed7.csv"
# addrow <line>: append an events row to the mesh run
addrow() { printf '%s\n' "$1" >> "$EV"; }

fresh
check "P0 the real run set" 0 -

fresh; sed -i '3s/[0-9],/7,/' "$T/f/star/trace_ca_tbl_full_seed7.csv"
check "N0 --meshLinks=star moves an output" 1 NULL
fresh; grep "NW-0" "$T/f/all.log" >> "$T/f/def.log"
check "N1 the default prints the NW-0 line" 1 NULL
fresh; cp "$T/f/star/events_ca_tbl_full_seed7.csv" "$EV"
check "N2 star offered as all" 1 PAIRS
fresh; addrow "100.5,0,3,101,DISCOVERY,delivered,-90,pkt=$(( (502 << 20) + 7 ));hops=0"
check "N3 a DISCOVERY rebroadcast counted" 1 DISCOVERY
# The first trace row is on 868 MHz, so UWB (tier 2) is asleep in the first cycle.
fresh; addrow "3.1,2,503,1,DISCOVERY,delivered,-80,pkt=$(( (503 << 20) + 99 ));hops=0"
check "N4 a reception at the node on a sleeping radio" 1 ASLEEP
fresh; addrow "3.2,2,1,503,QUERY,delivered,-80,pkt=$(( (101 << 20) + 99 ));hops=2;wants=1"
check "N5 a rebroadcast from the node on a sleeping radio" 1 ASLEEP
fresh; for n in 2 3 102 103 104 105 501 502 503 504 600 601 602 603; do addrow "300.0,0,$n,1,QUERY,delivered,-90,pkt=$(( (101 << 20) + 4242 ));hops=1;wants=1"; done
check "N6 one frame relayed by more nodes than exist" 1 DEDUP
fresh; sed -i 's/node rebroadcasts billed [0-9]* frames/node rebroadcasts billed 0 frames/' "$T/f/all.log"
check "N7 the node's relays unbilled" 1 BILLED
fresh; sed -i 's/peers kept out of poll sets [0-9]*/peers kept out of poll sets 0/' "$T/f/all.log"
check "N8 the poll guard never fired (a peer polled as an anchor)" 1 BILLED
fresh; echo 0 > "$T/f/r_bad.rc"
check "N9 an unknown --meshLinks value accepted" 1 R
fresh; echo 134 > "$T/f/all.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS

printf '  G45 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
