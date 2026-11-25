#!/usr/bin/env bash
# G188 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
GATE="$(pwd)/scripts/finder_duty_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g188self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash scripts/finder_duty_gate.sh "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/duty" ] || { echo "  G188 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G188 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G188 ok    %-58s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G188 FAIL  %-58s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/duty.rc"; check "N1 a run crashed" 1 RUNS
fresh; echo x >> "$T/f/false/trace_ca_tbl_full_seed731.csv"; check "N2 an explicit default changes an output" 1 OFFSAME
fresh; echo x >> "$T/f/report/arm_summary.csv"; check "N3 the report changes an output" 1 REPORT
fresh; sed -i '/DD60 finder 503/d' "$T/f/report.log"; check "N4 a searcher missing from the report" 1 REPORT
fresh; sed -i 's/audited peak [0-9.]*, replies/audited peak 0.02, replies/' "$T/f/report.log"; check "N5 no breach to remove" 1 OVER
fresh; sed -i '0,/audited peak [0-9.]*, replies/s//audited peak 0.0251, replies/' "$T/f/duty.log"; check "N6 one searcher over the cap" 1 LEGAL
fresh; sed -i '/DD60 finder 502/d' "$T/f/duty.log"; check "N7 a searcher missing when enforced" 1 LEGAL
fresh; sed -i 's/audited peak [0-9.]*, replies/audited peak 0.0, replies/' "$T/f/duty.log"; check "N8 an audit that measured nothing" 1 LEGAL
fresh; sed -i 's/(all finders) [0-9]*/(all finders) 0/' "$T/f/duty.log"; check "N9 nothing refused" 1 REFUSED
fresh; sed -i '/DD60 sharedDuty868Finders/d' "$T/f/duty.log"; check "N10 not announced" 1 LINES
echo "  G188 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
