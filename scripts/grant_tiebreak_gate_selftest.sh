#!/usr/bin/env bash
# G16 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# Every fixture is built outside the tree, in a mktemp directory, and each must produce
# the exit code stated for it. N0 is the one that matters most: the SAME scenario run
# with the flag OFF must fail the gate, because "mechanism not running" is exactly the
# state CaTblGrantCoordinator sat in, with a green unit suite, before L.1.4.
#
# The CSV fixtures each remove or invert ONE thing the gate claims to check, so a gate
# that only tested for a non-empty file would pass most of them and be caught here.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/grant_tiebreak_gate.py"
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
D=$(mktemp -d "${TMPDIR:-/tmp}/g16fix.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT

pass=0
fail=0
check() {  # check <name> <expected exit> <csv path>
    python3 "$GATE" "$3" > "$D/out.txt" 2>&1
    local rc=$?
    if [ "$rc" -eq "$2" ]; then
        printf '  G16 ok    %-52s exit=%d\n' "$1" "$rc"
        pass=$((pass + 1))
    else
        printf '  G16 FAIL  %-52s exit=%d expected %d\n' "$1" "$rc" "$2"
        sed 's/^/            /' "$D/out.txt" | head -6
        fail=$((fail + 1))
    fi
}

H='t,anchorId,targetId,otherAnchorId,source,verdict,action,windowStartS,windowEndS'
REQ1='162.553456,103,1,0,request,OK,grant,162.553456,172.553456'
REQ2='162.553456,104,1,0,request,OK,grant,162.553456,172.553456'
WIN='166.333142,104,1,103,gossip,WIN,keep,162.553456,172.553456'
DEF='166.771915,103,1,104,gossip,DEFER,yield,162.553456,172.553456'

# P1 -- the positive control: a pair as observed in the L.1.4 done-when run.
printf '%s\n' "$H" "$REQ1" "$REQ2" "$WIN" "$DEF" > "$D/p1.csv"
check "P1 observed WIN/DEFER pair" 0 "$D/p1.csv"

# N1 -- the file exists and is empty of verdicts. A non-empty test would pass this.
printf '%s\n' "$H" > "$D/n1.csv"
check "N1 header only, zero rows" 1 "$D/n1.csv"

# N2 -- grants and a WIN, but the lower id never defers: one-sided, not opposite.
printf '%s\n' "$H" "$REQ1" "$REQ2" "$WIN" > "$D/n2.csv"
check "N2 WIN without the matching DEFER" 1 "$D/n2.csv"

# N3 -- the rule inverted: the HIGHER id defers. Still opposite verdicts, so only the
# per-row rule check catches it.
printf '%s\n' "$H" \
    '166.333142,103,1,104,gossip,WIN,keep,162.553456,172.553456' \
    '166.771915,104,1,103,gossip,DEFER,yield,162.553456,172.553456' > "$D/n3.csv"
check "N3 rule inverted (higher id defers)" 1 "$D/n3.csv"

# N4 -- a DEFER about a DIFFERENT field node is not a contest over this one.
printf '%s\n' "$H" "$WIN" \
    '166.771915,103,2,104,gossip,DEFER,yield,162.553456,172.553456' > "$D/n4.csv"
check "N4 DEFER on a different target" 1 "$D/n4.csv"

# N5 -- windows that do not overlap are a handover, not a double grant.
printf '%s\n' "$H" "$WIN" \
    '176.771915,103,1,104,gossip,DEFER,yield,172.553456,182.553456' > "$D/n5.csv"
check "N5 non-overlapping windows" 1 "$D/n5.csv"

# N6 -- a yield recorded against a WIN verdict: action and verdict disagree.
printf '%s\n' "$H" "$DEF" '166.333142,104,1,103,gossip,WIN,yield,162.553456,172.553456' \
    > "$D/n6.csv"
check "N6 action contradicts verdict" 1 "$D/n6.csv"

# N0 -- the real thing with the mechanism off. Same scenario and seed as the gate's own
# run, flag at its default, and shorter because nothing here depends on duration: with
# no coordinator constructed there is no verdict file to write, however long it runs.
if [ -x "$BIN" ]; then
    mkdir -p "$D/n0"
    "$BIN" --arms=ca_tbl_full --seed=7 --duration=600 --trace=false --nFinders=4 \
        --finderPseudoAnchor=true --finderMode=2 --finderDispersal=1.0 \
        --sendHandshakeFrames=true --finderDigestPeriodS=20 --boxHalf=2000 --nGateways=5 \
        --gwArcDeg=140 --gwArcCentreDeg=180 --outdir="$D/n0" > "$D/n0.log" 2>&1
    rc=$?
    if [ "$rc" -ne 0 ]; then
        printf '  G16 FAIL  %-52s sim exit=%d\n' "N0 flag-off run" "$rc"
        fail=$((fail + 1))
    else
        check "N0 same run, flag off (no verdict file)" 1 \
            "$D/n0/grant_verdicts_ca_tbl_full_seed7.csv"
    fi
else
    printf '  G16 ERROR %-52s no binary at %s\n' "N0 flag-off run" "$BIN"
    fail=$((fail + 1))
fi

printf '  G16 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
