#!/usr/bin/env bash
# G19 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# Every fixture is built outside the tree, in a mktemp directory, and each must produce the
# exit code stated for it. N0 matters most: the SAME run with the flag OFF must fail,
# because "no TDoA fix" is the state tier 1 has been in since the energy model started
# billing a TDoA-shaped exchange. N0b is the flag ON in an arm that never uses tier 1: the
# file is written and holds no rows, which a non-empty-file test would not notice (rule 12).
#
# The CSV fixtures are real rows from the L.2.1 done-when run (static_868, seed 7, 3600 s)
# with ONE thing removed or falsified each, so a gate that only counted rows, or only
# counted OK verdicts, would pass most of them. N11 is the first draft's own runaway row.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/tdoa_gate.py"
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
D=$(mktemp -d "${TMPDIR:-/tmp}/g19tdoa.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT

pass=0
fail=0
check() {  # check <name> <expected exit> <gate args...>
    local name=$1
    local want=$2
    shift 2
    python3 "$GATE" "$@" > "$D/out.txt" 2>&1
    local rc=$?
    if [ "$rc" -eq "$want" ]; then
        printf '  G19 ok    %-60s exit=%d\n' "$name" "$rc"
        # A negative must fail for ITS reason, not for whichever check happens to trip
        # first: show the reason, so a reader can see each one is the fault it names.
        [ "$want" -ne 0 ] && grep -m1 'FAIL' "$D/out.txt" | sed 's/^/            /'
        pass=$((pass + 1))
    else
        printf '  G19 FAIL  %-60s exit=%d expected %d\n' "$name" "$rc" "$want"
        sed 's/^/            /' "$D/out.txt" | head -6
        fail=$((fail + 1))
    fi
}
# summary <file> <fixCycles> <ok> <fused> <policy> <stampsOk> <repliesOk> <downlinks>
#         <exchanges> <stamps> <replies> <beyond> <tx> -- the run's two printed lines
summary() {
    printf '    TDoA tier 1: fix cycles %s (tier-1 set >= 3 ranges), stamps 0 on them; solves OK %s (chan 0, rssi 0, root 0), refused TOO_FEW 0; fused %s (policy %s)\n' \
        "$2" "$3" "$4" "$5" > "$1"
    printf '    TDoA billing: %s stamps in OK solves, replies billed on those cycles %s, downlinks %s; node tier-1 exchanges %s (one billed broadcast each), stamps %s against replies %s billed, stamps beyond a billed reply %s; tier-1 over the run: tx %s, rx 0, exchanges %s\n' \
        "$6" "$7" "$8" "$9" "${10}" "${11}" "${12}" "${13}" "$9" >> "$1"
}

H='t,cycle,nRanges,nStamps,repliesBilled,downlinkBilled,verdict,seededBy,iterations,refAnchor,tdoaX,tdoaY,rxx,rxy,ryy,chi2,chanOk,trueX,trueY,tdoaErrM,rssiOk,rssiErrM,fused,rejected,estErrM,nRoots,tieChosen,tieOther'
A1='7.5,2,4,4,4,0,OK,chan,3,109,-156.3497809,-15.95898313,7513.107688,3333.897102,1889.264274,0.01154653244,1,-131.0468448,14.81368775,39.8396266,1,424.0548254,1,0,17.7078879,0,-1,-1'
A2='10,3,6,6,6,0,OK,chan,2,109,-118.8331866,10.93502272,225.2833336,-153.7476377,150.1466211,0.1418265642,1,-133.0557742,17.04173686,15.47817676,1,409.0650185,1,0,17.76442823,0,-1,-1'
A3='12.5,4,6,6,6,0,OK,chan,3,102,-114.4483234,21.67824422,379.1354326,-59.35211569,1060.383801,1.9431403,1,-134.3838146,19.73177565,20.03029075,1,423.0681448,1,0,19.4792189,0,-1,-1'
R1='325,129,3,3,3,0,OK,root,1,109,-355.5504775,323.5246257,4818.789966,-5893.623285,8435.377328,2.0368226e-29,0,-357.395517,316.4946708,7.268042123,1,738.9140223,1,0,16.86456756,2,0.2445321117,45.9021634'
C1='450,179,3,3,3,0,COLLINEAR,none,0,102,0,0,0,0,0,0,0,-444.4628425,436.7872268,-1,1,400.7816215,0,0,2.906814125,0,-1,-1'
M1='1222.5,488,3,3,3,0,AMBIGUOUS,none,0,109,323.3288287,447.5089246,0,0,0,0,0,315.2569883,469.0661872,-1,1,1075.503551,0,0,15.23670218,2,0.5808238204,2.243860664'
E1='7.5,2,4,4,0,1,OK,chan,3,109,-156.3497809,-15.95898313,7513.107688,3333.897102,1889.264274,0.01154653244,1,-131.0468448,14.81368775,39.8396266,1,424.0548254,1,0,17.7078879,0,-1,-1'

# P1 -- positive control: four OK solves (three Chan, one three-anchor root) and a refusal,
# with the run's counters agreeing.
printf '%s\n' "$H" "$A1" "$A2" "$A3" "$R1" "$C1" > "$D/p1.csv"
summary "$D/p1.log" 5 4 4 4 19 19 0 5 22 22 0 5
check "P1 OK solves fused and billed, counters agree" 0 --stdout "$D/p1.log" "$D/p1.csv"

# P2 -- positive control, edge server: the stamps go over the backhaul, no reply billed.
printf '%s\n' "$H" "$E1" > "$D/p2.csv"
summary "$D/p2.log" 1 1 1 1 4 0 1 1 4 4 0 1
check "P2 edge-server row, downlink billed, no reply billed" 0 --stdout "$D/p2.log" "$D/p2.csv"

# N1 -- the file exists and holds no rows. A non-empty test would pass this.
printf '%s\n' "$H" > "$D/n1.csv"
check "N1 header only, zero rows" 1 "$D/n1.csv"

# N2 -- rows, but every solve refused: nothing produced.
printf '%s\n' "$H" "$C1" "$M1" > "$D/n2.csv"
check "N2 every solve refused, no TDoA fix" 1 "$D/n2.csv"

# N3 -- one OK in four: a trivial share.
printf '%s\n' "$H" "$A1" "$C1" "$M1" "$C1" > "$D/n3.csv"
check "N3 OK on 1 of 4 tier-1 fix cycles (< 0.5)" 1 "$D/n3.csv"

# N4 -- four stamps, three replies billed: a stamp that rode an unbilled frame.
printf '%s\n' "$H" "${A1/7.5,2,4,4,4,0/7.5,2,4,4,3,0}" "$A2" "$A3" > "$D/n4.csv"
check "N4 nStamps 4 > repliesBilled 3" 1 "$D/n4.csv"

# N5 -- a refused solve that was fused anyway.
printf '%s\n' "$H" "$A1" "$A2" "$A3" "${C1/400.7816215,0,0/400.7816215,1,0}" > "$D/n5.csv"
check "N5 fused=1 on a COLLINEAR row" 1 "$D/n5.csv"

# N6 -- a fix produced on an accepted cycle and silently dropped.
printf '%s\n' "$H" "$A1" "${A2/409.0650185,1,0/409.0650185,0,0}" "$A3" > "$D/n6.csv"
check "N6 OK, not rejected, never fused" 1 "$D/n6.csv"

# N7 -- a covariance that is not positive definite (|rxy| > sqrt(rxx ryy) = 634).
printf '%s\n' "$H" "$A1" "$A2" "${A3/-59.35211569/-1000.0}" > "$D/n7.csv"
check "N7 OK row with a non-PD covariance" 1 "$D/n7.csv"

# N8 -- the policy's own counter disagrees with the scenario's (rule 3).
summary "$D/n8.log" 5 4 4 3 19 19 0 5 22 22 0 5
check "N8 printed fused 4 but policy 3" 1 --stdout "$D/n8.log" "$D/p1.csv"

# N9 -- the run reports a stamp beyond a billed reply.
summary "$D/n9.log" 5 4 4 4 19 19 0 5 22 22 1 5
check "N9 stamps beyond a billed reply 1" 1 --stdout "$D/n9.log" "$D/p1.csv"

# N10 -- four stamps reported as a three-anchor root.
printf '%s\n' "$H" "$A1" "$A2" "${R1/325,129,3,3,3,0/325,129,4,4,4,0}" > "$D/n10.csv"
check "N10 seededBy root with 4 stamps" 1 "$D/n10.csv"

# N11 -- the first draft's own runaway row (static_868, t=2945): OK at 5.6e17 m, and its
# covariance (~1e35) makes err/sigma ~ 1, so a z-test alone would have passed it.
N11='2945,1177,5,5,5,0,OK,chan,10,109,2.674571398e+16,-5.636297339e+17,2.646740998e+33,-2.382648859e+34,2.363184032e+35,210.4559793,1,-6.185239075,-473.9145999,5.642639543e+17,1,330.0575044,1,0,16.46059156,0,-1,-1'
printf '%s\n' "$H" "$A1" "$A2" "$N11" > "$D/n11.csv"
check "N11 the first draft's runaway OK fix, 5.6e17 m out" 1 "$D/n11.csv"

# N12 -- the run counted one more fix cycle than the file holds (rule 12).
summary "$D/n12.log" 6 4 4 4 19 19 0 5 22 22 0 5
check "N12 printed fix cycles 6, CSV holds 5" 1 --stdout "$D/n12.log" "$D/p1.csv"

# N13 -- an edge-server row that also bills replies: charged twice for one delivery.
printf '%s\n' "$H" "${E1/7.5,2,4,4,0,1/7.5,2,4,4,4,1}" > "$D/n13.csv"
check "N13 edge-server row billing 4 replies" 1 "$D/n13.csv"

# N0 / N0b -- the real scenario, short: off, nothing is constructed and no file is
# written; on in an arm that never uses tier 1, the file holds a header and no rows.
if [ -x "$BIN" ]; then
    mkdir -p "$D/n0" "$D/n0b"
    "$BIN" --arms=static_868 --seed=7 --duration=150 --trace=false --outdir="$D/n0" \
        > "$D/n0.log" 2>&1
    rc=$?
    if [ "$rc" -ne 0 ]; then
        printf '  G19 FAIL  %-60s sim exit=%d\n' "N0 flag-off run" "$rc"
        fail=$((fail + 1))
    else
        check "N0 same run, flag off (no TDoA log)" 1 "$D/n0/tdoa_static_868_seed7.csv"
    fi
    "$BIN" --arms=static_2p4 --seed=7 --duration=150 --trace=false --tier1Tdoa=true \
        --outdir="$D/n0b" > "$D/n0b.log" 2>&1
    rc=$?
    if [ "$rc" -ne 0 ]; then
        printf '  G19 FAIL  %-60s sim exit=%d\n' "N0b flag-on static_2p4 run" "$rc"
        fail=$((fail + 1))
    else
        check "N0b flag on, arm never on tier 1 (header, no rows)" 1 \
            --stdout "$D/n0b.log" "$D/n0b/tdoa_static_2p4_seed7.csv"
    fi
else
    printf '  G19 ERROR %-60s no binary at %s\n' "N0 flag-off run" "$BIN"
    fail=$((fail + 1))
fi

printf '  G19 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
