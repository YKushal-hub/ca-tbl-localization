#!/usr/bin/env bash
# G47 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/finder_tasking_gate_selftest.sh [<dir written by finder_tasking_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N3: a finder steering on tag
# 0's entry while tasked to another subject -- the behaviour before the flag.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/finder_tasking_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g47self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/finder_tasking_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/rr2/tasking_ca_tbl_full_seed7.csv" ] || { echo "  G47 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G47 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G47 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G47 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G47 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G47 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv> <python body over rows (list of dicts)>
edit() {
    python3 - "$@" <<'PY'
import csv, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}
T2="$T/f/rr2/tasking_ca_tbl_full_seed7.csv"
T5="$T/f/rr5/tasking_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$T/f/tag0/trace_ca_tbl_full_seed7.csv" 'rows[5]["estX"] = "1.0"'
check "N0 --finderTasking=tag0 moves an output" 1 NULL
fresh; edit "$T5" 'r = next(r for r in rows if r["finderId"] == "502"); r["targetId"] = "1"'
check "N1 a finder logged against the wrong subject" 1 TARGET
# N3: finder 502 at rr2 (tasked to subject 2) steering on tag 0's estimate, as before the flag
fresh; python3 - "$T/f/rr2" <<'PY'
import csv, sys
d = sys.argv[1]
tr = {round(float(r["t"]) / 2.5): r for r in csv.DictReader(open(d + "/trace_ca_tbl_full_seed7.csv"))}
p = d + "/tasking_ca_tbl_full_seed7.csv"
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
n = 0
for r in rows:
    if r["finderId"] == "502" and r["phase"] == "converge":
        k = round(float(r["entryT"]) / 2.5) - 1
        if k in tr:
            r["entryX"], r["entryY"] = tr[k]["estX"], tr[k]["estY"]
            r["goalX"], r["goalY"] = tr[k]["estX"], tr[k]["estY"]
            n += 1
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
print("rows moved", n, file=sys.stderr)
PY
check "N3 a finder tasked to subject 2 steering on tag 0's entry" 1 ENTRY
fresh; edit "$T2" '
for r in rows:
    if r["finderId"] == "503" and r["phase"] == "converge":
        r["goalX"] = r["entryX"]; r["goalY"] = r["entryY"]'
check "N4 a pair of finders stacked on one point (no spread within the group)" 1 GOAL
fresh; edit "$T5" '
for r in rows:
    if r["phase"] == "converge":
        r["goalX"] = "%.4f" % (float(r["goalX"]) + 5.0)
        break'
check "N5 a finder spread although it is alone on its subject" 1 GOAL
fresh; edit "$T2" 'del rows[10]'
check "N6 a finder-cycle missing from the log" 1 COUNTS
fresh; sed -i '0,/converge [0-9]*/s//converge 99999/' "$T/f/rr2.log"
check "N7 an MS-1 tasking line that disagrees with the log" 1 COUNTS
fresh; edit "$T/f/sweep/tasking_ca_tbl_full_seed7.csv" '
for r in rows:
    if r["phase"] == "sweep":
        r["phase"] = "blind"'
sed -i 's/sweep [0-9]* converge/sweep 0 converge/g' "$T/f/sweep.log"
check "N8 the sweep phase never exercised" 1 COUNTS
fresh; edit "$T5" 'rows[0]["goalX"] = rows[0]["goalX"]; rows[1]["entryVarX"] = "0.5"'
check "N9 a tasking log that is not the file digested" 1 DIGEST
fresh; echo 0 > "$T/f/r_one.rc"
check "N10 roundrobin accepted with one subject" 1 R
fresh; echo 134 > "$T/f/rr2.rc"
check "N11 a run the checks read did not exit 0" 1 RUNS

printf '  G47 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
