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
#   uwb_6m8      --uwbMode 6m8, tier 3 at -93.2                                   -> pass, tiers 1-3
#   uwb_850k     --uwbMode 850k, tier 3 at -99.3                                  -> pass, tiers 1-3
#   uwb_legacy   --uwbMode 850k, tier 3 at the code default's -104                -> fail, tier3
#   uwb_swapped  --uwbMode 6m8, tier 3 at the 850k row (-99.3)                    -> fail, tier3, names 850k
#   uwb_nodata   --uwbMode 6m8, tier 3 at the no-data row (-98.0)                 -> fail, tier3
#   uwb_one_sf   --uwbMode 850k, -99.3 except SF12 at -104                        -> fail, tier3
#   uwb_badmode  --uwbMode 2m                                                     -> exit 2

set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2
CHECK=scripts/sensitivity_fidelity_check.py
TMP=$(mktemp -d "${TMPDIR:-/tmp}/catbl_g93.XXXXXX"); trap 'rm -rf "$TMP"' EXIT
RC=0

printf '  phy[CA_TBL_TIER_868] = {7, 125e3, 14.0};\n  phy[CA_TBL_TIER_2P4] = {7, 800e3, 12.5};\n' > "$TMP/scen.cc"
printf 'int main() { return 0; }\n' > "$TMP/scen_bad.cc"

probe() {  # probe <file> <tier1-SF7> <tier2-SF7> [<tier3> [<tier3-SF12>]]
    {
        echo "tier,sf,crDenom,configuredCr,thresholdDbm"
        for sf in 5 6 7 8 9 10 11 12; do
            t1=-130; t2=-110; t3=${4:--104}
            [ "$sf" -eq 7 ] && { t1=$2; t2=$3; }
            [ "$sf" -eq 12 ] && [ -n "${5:-}" ] && t3=$5
            echo "868,$sf,5,5,$t1"; echo "2p4,$sf,5,5,$t2"; echo "uwb,$sf,5,5,$t3"
        done
    } > "$1"
}
probe "$TMP/correct.csv"   -124 -112
probe "$TMP/d1_era.csv"    -124 -100
probe "$TMP/bw_row.csv"    -123 -106
probe "$TMP/tier1_off.csv" -119 -112
probe "$TMP/sy3_shift.csv" -124 -102.9
probe "$TMP/sy3_row.csv"   -124 -106
probe "$TMP/uwb_6m8.csv"   -124 -112 -93.2
probe "$TMP/uwb_850k.csv"  -124 -112 -99.3
probe "$TMP/uwb_nodata.csv" -124 -112 -98.0
probe "$TMP/uwb_one_sf.csv" -124 -112 -99.3 -104

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

printf 'G93 fixture -- fourteen directions\n\n'
expect correct     0 "G93 PASSED"                 "$TMP/correct.csv"   "$TMP/scen.cc"
expect d1_era      1 "G93 FAILED: tier2"          "$TMP/d1_era.csv"    "$TMP/scen.cc"
expect bw_row      1 "data sheet's 1625 kHz row" "$TMP/bw_row.csv"    "$TMP/scen.cc"
expect tier1_off   1 "G93 FAILED: tier1"          "$TMP/tier1_off.csv" "$TMP/scen.cc"
expect no_scenario 2 "could not parse"           "$TMP/correct.csv"   "$TMP/scen_bad.cc"
expect sy3_shift   1 "G93 FAILED: tier2"         "$TMP/sy3_shift.csv" "$TMP/scen.cc" --bw2p4 1625000
expect sy3_row     0 "G93 PASSED"                "$TMP/sy3_row.csv"   "$TMP/scen.cc" --bw2p4 1625000
expect uwb_6m8     0 "G93 PASSED (tiers 1-3)"    "$TMP/uwb_6m8.csv"   "$TMP/scen.cc" --uwbMode 6m8
expect uwb_850k    0 "G93 PASSED (tiers 1-3)"    "$TMP/uwb_850k.csv"  "$TMP/scen.cc" --uwbMode 850k
expect uwb_legacy  1 "G93 FAILED: tier3"         "$TMP/correct.csv"   "$TMP/scen.cc" --uwbMode 850k
expect uwb_swapped 1 "data sheet's 850k row"     "$TMP/uwb_850k.csv"  "$TMP/scen.cc" --uwbMode 6m8
expect uwb_nodata  1 "G93 FAILED: tier3"         "$TMP/uwb_nodata.csv" "$TMP/scen.cc" --uwbMode 6m8
expect uwb_one_sf  1 "G93 FAILED: tier3"         "$TMP/uwb_one_sf.csv" "$TMP/scen.cc" --uwbMode 850k
expect uwb_badmode 2 "must be one of"            "$TMP/uwb_850k.csv"  "$TMP/scen.cc" --uwbMode 2m

if [ "$RC" -eq 0 ]; then printf '\nG93 fixture: all directions behaved\n'; else printf '\nG93 fixture: FAILED\n'; fi
exit "$RC"
