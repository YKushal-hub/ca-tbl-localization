#!/usr/bin/env bash
# G131 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/nlos_r_gate_selftest.sh [<dir written by nlos_r_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/nlos_r_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g131self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/nlos_r_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G131 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <label that must FAIL, or ->
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G131 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G131 ok    %-64s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G131 FAIL  %-64s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G131 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/none"; cp -r "$REAL/base" "$T/f/none"; check "N1 an oracle-keyed run offered as 'none'" 1 NONE
fresh; f=$(cd "$T/f/oper" && find . -type f ! -name 'run_id.txt' | sort | head -1); echo x >> "$T/f/oper/$f"
check "N2 the default given explicitly changes an output ($f)" 1 DEFAULT
fresh; cp "$REAL/base/trace_ca_tbl_full_seed7.csv" "$T/f/det/"; check "N3 oracle-keyed counts offered as the detector run's" 1 DET
fresh; echo 134 > "$T/f/det.rc"; check "N4 a run did not exit 0" 1 RUNS
printf '  G131 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
