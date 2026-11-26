#!/usr/bin/env bash
# G183 selftest: a World read planted in a system-side function (copy of the source) must make the check FAIL.
set -u
cd "$(dirname "$0")/.." || exit 2
T=$(mktemp -d); trap 'rm -rf "$T"' EXIT
python3 - "$T/sim.cc" <<'PY'
import sys
s = open("scratch/ca_tbl_causal_sim.cc").read()
k = "    const double kHold = 1.5;"
assert k in s
open(sys.argv[1], "w").write(s.replace(k, k + " const double leak = g_collapse.pileH; (void)leak;", 1))
PY
if python3 scripts/collapse_sim_leak_check.py "$T/sim.cc" > "$T/out" 2>&1; then
    echo "G183 selftest FAIL: planted leak not caught"; exit 1
fi
echo "selftest: planted leak caught"; echo "G183 selftest PASS"
