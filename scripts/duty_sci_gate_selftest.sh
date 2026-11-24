#!/usr/bin/env bash
# G185/G186 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
GATE="$(pwd)/scripts/duty_sci_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g185self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash scripts/duty_sci_gate.sh "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/duty" ] || { echo "  G185 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G18[56] FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G185 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G185 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/duty.rc"; check "N1 a run crashed" 1 RUNS
fresh; echo x >> "$T/f/false/trace_ca_tbl_full_seed722.csv"; check "N2 an explicit default changes an output" 1 OFFSAME
fresh; echo x >> "$T/f/base/arm_summary.csv"; check "N3 the report changes an output" 1 REPORT
fresh; sed -i '0,/DD56 device/{/DD56 device/d}' "$T/f/base.log"; check "N4 a device missing from the report" 1 REPORT
fresh; sed -i 's/trailing-hour peak [0-9.e+-]*/trailing-hour peak 0.02/' "$T/f/base.log"; check "N5 no breach to remove" 1 OVER
fresh; sed -i '0,/trailing-hour peak [0-9.e+-]*/s//trailing-hour peak 0.0251/' "$T/f/duty.log"; check "N6 one device over the cap in one hour" 1 LEGAL
fresh; sed -i 's/ s: [0-9.e+-]*, trailing/ s: 0.026, trailing/' "$T/f/duty.log"; check "N7 mean over the cap" 1 LEGAL
fresh; sed -i '/DD57 peerSplitCi/d' "$T/f/sci.log"; check "N8 the fusion not announced" 1 LINES
fresh; echo 0 > "$T/f/fatal.rc"; check "N9 the fusion accepted without its filter" 1 FATAL
fresh; cp "$T"/f/off/subjtrace_* "$T/f/sci/"; check "N10 coverage does not rise" 1 SCI
fresh; rm "$T"/f/sci/subjtrace_*; check "N11 no crew traces (vacuous)" 1 SCI
echo "  G185 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
