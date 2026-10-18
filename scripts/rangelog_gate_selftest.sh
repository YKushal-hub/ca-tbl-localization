#!/usr/bin/env bash
# G137 negative fixtures (rule 6). usage: bash scripts/rangelog_gate_selftest.sh [<dir written by rangelog_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/rangelog_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g137self.XXXXXX") || exit 2; trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/rangelog_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G137 selftest: no real run set"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() { python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G137 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then printf '  G137 ok    %-56s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else printf '  G137 FAIL  %-56s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G137 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1)); fi; }
fresh; check "P0 the real run set" 0 -
fresh; rm "$T/f/on/ranges_ca_tbl_full_seed7.csv"; check "N1 the ranges file is missing" 1 LOG
fresh; echo x >> "$T/f/on/arm_summary.csv"; check "N2 the flag changes another output" 1 INERT
fresh; echo x >> "$T/f/off/arm_summary.csv"; check "N3 the explicit default changes an output" 1 DEFAULT
fresh; python3 - "$T/f/on/ranges_ca_tbl_full_seed7.csv" <<'PY'
import sys
p = sys.argv[1]; L = open(p).read().splitlines(); t = L[1].split(",")[0]
open(p, "w").write("\n".join([L[0]] + [l for l in L[1:] if l.split(",")[0] != t]) + "\n")
PY
check "N4 one cycle's ranges are dropped from the log" 1 LOG
printf '  G137 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
