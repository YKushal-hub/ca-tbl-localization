#!/usr/bin/env bash
# G113 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/unknown_start_gate_selftest.sh [<dir written by unknown_start_gate.sh>]
#   Without a directory it makes its own real run set first.
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/unknown_start_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g113self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/unknown_start_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.rc" ] || { echo "  G113 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G113 FAIL +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G113 ok    %-66s exit=%d\n' "$1" "$rc"
        pass=$((pass + 1))
    else
        printf '  G113 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G113 (FAIL|MISSING)" "$T/out.txt" | head -4
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
# every subject trace of a run, one arm or both
subj() { ls "$T/f/$1"/subjtrace_*_seed1_subj*.csv; }

fresh
check "P0  the real run set passes" 0 -

fresh
echo 1 > "$T/f/on.rc"
check "N1  a run exited non-zero" 1 RC

fresh
edit "$T/f/off/trace_static_2p4_seed1.csv" 'rows[3]["errM"] = "%.4f" % (float(rows[3]["errM"]) + 0.0001)'
check "N2  the flag at 0 differs from absent in one value" 1 NULL

fresh
for f in $(subj on); do edit "$f" 'rows[0]["errM"] = "0.0500"'; done
check "N3  at s = 100 the offset was not applied" 1 OFFSET

fresh
for f in $(subj base); do edit "$f" 'rows[0]["errM"] = "93.0000"'; done
check "N4  flags absent, an offset was applied" 1 OFFSET

fresh
f=$(subj on | head -1); edit "$f" 'rows[0]["traceSigma"] = "2500.000000"; rows[0]["nAnchors"] = "0"'
check "N5  an offset under a prior narrower than s" 1 PRIOR

fresh
f=$(ls "$T/f/on"/subjtrace_static_2p4_seed1_subj*.csv | head -1)
edit "$f" 'rows[0]["errM"] = "%.4f" % (float(rows[0]["errM"]) + 7.0); rows[0]["nAnchors"] = "0"'
g=$(echo "$f" | sed 's/static_2p4/ca_tbl_full/'); edit "$g" 'rows[0]["nAnchors"] = "0"'
check "N6  the offset depends on the arm" 1 ARMS

fresh
edit "$T/f/on/trace_ca_tbl_full_seed1.csv" 'rows[10]["trueX"] = "%.4f" % (float(rows[10]["trueX"]) + 1.0)'
check "N7  the flag moved a true trajectory (an ns-3 stream drew)" 1 STREAM

printf '  G113 selftest: %d ok, %d FAIL\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
