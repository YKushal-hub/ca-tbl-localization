#!/usr/bin/env bash
# G179 negative fixtures: doctored run directories must make the checker FAIL.
set -u
cd "$(dirname "$0")/.." || exit 2
D=$(mktemp -d "${TMPDIR:-/tmp}/g179s.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT
bash scripts/collapse_main_gate.sh "$D/good" > "$D/good.out" || { echo "selftest: good run does not pass"; cat "$D/good.out"; exit 1; }
bad=0
check() { if python3 scripts/collapse_main_gate.py "$D/$1" > /dev/null; then echo "selftest: $1 NOT caught"; bad=1; else echo "selftest: $1 caught"; fi; }
cp -r "$D/good" "$D/missing"; sed -i '$d' "$D/missing/n20/subject_summary.csv"; check missing
cp -r "$D/good" "$D/moved"; f="$D/moved/n20/subjtrace_ca_tbl_full_seed5_subj7.csv"; awk -F, -v OFS=, 'NR==3{$7=$7+1}1' "$f" > "$f.x" && mv "$f.x" "$f"; check moved
cp -r "$D/good" "$D/shifted"; f="$D/shifted/n1/trace_ca_tbl_full_seed5.csv"; awk -F, -v OFS=, 'NR==2{$8=$8+0.5}1' "$f" > "$f.x" && mv "$f.x" "$f"; check shifted
cp -r "$D/good" "$D/norefuse"; echo 0 > "$D/norefuse/r_depth.rc"; check norefuse
[ $bad -eq 0 ] && echo "G179 selftest PASS" || { echo "G179 selftest FAIL"; exit 1; }
