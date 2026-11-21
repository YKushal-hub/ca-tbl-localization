#!/usr/bin/env bash
# G180 negative fixtures.
set -u
cd "$(dirname "$0")/.." || exit 2
D=$(mktemp -d "${TMPDIR:-/tmp}/g180s.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT
bash scripts/frame_key_gate.sh "$D/good" > "$D/good.out" || { echo "selftest: good run does not pass"; cat "$D/good.out"; exit 1; }
bad=0
check() { if python3 scripts/frame_key_gate.py "$D/$1" > /dev/null; then echo "selftest: $1 NOT caught"; bad=1; else echo "selftest: $1 caught"; fi; }
cp -r "$D/good" "$D/starve"; for f in "$D"/starve/on/subjtrace_*; do awk -F, -v OFS=, 'NR>1{$14=0}1' "$f" > "$f.x" && mv "$f.x" "$f"; done; check starve
cp -r "$D/good" "$D/nodefect"; for f in "$D"/nodefect/off/subjtrace_*; do awk -F, -v OFS=, 'NR>1{$14=6}1' "$f" > "$f.x" && mv "$f.x" "$f"; done; check nodefect
cp -r "$D/good" "$D/crash"; echo 1 > "$D/crash/on.rc"; check crash
[ $bad -eq 0 ] && echo "G180 selftest PASS" || { echo "G180 selftest FAIL"; exit 1; }
