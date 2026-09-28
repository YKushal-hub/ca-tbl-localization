#!/usr/bin/env bash
# G18 / G18b negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# Every fixture is built outside the tree, in a mktemp directory, and each must produce the
# exit code stated for it. N0 matters most: the SAME scenario with the flag OFF must fail,
# because "nothing routed" is exactly the state CaTblRoutingTable sat in, with a green unit
# suite, before L.1.1. N0b is its G18b twin: a real ETX-routed log offered as evidence that
# the confidence metric routed must fail.
#
# The CSV fixtures each remove or falsify ONE thing the gate claims to check, so a gate that
# only tested for a non-empty file, or only counted divergences, would pass most of them.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/fix_routing_gate.py"
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
D=$(mktemp -d "${TMPDIR:-/tmp}/g18fix.XXXXXX") || exit 2
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
        printf '  G18 ok    %-58s exit=%d\n' "$name" "$rc"
        pass=$((pass + 1))
    else
        printf '  G18 FAIL  %-58s exit=%d expected %d\n' "$name" "$rc" "$want"
        sed 's/^/            /' "$D/out.txt" | head -6
        fail=$((fail + 1))
    fi
}
summary() {  # summary <file> <selections> <divergences> -- the run's printed counters
    printf '      selections %s (tables %s), divergences %s (tables %s); next hop chosen\n' \
        "$2" "$2" "$3" "$3" > "$1"
}

H='t,originId,hop,senderId,etxNext,etxHops,etxCost,confNext,confHops,confCost,metric,chosenNext,chosenClass,diverged,outcome'
# Real rows, from the L.1.1 done-when run (static_2p4, seed 7, 600 s, --nTags=6).
A0='12.5000,2,0,2,3,2,2.000000,3,2,29.057619,etx,3,relay,0,forwarded'
A1='12.5000,2,1,3,105,1,1.000000,105,1,27.490496,etx,105,gateway,0,delivered'
B0='25.0000,4,0,4,3,2,2.816735,6,3,70.722240,etx,3,relay,1,forwarded'
B1='25.0000,4,1,3,105,1,1.052811,105,1,42.608470,etx,105,gateway,0,delivered'
C0='27.5000,4,0,4,5,3,6.839566,6,3,70.722240,confidence,6,relay,1,forwarded'
C1='27.5000,4,1,6,3,2,2.058533,3,2,60.253930,confidence,3,relay,0,forwarded'
C2='27.5000,4,2,3,105,1,1.000000,105,1,27.490496,confidence,105,gateway,0,delivered'

# P1 -- positive control: a relayed delivery, one of them through a diverged hop.
printf '%s\n' "$H" "$A0" "$A1" "$B0" "$B1" > "$D/p1.csv"
summary "$D/p1.log" 4 1
check "P1 relayed deliveries with a divergence" 0 --stdout "$D/p1.log" "$D/p1.csv"

# P2 -- positive control for G18b: the confidence metric sends a fix where ETX would not.
printf '%s\n' "$H" "$C0" "$C1" "$C2" > "$D/p2.csv"
summary "$D/p2.log" 3 1
check "P2 confidence-routed fix, diverged hop carried" 0 --require-confidence \
    --stdout "$D/p2.log" "$D/p2.csv"

# N1 -- the file exists and holds no decisions. A non-empty test would pass this.
printf '%s\n' "$H" > "$D/n1.csv"
check "N1 header only, zero rows" 1 "$D/n1.csv"

# N2 -- divergence present, but every fix went straight to a gateway: nothing relayed.
printf '%s\n' "$H" \
    '12.5000,5,0,5,105,1,1.000000,105,1,20.000000,etx,105,gateway,0,delivered' \
    '15.0000,5,0,5,105,1,1.000000,106,1,10.000000,etx,105,gateway,1,delivered' > "$D/n2.csv"
check "N2 direct deliveries only, no relay next hop" 1 "$D/n2.csv"

# N3 -- relayed and delivered, but the two metrics never disagreed.
printf '%s\n' "$H" "$A0" "$A1" \
    '25.0000,4,0,4,3,2,2.816735,3,2,70.722240,etx,3,relay,0,forwarded' "$B1" > "$D/n3.csv"
check "N3 relayed, but Divergences() zero" 1 "$D/n3.csv"

# N4 -- the frame went to the confidence pick while the row says ETX was routing.
printf '%s\n' "$H" "$A0" "$A1" \
    '25.0000,4,0,4,3,2,2.816735,6,3,70.722240,etx,6,relay,1,forwarded' \
    '25.0000,4,1,6,105,1,1.052811,105,1,42.608470,etx,105,gateway,0,delivered' > "$D/n4.csv"
check "N4 next hop taken is not the active metric's pick" 1 "$D/n4.csv"

# N5 -- a relayed hop with no forward behind it: a route from nowhere.
printf '%s\n' "$H" "$A1" "$B0" "$B1" > "$D/n5.csv"
check "N5 hop 1 with no forwarded hop 0 before it" 1 "$D/n5.csv"

# N6 -- the diverged flag lies about the two picks.
printf '%s\n' "$H" "$A0" "$A1" \
    '25.0000,4,0,4,3,2,2.816735,6,3,70.722240,etx,3,relay,0,forwarded' "$B1" \
    '30.0000,4,0,4,105,1,1.000000,105,1,9.000000,etx,105,gateway,1,delivered' > "$D/n6.csv"
check "N6 diverged flag contradicts etxNext/confNext" 1 "$D/n6.csv"

# N7 -- "delivered" to a field node id dressed as a gateway.
printf '%s\n' "$H" "$A0" "$B0" \
    '25.0000,4,1,3,5,1,1.052811,5,1,42.608470,etx,5,gateway,0,delivered' > "$D/n7.csv"
check "N7 delivered to relay id 5 labelled gateway" 1 "$D/n7.csv"

# N8 -- a good ETX log offered as evidence the confidence metric routed.
check "N8 ETX-routed log under --require-confidence" 1 --require-confidence "$D/p1.csv"

# N9 -- the CSV and the run's own counters disagree (rule 3).
summary "$D/n9.log" 4 2
check "N9 printed divergences 2, CSV holds 1" 1 --stdout "$D/n9.log" "$D/p1.csv"

# N10 -- confidence-routed, but the only diverged hop was lost: never carried.
printf '%s\n' "$H" \
    '27.5000,4,0,4,5,3,6.839566,6,3,70.722240,confidence,6,relay,1,lost' \
    '30.0000,2,0,2,3,2,2.000000,3,2,29.057619,confidence,3,relay,0,forwarded' \
    '30.0000,2,1,3,105,1,1.000000,105,1,27.490496,confidence,105,gateway,0,delivered' \
    > "$D/n10.csv"
check "N10 G18b: the diverged hop was never carried" 1 --require-confidence "$D/n10.csv"

# N0 / N0b -- the real scenario. Same flags as the gate's run, shorter: with the flag off
# no router is constructed and no log is written however long it runs, and the ETX log
# needs only to exist to be refused by G18b.
if [ -x "$BIN" ]; then
    F="--arms=static_2p4 --seed=7 --duration=150 --trace=false --relayedRanging=true --nTags=6"
    mkdir -p "$D/n0" "$D/n0b"
    # shellcheck disable=SC2086
    "$BIN" $F --outdir="$D/n0" > "$D/n0.log" 2>&1
    rc=$?
    if [ "$rc" -ne 0 ]; then
        printf '  G18 FAIL  %-58s sim exit=%d\n' "N0 flag-off run" "$rc"
        fail=$((fail + 1))
    else
        check "N0 same run, flag off (no routing log)" 1 \
            "$D/n0/fix_routing_static_2p4_seed7.csv"
    fi
    # shellcheck disable=SC2086
    "$BIN" $F --fixRouting=true --outdir="$D/n0b" > "$D/n0b.log" 2>&1
    rc=$?
    if [ "$rc" -ne 0 ]; then
        printf '  G18 FAIL  %-58s sim exit=%d\n' "N0b ETX run" "$rc"
        fail=$((fail + 1))
    else
        check "N0b real ETX-routed log under --require-confidence" 1 --require-confidence \
            "$D/n0b/fix_routing_static_2p4_seed7.csv"
    fi
else
    printf '  G18 ERROR %-58s no binary at %s\n' "N0 flag-off run" "$BIN"
    fail=$((fail + 1))
fi

printf '  G18 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
