#!/usr/bin/env bash
# G93 fixture (A7 / D-5) -- scripts/sensitivity_fidelity_check.py, every direction, on synthetic probe
# output so each verdict is known in advance. The real probe output is scored by the gate;
# this proves the scorer can tell the cases apart.
#
#   correct      tier 2 at the 812.5 kHz row (-112 @ SF7), tier 1 at -124      -> pass
#   d1_era       tier 2 reads the SF5 cell for SF7 (-100), the defect 0d5e747 fixed -> fail, tier2
#   bw_row       tier 2 at the 1625 kHz row (-106) while configured at 800 kHz -> fail, tier2,
#                and the diagnosis names the 1625 row
#   tier1_off    tier 1 5 dB off (-119)                                         -> fail, tier1
#   no_scenario  scenario file without the phy lines                            -> exit 2
#   sy3_shift    --bw2p4 1625000: main's SY-3 shift of the 1625 row (-102.9)    -> fail, tier2
#   sy3_row      --bw2p4 1625000 with the data sheet's 1625 kHz row (-106)      -> pass

set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2
CHECK=scripts/sensitivity_fidelity_check.py
TMP=$(mktemp -d "${TMPDIR:-/tmp}/catbl_g93.XXXXXX"); trap 'rm -rf "$TMP"' EXIT
RC=0

printf '  phy[CA_TBL_TIER_868] = {7, 125e3, 14.0};\n  phy[CA_TBL_TIER_2P4] = {7, 800e3, 12.5};\n' > "$TMP/scen.cc"
printf 'int main() { return 0; }\n' > "$TMP/scen_bad.cc"

probe() {  # probe <file> <tier1-SF7> <tier2-SF7>
    {
        echo "tier,sf,crDenom,configuredCr,thresholdDbm"
        for sf in 5 6 7 8 9 10 11 12; do
            t1=-130; t2=-110
            [ "$sf" -eq 7 ] && { t1=$2; t2=$3; }
            echo "868,$sf,5,5,$t1"; echo "2p4,$sf,5,5,$t2"; echo "uwb,$sf,5,5,-104"
        done
    } > "$1"
}
probe "$TMP/correct.csv"   -124 -112
probe "$TMP/d1_era.csv"    -124 -100
probe "$TMP/bw_row.csv"    -123 -106
probe "$TMP/tier1_off.csv" -119 -112
probe "$TMP/sy3_shift.csv" -124 -102.9
probe "$TMP/sy3_row.csv"   -124 -106

expect() {  # expect <label> <want-exit> <needle-or-empty> <probe> <scenario> [checker args]
    out=$(python3 "$CHECK" "$4" "$5" "${@:6}" 2>&1); got=$?
    if [ "$got" -ne "$2" ]; then
        printf '  BAD  %-12s expected exit %s, got %s\n' "$1" "$2" "$got"; printf '%s\n' "$out" | sed 's/^/       /'; RC=1; return
    fi
    if [ -n "$3" ] && ! printf '%s\n' "$out" | grep -q -- "$3"; then
        printf '  BAD  %-12s right exit, output lacks: %s\n' "$1" "$3"; RC=1; return
    fi
    printf '  OK   %-12s exit %s%s\n' "$1" "$got" "${3:+, names: $3}"
}

printf 'G93 fixture -- seven directions\n\n'
expect correct     0 "G93 PASSED"                 "$TMP/correct.csv"   "$TMP/scen.cc"
expect d1_era      1 "G93 FAILED: tier2"          "$TMP/d1_era.csv"    "$TMP/scen.cc"
expect bw_row      1 "data sheet's 1625 kHz row" "$TMP/bw_row.csv"    "$TMP/scen.cc"
expect tier1_off   1 "G93 FAILED: tier1"          "$TMP/tier1_off.csv" "$TMP/scen.cc"
expect no_scenario 2 "could not parse"           "$TMP/correct.csv"   "$TMP/scen_bad.cc"
expect sy3_shift   1 "G93 FAILED: tier2"         "$TMP/sy3_shift.csv" "$TMP/scen.cc" --bw2p4 1625000
expect sy3_row     0 "G93 PASSED"                "$TMP/sy3_row.csv"   "$TMP/scen.cc" --bw2p4 1625000

if [ "$RC" -eq 0 ]; then printf '\nG93 fixture: all directions behaved\n'; else printf '\nG93 fixture: FAILED\n'; fi
exit "$RC"
