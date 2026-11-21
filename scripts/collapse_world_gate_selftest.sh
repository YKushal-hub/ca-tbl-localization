#!/usr/bin/env bash
# G178 negative fixtures: doctored run directories and a planted leak must make the checker FAIL.
set -u
cd "$(dirname "$0")/.." || exit 2
D=$(mktemp -d "${TMPDIR:-/tmp}/g178s.XXXXXX") || exit 2
trap 'rm -rf "$D"; rm -f scripts/collapse/zz_leak_fixture.py' EXIT
bash scripts/collapse_world_gate.sh "$D/good" > "$D/good.out" || { echo "selftest: good run does not pass"; cat "$D/good.out"; exit 1; }
bad=0
check() { # name
    if python3 scripts/collapse_world_gate.py "$D/$1" > /dev/null; then echo "selftest: $1 NOT caught"; bad=1; else echo "selftest: $1 caught"; fi
}
cp -r "$D/good" "$D/nondet"; sed -i '3s/,1,/,0,/' "$D/nondet/det2/obs_links.csv"; check nondet
cp -r "$D/good" "$D/unkeyed"; sed -i '5s/$/9/' "$D/unkeyed/keyed/obs_links.csv"; check unkeyed
cp -r "$D/good" "$D/norefuse"; echo 0 > "$D/norefuse/r_depth.rc"; check norefuse
cp -r "$D/good" "$D/depth"; sed -i '2s/^\([0-9]*,[^,]*,[^,]*,[^,]*\),[^,]*,/\1,9.9,/' "$D/depth/det1/truth_victims.csv"; check depth
echo 'open("x/truth_victims.csv")' > scripts/collapse/zz_leak_fixture.py
check good; rm -f scripts/collapse/zz_leak_fixture.py
[ $bad -eq 0 ] && echo "G178 selftest PASS" || { echo "G178 selftest FAIL"; exit 1; }
