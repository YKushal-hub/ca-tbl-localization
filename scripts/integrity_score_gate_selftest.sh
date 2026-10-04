#!/usr/bin/env bash
# G30 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/integrity_score_gate_selftest.sh [<dir written by integrity_score_gate.sh>]
#   Without a directory it makes its own real run set first (about 10 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N0  the control offered as the flag run -- the tree before DC-8, or a later one where
#       the flag reaches nothing;
#   N6  the 0.1 run's verdicts taken at 0.5 -- the threshold not reaching the policy;
#   N3/N4  the policy's mean or fix set disagreeing with what the scorer wrote.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/integrity_score_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g30self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/integrity_score_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/score/integrity_ca_tbl_full_seed7.csv" ] || { echo "  G30 selftest: no real run set in $REAL"; exit 2; }
INT=integrity_ca_tbl_full_seed7.csv
TR=trace_ca_tbl_full_seed7.csv
LK=nlos_links_ca_tbl_full_seed7.csv

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G30 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G30 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G30 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /'
        pass=$((pass + 1))
    else
        printf '  G30 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G30 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# rowmap <csv> <python predicate over r> <python dict of column: expression over r> [all]
rowmap() {
    python3 - "$@" <<'PY'
import csv, sys
p, pred, upd = sys.argv[1:4]
every = len(sys.argv) > 4 and sys.argv[4] == "all"
with open(p, newline="") as f:
    rd = csv.DictReader(f)
    fields = rd.fieldnames
    rows = list(rd)
hit = 0
keep = []
for r in rows:
    if (every or not hit) and eval(pred):
        hit += 1
        u = eval(upd)
        if u is None:
            continue  # None deletes the row
        r.update(u)
    keep.append(r)
assert hit, "fixture: no row matched " + pred
with open(p, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=fields, lineterminator="\n")
    w.writeheader()
    w.writerows(keep)
PY
}

fresh
check "P0 the real run set" 0 -

fresh; rm -rf "$T/f/score"; cp -r "$T/f/tag" "$T/f/score"; cp "$T/f/tag.log" "$T/f/score.log"
check "N0 the control offered as the flag run (the tree before DC-8)" 2 JOIN
fresh; cp "$T/f/score/$INT" "$T/f/tag/"
check "N1 the control writes an integrity file (the flag not default-off)" 1 OFF
fresh; rowmap "$T/f/tag/$TR" "float(r['mahalanobis']) > 0 and r['nNlos'] != '0'" "{'nNlos': '0'}"
check "N2 the control has an all-LOS fix (the premise moved)" 1 OFF
fresh; rowmap "$T/f/score/$INT" "r['basis'] == '1' and float(r['meanScore']) > 0.02" "{'meanScore': '%.9f' % (float(r['meanScore']) - 0.01)}"
check "N3 the policy's mean differs from the scorer's links" 1 JOIN
fresh; python3 - "$T/f/score" <<'PY'
import csv, sys
d = sys.argv[1]
tier = {"868MHz": "868", "2.4GHz": "2.4", "UWB": "UWB"}
ev = next(r for r in csv.DictReader(open(d + "/integrity_ca_tbl_full_seed7.csv"))
          if r["basis"] == "1" and float(r["t"]) > 100)
k = (round(float(ev["t"]), 4), tier[ev["fixTier"]])
with open(d + "/nlos_links_ca_tbl_full_seed7.csv", newline="") as f:
    rd = csv.DictReader(f)
    fields, rows = rd.fieldnames, list(rd)
i = next(i for i, r in enumerate(rows) if (round(float(r["t"]), 4), r["tier"]) == k)
del rows[i]  # one link of an evaluated fix set
with open(d + "/nlos_links_ca_tbl_full_seed7.csv", "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=fields, lineterminator="\n")
    w.writeheader()
    w.writerows(rows)
PY
check "N4 one link of an evaluated fix set missing from the scorer's file" 1 JOIN
fresh; rowmap "$T/f/score/$INT" "r['basis'] == '1' and r['eligible'] == '1' and r['suspect'] == '0'" "{'eligible': '0'}"
check "N5 an eligible fix recorded as ineligible" 1 RULE
fresh; rowmap "$T/f/score_t01/$INT" "r['basis'] == '1'" "{'eligible': '1' if float(r['meanScore']) < 0.5 else '0', 'suspect': '1' if float(r['meanScore']) < 0.5 and float(r['mahalanobis']) > 9.21 else '0'}" all
check "N6 the 0.1 run decided at 0.5 (the threshold did not arrive)" 1 RULE
fresh; rowmap "$T/f/score/$TR" "r['suspect'] == '1'" "{'suspect': '0'}"
check "N7 the trace's suspect cycles are not the file's" 1 TRACE
fresh; sed -i 's/DC-8 integrity eligibility from NLOS_score: [0-9]* eligible/DC-8 integrity eligibility from NLOS_score: 1 eligible/' "$T/f/score.log"
check "N8 the stdout count differs from the file" 1 COUNT
fresh; rowmap "$T/f/score/$INT" "r['basis'] == '1'" "{'meanScore': '0.900000000', 'eligible': '0', 'suspect': '0'}" all
check "N9 no fix eligible, no suspect (no population)" 1 POP
fresh; cp "$T/f/score/$INT" "$T/f/score/integrity_ca_tbl_no_integrity_seed7.csv"
check "N10 an arm that runs no check writes the file" 1 ARM
fresh; echo 0 > "$T/f/r_noclf.rc"
check "N11 the flag accepted without the classifier" 1 R
fresh; echo 134 > "$T/f/score_t01.rc"
check "N12 a run the checks read did not exit 0" 1 RUNS

printf '  G30 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
