#!/usr/bin/env bash
# G34 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/triggered_updates_gate_selftest.sh [<dir written by triggered_updates_gate.sh>]
#   Without a directory it makes its own real run set first (about 30 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N2  a large correction that did not move more than its threshold;
#   N4  a frame of node 1's own heard after its last gasp -- the silence not holding;
#   N6  a large correction on a radio that did not transmit that cycle (RULES.md sect 6).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/triggered_updates_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g34self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/triggered_updates_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G34 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G34 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G34 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G34 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G34 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G34 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv> <python body over rows (list of dicts) and header h>
edit() {
    python3 - "$@" <<'PY'
import csv, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p))
h = rd.fieldnames
rows = list(rd)
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n")
w.writeheader()
w.writerows(rows)
PY
}
TG="$T/f/on/triggered_ca_tbl_full_seed7.csv"
EV="$T/f/on/events_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; sed -i '2s/,/;/' "$T/f/off/arm_summary.csv"
check "N0 --triggeredUpdates=false moves an output" 1 NULL
fresh; grep "MC-2" "$T/f/on.log" >> "$T/f/def.log"
check "N1 the default prints the MC-2 line" 1 NULL
fresh; edit "$TG" '
for r in rows:
    if r["kind"] == "large_correction":
        r["threshM"] = "%.4f" % (float(r["moveM"]) + 1.0)'
check "N2 a large correction below its own threshold" 1 LARGE
fresh; edit "$TG" '
for r in rows:
    if r["kind"] == "large_correction":
        r["estX"] = "%.4f" % (float(r["estX"]) + 50.0)'
check "N3 a large correction whose estimate is not the trace's" 1 LARGE
fresh; edit "$EV" '
g = next(r for r in rows if ";reason=2" in r["payload"])
x = dict(g); x["t"] = str(float(g["t"]) + 30.0); x["msgType"] = "DISCOVERY"
x["payload"] = "pkt=1049999;hops=3"
rows.append(x)'
check "N4 a frame of node 1's own heard after its last gasp" 1 GASP
fresh; edit "$TG" '
rows[:] = [r for r in rows if r["kind"] != "last_gasp"]'
check "N5 the battery died and no last gasp was recorded" 1 GASP
fresh; edit "$TG" '
for r in rows:
    if r["kind"] == "large_correction":
        r["tier"] = "2" if r["tier"] != "2" else "0"'
check "N6 a large correction on a radio that did not transmit" 1 LARGE
fresh; edit "$EV" '
g = next(r for r in rows if ";reason=1" in r["payload"])
import re
x = dict(g); x["t"] = str(float(g["t"]) + 100.0); rows.append(x)
x["payload"] = re.sub(r"pkt=\d+", "pkt=%d" % ((1 << 20) + 99999), x["payload"])'
check "N7 a reason=1 frame with no trigger before it" 1 FRAMES
fresh; edit "$TG" '
rows.append(dict(rows[0]))'
check "N8 the CSV and the MC-2 line disagree" 1 LINE
fresh; echo 0 > "$T/f/r_k.rc"
check "N9 --triggeredUpdateK accepted without the flag" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS

printf '  G34 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
