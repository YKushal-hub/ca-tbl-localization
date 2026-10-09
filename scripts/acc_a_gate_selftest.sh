#!/usr/bin/env bash
# G100 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/acc_a_gate_selftest.sh [<dir written by acc_a_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/acc_a_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g100self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/acc_a_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G100 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <label that must FAIL, or ->
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G100 (FAIL|MISSING) +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G100 ok    %-64s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G100 FAIL  %-64s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G100 (FAIL|MISSING)" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set" 0 -
fresh; cp "$REAL"/one/trace_* "$T/f/ten/"; check "N1 a run made at 1 offered as tag 0's run at 10" 1 TAG0
fresh; cp "$REAL"/one/subjtrace_* "$T/f/ten/"; check "N2 subjects that did not get the multiplier" 1 SUBJ
fresh; f=$(cd "$T/f/one" && find . -type f ! -name 'run_id.txt' ! -name 'trace_*' | head -1); echo x >> "$T/f/one/$f"
check "N3 the default given explicitly changes an output ($f)" 1 DEFAULT
fresh; python3 - "$T/f/ten" <<'PY'
import glob, sys
for p in glob.glob(sys.argv[1] + "/*trace_*.csv"):
    L = open(p).read().splitlines(); h = L[0].split(","); i = h.index("hadFix")
    out = [L[0]] + [",".join(c[:i] + ["1"] + c[i + 1:]) for c in (x.split(",") for x in L[1:])]
    open(p, "w").write("\n".join(out) + "\n")
PY
check "N4 no cycle without a fix to measure (every row a fix)" 1 TAG0
fresh; echo 134 > "$T/f/ten.rc"; check "N5 a run did not exit 0" 1 RUNS
printf '  G100 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
