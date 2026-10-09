#!/usr/bin/env bash
# G24 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/spec_profile_gate_selftest.sh [<dir written by spec_profile_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# G24 FAILS on the real run set today: two mechanisms have a zero population under the
# preset (TECH_DEBT N.15). So P0 asserts that those two are the ONLY failures, and each
# fixture -- a copy of the real set with one thing changed -- must add a FAIL of the check
# it names. The flag-efficacy half of G24 (the preset with one flag off, per mechanism) is
# made of real runs and needs no fixture: each EFF line already shows the mechanism off.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/spec_profile_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g24self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/spec_profile_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/main.log" ] || { echo "  G24 selftest: no real run set in $REAL"; exit 2; }
KNOWN="POP (Case A fixes|arbitration pre-emptions|queue gossip items|grant tie-break verdicts|subjects' TDoA solves)"

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check_pop_ok() {  # check_pop_ok <name> <label that must stay ok>
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    if grep -Eq "G24 ok +$2 " "$T/out.txt"; then
        printf '  G24 ok    %-66s\n' "$1"
        grep -m1 -E "G24 ok +$2 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G24 FAIL  %-66s %s is not ok\n' "$1" "$2"
        grep -E "$2" "$T/out.txt" | sed 's/^ */            /' | head -3
        fail=$((fail + 1))
    fi
}
check() {  # check <name> <label that must FAIL, or - for the baseline>
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local ok=0
    if [ "$2" = - ]; then
        # the baseline: failures are exactly the known zero populations
        local other
        other=$(grep -E "G24 (FAIL|MISSING)" "$T/out.txt" | grep -vE "$KNOWN" | wc -l)
        local known
        known=$(grep -E "G24 FAIL" "$T/out.txt" | grep -cE "$KNOWN")
        [ "$rc" -eq 1 ] && [ "$other" -eq 0 ] && [ "$known" -eq 5 ] && ok=1
    else
        grep -Eq "G24 (FAIL|MISSING) +$2" "$T/out.txt" && [ "$rc" -eq 1 ] && ok=1
    fi
    if [ "$ok" -eq 1 ]; then
        printf '  G24 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" != - ] && grep -m1 -E "G24 (FAIL|MISSING) +$2" "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G24 FAIL  %-66s exit=%d, failing check %s\n' "$1" "$rc" "$2"
        grep -E "G24 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -8
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set fails only on its five zero populations" -
fresh; f=$(ls "$T"/f/eq_flags/trace_*.csv | head -1); sed -i '2s/,/,9/' "$f"
check "N1 the preset and its flags written out disagree" EQUIV
fresh; sed -i '/^boxHalf: explicit/d' "$T/f/end/spec_profile.txt"
check "N2 an explicit flag not recorded as kept" FILE
fresh; sed -i -E 's/solves OK [0-9]+/solves OK 0/' "$T/f/main.log" "$T/f/main_s8.log" "$T/f/main_s9.log"
check "N3 a mechanism with no population in any of the three seeds (TDoA)" "POP tier-1 TDoA solves"
fresh; sed -i -E '/TABLE CI:/d' "$T/f/main.log" "$T/f/main_s8.log" "$T/f/main_s9.log"
check "N4 a mechanism whose evidence line is missing in every seed" "POP CI table fuses"
fresh; sed -i -E 's/solves OK [0-9]+/solves OK 0/' "$T/f/main_s8.log" "$T/f/main_s9.log"
check_pop_ok "P1 a population present in one seed only is not a zero (N.19)" "POP tier-1 TDoA solves"
fresh; rm -rf "$T/f/end_s9" "$T/f/end_s9.log"; echo 134 > "$T/f/end_s9.rc"
check "N12 an extra seed's run did not exit 0" RUNS
fresh; printf '  DC-4 Case D: cycles 5, updates applied 5, refused by NIS 0\n' >> "$T/f/off_caseD.log"
check "N5 a mechanism still firing with its flag off" "EFF caseD"
fresh; echo 0 > "$T/f/r_topo.rc"
check "N6 --specProfile accepted with --topology=1" R
fresh; echo 134 > "$T/f/gasp.rc"
check "N7 a run did not exit 0" RUNS
fresh; echo 0 > "$T/f/r_n12.rc"; sed -i '/needs --fixRoutingSubjects/d' "$T/f/r_n12.log"
check "N8 the preset accepted with --fixRoutingSubjects=false (N.12)" R12
fresh; sed -i -E '0,/NW-2 weights alpha 0 beta 0 /s//NW-2 weights alpha 1 beta 1 /' "$T/f/main.log"
check "N9 a router not running the preset's weights (0,0,1,1)" WEIGHTS
fresh; echo "NOTE: PCS form legacy resolved: up 868/2p4/uwb 3.000016 2.023835 1.501074; down 2p4/uwb 2.3611 1.681203" >> "$T/f/x_pcsleg.log"
check "N13 the preset's calibrated thresholds applied to a legacy score" PCSL
fresh; echo 1 > "$T/f/x_froff.rc"
check "N10 the preset refused with routing turned off explicitly" RUNS
fresh; echo 'hopVarianceTerms=true  # final plan item 1' >> "$T/f/main/spec_profile.txt"
check "N11 the preset still setting --hopVarianceTerms" NOHV

printf '  G24 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
