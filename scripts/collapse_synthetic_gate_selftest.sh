#!/usr/bin/env bash
# G182 negative fixtures: injected faults must make scripts/collapse/synthetic_gate.py FAIL.
set -u
cd "$(dirname "$0")/.." || exit 2
python3 scripts/collapse/synthetic_gate.py > /dev/null || { echo "selftest: clean gate does not pass"; exit 1; }
bad=0
for f in rho norank; do
    if G182_FAULT=$f python3 scripts/collapse/synthetic_gate.py > /dev/null; then echo "selftest: $f NOT caught"; bad=1; else echo "selftest: $f caught"; fi
done
[ $bad -eq 0 ] && echo "G182 selftest PASS" || { echo "G182 selftest FAIL"; exit 1; }
