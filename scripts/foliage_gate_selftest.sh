#!/usr/bin/env bash
# G134 negative fixtures (rule 6). usage: bash scripts/foliage_gate_selftest.sh [<dir written by foliage_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/foliage_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g134self.XXXXXX") || exit 2; trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/foliage_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/pbase.rc" ] || { echo "  G134 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() { python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G134 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then printf '  G134 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else printf '  G134 FAIL  %-60s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G134 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1)); fi; }
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/pon"; cp -r "$REAL/pbase" "$T/f/pon"; check "N1 a probe without the clamp offered as the clamped probe" 1 REACH
fresh; rm -rf "$T/f/son" "$T/f/son.log"; cp -r "$REAL/sbase" "$T/f/son"; cp "$REAL/sbase.log" "$T/f/son.log"; check "N2 a sim without the clamp offered as the clamped sim" 1 FIRES
fresh; f=$(cd "$T/f/soff" && find . -type f ! -name 'run_id.txt' | sort | head -1); echo x >> "$T/f/soff/$f"; check "N3 the default given explicitly changes an output" 1 DEFAULT
fresh; echo 134 > "$T/f/pon.rc"; check "N4 a run did not exit 0" 1 RUNS
fresh; python3 - "$T/f/pon/probe_range_sweep.csv" <<'PY'
import sys
p = sys.argv[1]; L = open(p).read().splitlines()
for i, l in enumerate(L):
    if l.startswith("868MHz,") and float(l.split(",")[1]) >= 500:
        f = l.split(","); f[2] = "%.4f" % max(0.0, float(f[2]) - 0.2); L[i] = ",".join(f); break
open(p, "w").write("\n".join(L) + "\n")
PY
check "N5 a long 868 MHz probe row moved under the clamp" 1 REACH
printf '  G134 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
