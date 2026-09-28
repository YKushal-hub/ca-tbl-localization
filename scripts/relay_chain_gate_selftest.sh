#!/usr/bin/env bash
# G17 / G17b negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# Every fixture is built outside the tree, in a mktemp directory, and each must produce the
# exit code stated for it. N0 is the absence form: the SAME scenario run with the flag OFF
# must fail, because "chain not running" is the state L.1.2 started from. N4 is the other
# form, and the one that actually happened: two rows copied from the FIRST DRAFT of L.1.2,
# which reported depth 2 through relays that had never fused a relayed range. A gate that
# only counted depth-2 rows would have passed that draft.
#
# The DIAG rows are as observed in ~/catbl_probe/v3/L.1.2/diag2/d_bh200_n4 -- a diagnostic
# build with each relay's exchanges moved off the cycle's shared instant (not the committed
# code; see the RelayChain comment). They are the only real rows at depth 2 there are, and
# they are the positive control for G17b.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/relay_chain_gate.py"
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
D=$(mktemp -d "${TMPDIR:-/tmp}/g17fix.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT

pass=0
fail=0
check() {  # check <name> <expected exit> <csv path> [--require-depth2]
    python3 "$GATE" ${4:-} "$3" > "$D/out.txt" 2>&1
    local rc=$?
    if [ "$rc" -eq "$2" ]; then
        printf '  G17 ok    %-56s exit=%d\n' "$1" "$rc"
        pass=$((pass + 1))
    else
        printf '  G17 FAIL  %-56s exit=%d expected %d\n' "$1" "$rc" "$2"
        sed 's/^/            /' "$D/out.txt" | head -6
        fail=$((fail + 1))
    fi
}

H='t,consumerId,relayId,leg,tier,hopDepth,maxRelayDepth,distM,carriedVarM2,inheritedVarM2,budgetM2,a2,verdict,sigmaRawM,sigmaEffM'
# depth 1, the measured tag's leg, both halves of the budget (as observed, diag2)
T1P='27.5000,1,2,tag,1,1,2,58.797094,32.684966,0.000000,24336.000000,pass,ADMIT,3.000000,5.197425'
T1B='10.0000,1,2,tag,1,1,2,63.758048,25129.375000,0.000000,24336.000000,budget,REJECT_DEPTH(A2),3.000000,0.000000'
# relay 3 ranges holder 2 and fuses it: the fuse row is what earns relay 3 depth 1
R1A='75.0000,3,2,relay,1,1,2,45.954707,126.000041,0.000000,24336.000000,pass,ADMIT,3.000000,7.569242'
FU1='75.0000,3,2,fuse,1,1,2,0.000000,200.448175,48.293431,24336.000000,-,FUSED,0.000000,0.000000'
# the measured tag ranges relay 3: depth 2, admitted, then refused on the budget as relay 3
# coasts and its composed covariance grows past 156^2
T2P='80.0000,1,3,tag,1,2,2,138.231779,710.482000,48.293431,24336.000000,pass,ADMIT,16.072078,22.757467'
T2B='110.0000,1,3,tag,1,2,2,171.466404,27301.510993,48.293431,24336.000000,budget,REJECT_DEPTH(A2),6.729460,0.000000'

# P1 -- the observed chain, G17 and G17b both.
printf '%s\n' "$H" "$T1B" "$T1P" "$R1A" "$FU1" "$T2P" "$T2B" > "$D/p1.csv"
check "P1 observed chain (diag2), G17" 0 "$D/p1.csv"
check "P1 observed chain (diag2), G17b" 0 "$D/p1.csv" --require-depth2

# P2 -- depth 1 only, as the committed code produces today: G17 passes, G17b must not.
printf '%s\n' "$H" "$T1B" "$T1P" > "$D/p2.csv"
check "P2 depth 1 only, G17" 0 "$D/p2.csv"
check "N7 depth 1 only, G17b (no fuse, no depth 2)" 1 "$D/p2.csv" --require-depth2

# N1 -- the file exists and is empty. A non-empty test would pass this.
printf '%s\n' "$H" > "$D/n1.csv"
check "N1 header only, zero rows" 1 "$D/n1.csv"

# N2 -- A2 labelled pass while the carried variance exceeds the budget.
printf '%s\n' "$H" "$T1B" "$T1P" \
    '12.5000,1,2,tag,1,1,2,63.758048,25129.375000,0.000000,24336.000000,pass,ADMIT,3.000000,5.0' \
    > "$D/n2.csv"
check "N2 a2=pass with carried > budget" 1 "$D/n2.csv"

# N3 -- the verdict says A2 refused, the reason says it passed.
printf '%s\n' "$H" "$T1B" "$T1P" \
    '12.5000,1,2,tag,1,1,2,58.797094,32.684966,0.000000,24336.000000,pass,REJECT_DEPTH(A2),3.000000,0.000000' \
    > "$D/n3.csv"
check "N3 REJECT_DEPTH with a2=pass" 1 "$D/n3.csv"

# N4 -- THE FIRST DRAFT: depth 2 with no relay ever having fused (its real rows).
printf '%s\n' "$H" "$T1B" "$T1P" \
    '102.5000,1,5,tag,1,2,2,34.772702,718518.540448,32.290448,500.000000,budget,REJECT_DEPTH(A2),3.000000,0.000000' \
    '105.0000,1,5,tag,1,2,2,34.772702,757569.043712,7.481212,500.000000,budget,REJECT_DEPTH(A2),3.000000,0.000000' \
    > "$D/n4.csv"
check "N4 first-draft rows: depth 2 from nowhere" 1 "$D/n4.csv"

# N5 -- the fuse row exists but AFTER the depth-2 use: provenance is in file order.
printf '%s\n' "$H" "$T1B" "$T1P" "$T2P" "$FU1" > "$D/n5.csv"
check "N5 fuse recorded after the depth it earned" 1 "$D/n5.csv"

# N6 -- the budget never binds: every candidate passes it.
printf '%s\n' "$H" "$T1P" "$R1A" "$FU1" "$T2P" > "$D/n6.csv"
check "N6 budget never refuses" 1 "$D/n6.csv"

# N8 -- no budget set at all (0 = off): a pass then is not a pass ON the budget.
printf '%s\n' "$H" \
    '27.5000,1,2,tag,1,1,2,58.797094,32.684966,0.000000,0.000000,pass,ADMIT,3.000000,5.197425' \
    '10.0000,1,2,tag,1,1,2,63.758048,25129.375000,0.000000,0.000000,pass,REJECT_GEOMETRY(A3),3.000000,0.000000' \
    > "$D/n8.csv"
check "N8 budget off (0), nothing on the budget" 1 "$D/n8.csv"

# N9 -- depth 2 reached and admitted, but never refused on the budget AT depth 2.
printf '%s\n' "$H" "$T1B" "$T1P" "$R1A" "$FU1" "$T2P" > "$D/n9.csv"
check "N9 depth 2 admitted only, G17b" 1 "$D/n9.csv" --require-depth2

# N10 -- an admitted range whose sigma_eff is BELOW sigma_raw: inflation that deflates.
printf '%s\n' "$H" "$T1B" \
    '27.5000,1,2,tag,1,1,2,58.797094,32.684966,0.000000,24336.000000,pass,ADMIT,3.000000,2.000000' \
    > "$D/n10.csv"
check "N10 ADMIT with sigma_eff < sigma_raw" 1 "$D/n10.csv"

# N11 -- depth 3 labelled pass under a ceiling of 2. Provenance is intact (4 fused through
# 3, which had fused through 2), so only the ceiling label is wrong.
FU2='265.0000,4,3,fuse,1,2,2,0.000000,175.779711,161.341236,24336.000000,-,FUSED,0.000000,0.000000'
printf '%s\n' "$H" "$T1B" "$T1P" "$R1A" "$FU1" "$FU2" \
    '267.5000,1,4,tag,1,3,2,62.436212,337.120948,161.341236,24336.000000,pass,ADMIT,3.000000,5.0' \
    > "$D/n11.csv"
check "N11 depth 3 passes a ceiling of 2" 1 "$D/n11.csv"
# ...and the same rows with the label the ceiling actually gives, which must pass.
printf '%s\n' "$H" "$T1B" "$T1P" "$R1A" "$FU1" "$FU2" \
    '267.5000,1,4,tag,1,3,2,61.832243,596.397022,161.341236,24336.000000,ceiling,REJECT_DEPTH(A2),3.000000,0.000000' \
    > "$D/p3.csv"
check "P3 depth 3 refused by the ceiling (diag2 row)" 0 "$D/p3.csv"

# N13 -- a fuse at depth 2 through a holder that never fused at depth 1.
printf '%s\n' "$H" "$T1B" "$T1P" \
    '265.0000,4,2,fuse,1,2,2,0.000000,175.779711,161.341236,24336.000000,-,FUSED,0.000000,0.000000' \
    > "$D/n13.csv"
check "N13 fuse at depth 2 through an unfused holder" 1 "$D/n13.csv"

# N12 -- the header of a different schema (no distM): refuse rather than misread columns.
printf '%s\n' 't,consumerId,relayId,leg,tier,hopDepth,maxRelayDepth,carriedVarM2,inheritedVarM2,budgetM2,a2,verdict,sigmaRawM,sigmaEffM' \
    > "$D/n12.csv"
check "N12 wrong header" 1 "$D/n12.csv"

# N0 -- the real thing with the mechanism off. Same scenario and seed as the gate's own
# run, --relayChain at its default: no chain is constructed, so no log is written.
if [ -x "$BIN" ]; then
    mkdir -p "$D/n0"
    "$BIN" --arms=static_2p4 --seed=7 --duration=600 --trace=false --boxHalf=200 --nTags=4 \
        --relayedRanging=true --maxRelayDepth=2 --relayVarBudget=24336 \
        --outdir="$D/n0" > "$D/n0.log" 2>&1
    rc=$?
    if [ "$rc" -ne 0 ]; then
        printf '  G17 FAIL  %-56s sim exit=%d\n' "N0 flag-off run" "$rc"
        fail=$((fail + 1))
    else
        check "N0 same run, flag off (no chain log)" 1 \
            "$D/n0/relay_chain_static_2p4_seed7.csv"
    fi
else
    printf '  G17 ERROR %-56s no binary at %s\n' "N0 flag-off run" "$BIN"
    fail=$((fail + 1))
fi

printf '  G17 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
