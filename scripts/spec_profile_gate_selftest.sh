#!/usr/bin/env bash
# G24 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/spec_profile_gate_selftest.sh [<dir written by spec_profile_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# G24 FAILS on the real run set today: five mechanisms have a zero population under the
# preset (TECH_DEBT N.15). So P0 asserts that those five are the ONLY failures, and each
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
KNOWN='POP (triggered updates|fixes at hop depth 2|per-hop variance terms|grant tie-break verdicts|UWB-delivered fixes)'

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
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
fresh; sed -i -E 's/solves OK [0-9]+/solves OK 0/' "$T/f/main.log"
check "N3 a mechanism with no population (TDoA)" "POP tier-1 TDoA solves"
fresh; sed -i -E '/TABLE CI:/d' "$T/f/main.log"
check "N4 a mechanism whose evidence line is missing" "POP CI table fuses"
fresh; sed -i -E 's/a variance term on [0-9]+/a variance term on 5/' "$T/f/off_hopVarianceTerms.log"; printf '  NW-1 hop variance terms (ca_tbl_full): legs 5; a variance term on 5 (mean sigma_var 1 m); NLOS-scored 0, inflated 0 (x3 at score >= 0.5); mean sigma raw 1 m -> leg 1 m\n' >> "$T/f/off_hopVarianceTerms.log"
check "N5 a mechanism still firing with its flag off" "EFF hopVarianceTerms"
fresh; echo 0 > "$T/f/r_topo.rc"
check "N6 --specProfile accepted with --topology=1" R
fresh; echo 134 > "$T/f/gasp.rc"
check "N7 a run did not exit 0" RUNS
fresh; echo 0 > "$T/f/r_n12.rc"; sed -i '/needs --fixRoutingSubjects/d' "$T/f/r_n12.log"
check "N8 the preset accepted with --fixRoutingSubjects=false (N.12)" R12
fresh; sed -i -E '0,/NW-2 weights alpha 0 beta 0 /s//NW-2 weights alpha 1 beta 1 /' "$T/f/main.log"
check "N9 a router not running the preset's weights (0,0,1,1)" WEIGHTS
fresh; echo 1 > "$T/f/x_froff.rc"
check "N10 the preset refused with routing turned off explicitly" RUNS

printf '  G24 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
