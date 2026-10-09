#!/usr/bin/env bash
# G101 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/ie_gate_selftest.sh [<dir written by ie_gate.sh>]
#   Without a directory it makes its own real run set first.
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. N2 is the plan's Jacobian test: the reference rebuilt with one
# partial's sign flipped must not agree with the C++ filter.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/ie_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g101self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/ie_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/r_noimu.rc" ] || { echo "  G101 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G101 FAIL +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G101 ok    %-66s exit=%d\n' "$1" "$rc"
        pass=$((pass + 1))
    else
        printf '  G101 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G101 (FAIL|MISSING)" "$T/out.txt" | head -4
        fail=$((fail + 1))
    fi
}

fresh
check "P0  the real run set passes" 0 -

fresh
python3 - "$T/f/ie_cpp.txt" <<'PY'
import sys
p = sys.argv[1]; L = open(p).read().split("\n")
k, v = L[6].split(); L[6] = f"{k} {float(v) * (1 + 1e-7):.12e}"
open(p, "w").write("\n".join(L))
PY
check "N1  the C++ filter's pxy off by 1e-7 relative" 1 UNIT

fresh
mkdir -p "$T/ref"
cp "$ROOT/analysis/accuracy_replay/replay.py" "$T/ref/"
sed 's/F\[0, 2\], F\[1, 2\] = (1 - s) \* ay, -(1 - s) \* ax/F[0, 2], F[1, 2] = -(1 - s) * ay, -(1 - s) * ax/' \
    "$ROOT/analysis/accuracy_replay/make_ie_vectors.py" > "$T/ref/make_ie_vectors.py"
grep -q 'F\[0, 2\], F\[1, 2\] = -(1 - s)' "$T/ref/make_ie_vectors.py" || echo "  G101 selftest: the sign flip did not apply"
python3 -I "$T/ref/make_ie_vectors.py" > "$T/f/ie_py.txt"
check "N2  the Jacobian's d x'/d psi sign flipped in the reference" 1 UNIT

fresh
echo 1 > "$T/f/ie_cpp.rc"
check "N3  the C++ test did not run" 1 UNIT

fresh
echo 1 > "$T/f/on.rc"
check "N4  the flagged run exited non-zero" 1 RC

fresh
python3 - "$T/f/off/trace_static_2p4_seed1.csv" <<'PY'
import sys
p = sys.argv[1]; L = open(p).read().split("\n"); L[5] = L[5].replace(",", ", ", 1); open(p, "w").write("\n".join(L))
PY
check "N5  the flag at false differs from absent" 1 NULL

fresh
cp "$T/f/base/trace_static_2p4_seed1.csv" "$T/f/on/trace_static_2p4_seed1.csv"
check "N6  one arm's tag 0 did not move under the flag" 1 REACH

fresh
f=$(ls "$T/f/base"/subjtrace_ca_tbl_full_seed1_subj*.csv | head -1); cp "$f" "$T/f/on/"
check "N7  one subject's filter did not move under the flag" 1 REACH

fresh
echo 0 > "$T/f/r_noimu.rc"
check "N8  the refusal without --imuPredict exited 0" 1 REFUSE

printf '  G101 selftest: %d ok, %d FAIL\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
