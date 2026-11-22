#!/usr/bin/env bash
# G181 negative fixtures.
set -u
cd "$(dirname "$0")/.." || exit 2
D=$(mktemp -d "${TMPDIR:-/tmp}/g181s.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT
bash scripts/collapse_mech_gate.sh "$D/good" > "$D/good.out" || { echo "selftest: good run does not pass"; cat "$D/good.out"; exit 1; }
bad=0
check() { if python3 scripts/collapse_mech_gate.py "$D/$1" > /dev/null; then echo "selftest: $1 NOT caught"; bad=1; else echo "selftest: $1 caught"; fi; }
cp -r "$D/good" "$D/nogate"; cp "$D/nogate/g_off/arm_summary.csv" "$D/nogate/g_on/arm_summary.csv"; check nogate
cp -r "$D/good" "$D/baddepth"; f="$D/baddepth/d/collapse_depth_ca_tbl_full_seed902.csv"; awk -F, -v OFS=, 'NR==2{$2=42;$3=0.1}1' "$f" > "$f.x" && mv "$f.x" "$f"; check baddepth
cp -r "$D/good" "$D/noframe"; cp "$D/noframe/f_off/subject_summary.csv" "$D/noframe/f_on/subject_summary.csv"; check noframe
[ $bad -eq 0 ] && echo "G181 selftest PASS" || { echo "G181 selftest FAIL"; exit 1; }
