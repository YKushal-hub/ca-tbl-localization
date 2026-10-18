#!/usr/bin/env bash
# G140 negative fixtures (rule 6). usage: bash scripts/linkleak_gate_selftest.sh <dir written by linkleak_gate.sh>
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/linkleak_gate.py"
REAL=${1:?usage}
T=$(mktemp -d "${TMPDIR:-/tmp}/g140self.XXXXXX") || exit 2; trap 'rm -rf "$T"' EXIT
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() { python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G140 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then printf '  G140 ok    %-56s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else printf '  G140 FAIL  %-56s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi; }
fresh; check "P0 the real run set" 0 -
fresh; cp "$REAL/leak1/probe_range_sweep.csv" "$T/f/fix1/probe_range_sweep.csv"; check "N1 the leaking run offered as fixed" 1 NOLEAK
fresh; cp "$REAL/base/probe_range_sweep.csv" "$T/f/leak1/probe_range_sweep.csv"; check "N2 no leak to begin with" 1 LEAK
fresh; rm "$T/f/fix0/probe_terms.csv"; check "N3 terms file missing" 1 TERMS
fresh; sed -i 's/probe_terms.csv/probe_termz.csv/' "$T/f/fix0/run_id.txt"; check "N4 terms file not digested" 1 TERMS
fresh; cp "$REAL/fix0/probe_terms.csv" "$T/f/base/"; check "N5 a terms file written by default" 1 DEFAULT
fresh; cp "$REAL/fix0/probe_range_sweep.csv" "$T/f/offx/probe_range_sweep.csv"; check "N6 explicit false changes the probe" 1 DEFAULT
fresh; echo 1 > "$T/f/fix1.rc"; check "N7 a probe did not exit 0" 1 RUNS
printf '  G140 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
