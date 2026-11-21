#!/usr/bin/env bash
# G177 negative fixtures: each doctored input must make scripts/rubble_gate.py FAIL.
set -u
cd "$(dirname "$0")/.." || exit 2
D=$(mktemp -d "${TMPDIR:-/tmp}/g177s.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT
build/scratch/ns3.47-ca_tbl_rubble_selftest-optimized > "$D/good" || exit 2
python3 scripts/rubble_gate.py "$D/good" > /dev/null || { echo "selftest: good input does not pass"; exit 1; }
bad=0
expect_fail() { # name sed-expression
    sed "$2" "$D/good" > "$D/$1"
    if python3 scripts/rubble_gate.py "$D/$1" > /dev/null; then echo "selftest: $1 NOT caught"; bad=1; else echo "selftest: $1 caught"; fi
}
expect_fail itu_drift      's/^itu_2400=.*/itu_2400=65.600000/'
expect_fail bracket_tuned  's/^A_central_868=.*/A_central_868=11.000000/'
expect_fail eta_drift      's/^eta_high=.*/eta_high=0.800000/'
expect_fail bias_drift     's/^bias_central=.*/bias_central=1.320000/'
expect_fail uwb_drift      's/^A_high_8000=.*/A_high_8000=200.000000/'
expect_fail oes_anchor     's/^A_low_2000=.*/A_low_2000=20.500000/'
expect_fail nist_span      's/^A_high_868=.*/A_high_868=15.000000/'
expect_fail geo_void       's/^geo_vert_void=.*/geo_vert_void=4.000000/'
expect_fail neg_overlap    's/^neg_overlap=.*/neg_overlap=1.000000/'
expect_fail neg_void       's/^neg_void_outside=.*/neg_void_outside=1.000000/'
# a doctored source log: the row number no longer matches the code
cp docs/SOURCES_COLLAPSE.md "$D/log.bak"
sed -i 's/| SLOW-BR | ranging bias per metre of rubble \*\*0.454/| SLOW-BR | ranging bias per metre of rubble **0.999/' docs/SOURCES_COLLAPSE.md
if python3 scripts/rubble_gate.py "$D/good" > /dev/null; then echo "selftest: row_edit NOT caught"; bad=1; else echo "selftest: row_edit caught"; fi
cp "$D/log.bak" docs/SOURCES_COLLAPSE.md
# a system-side file that includes the World header (leak)
L=src/lora-physics-engine/model/ca-tbl-zz-leak-fixture.h
echo '#include "ns3/ca-tbl-rubble-world.h"' > "$L"
if python3 scripts/rubble_gate.py "$D/good" > /dev/null; then echo "selftest: leak NOT caught"; bad=1; else echo "selftest: leak caught"; fi
rm -f "$L"
git diff --quiet -- docs/SOURCES_COLLAPSE.md || { echo "selftest: log not restored"; exit 1; }
[ $bad -eq 0 ] && echo "G177 selftest PASS" || { echo "G177 selftest FAIL"; exit 1; }
