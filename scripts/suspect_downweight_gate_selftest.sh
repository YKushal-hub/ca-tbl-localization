#!/usr/bin/env bash
# G29 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/suspect_downweight_gate_selftest.sh [<dir written by suspect_downweight_gate.sh>]
#   Without a directory it makes its own real run set first (about 20 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N0   the default's run offered as the downweight run -- the tree before DC-7, or a
#        later one where downweight silently rejects;
#   N5   the R x 4 run offered as the exact control, which must be at R x 1;
#   N8   the PCS factor not applied;
#   N11  the inflate factor not applied to R.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/suspect_downweight_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g29self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/suspect_downweight_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/dw/arm_summary.csv" ] || { echo "  G29 selftest: no real run set in $REAL"; exit 2; }
TR=trace_ca_tbl_full_seed7.csv

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G29 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G29 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G29 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /'
        pass=$((pass + 1))
    else
        printf '  G29 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G29 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# rowmap <csv> <python predicate over r> <python dict of column: expression over r>
# rewrites the columns of the FIRST row satisfying the predicate (or every such row, if
# the 4th argument is "all")
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
for r in rows:
    if eval(pred):
        for k, v in eval(upd).items():
            r[k] = v
        hit += 1
        if not every:
            break
assert hit, "fixture: no row matched " + pred
with open(p, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=fields, lineterminator="\n")
    w.writeheader()
    w.writerows(rows)
PY
}
swap() { rm -rf "$T/f/$2"; cp -r "$T/f/$1" "$T/f/$2"; cp "$T/f/$1.log" "$T/f/$2.log"; }

fresh
check "P0 the real run set" 0 -

fresh; swap none dw
check "N0 the default's run offered as downweight (the tree before DC-7)" 1 POP
fresh; rowmap "$T/f/dw/$TR" "r['suspect'] == '1'" "{'rejected': '1'}"
check "N1 one downweighted suspect fix rejected after all" 1 POP
fresh; rowmap "$T/f/none/$TR" "r['suspect'] == '1'" "{'rejected': '0'}"
check "N2 the control: a suspect fix NOT rejected by default" 1 POP
fresh; sed -i 's/DC-7 downweighted suspect fixes: \([0-9]*\) of/DC-7 downweighted suspect fixes: 1 of/' "$T/f/dw.log"
check "N3 fewer fixes downweighted than flagged" 1 COUNT
fresh; echo "    DC-7 downweighted suspect fixes: 0 of 131 suspects" >> "$T/f/none.log"
check "N4 the count line printed without the flag" 1 COUNT
fresh; swap dw dw11
check "N5 the inflated run offered as the exact control" 1 EXACT
fresh; rowmap "$T/f/dw11/$TR" "r['suspect'] == '0' and r['rejected'] == '0'" "{'rejected': '1'}"
check "N6 a clean cycle rejected under downweight" 1 EXACT
fresh; rowmap "$T/f/dw11/$TR" "r['suspect'] == '1'" "{'estX': '%.4f' % (float(r['estX']) + 1.0)}"
check "N7 the exact control's estimate differs on a suspect cycle" 1 EXACT
fresh; swap dw11 dw1h
check "N8 the PCS factor not applied" 1 PCSF
fresh; rowmap "$T/f/dw1h/$TR" "r['suspect'] == '0' and float(r['pcs']) > 0.1" "{'pcs': '%.4f' % (0.5 * float(r['pcs']))}"
check "N9 PCS downweighted on a clean cycle" 1 PCSF
fresh; cp "$T/f/dw11/$TR" "$T/f/dw1h/$TR"
rowmap "$T/f/dw1h/$TR" "r['suspect'] == '1'" "{'pcs': '%.4f' % (0.75 * float(r['pcs']))}" all
check "N10 PCS x 0.75 where 0.5 was asked" 1 PCSF
fresh; swap dw11 dw4_1
check "N11 the inflate factor not applied to R" 1 RINF
fresh; echo 0 > "$T/f/r_nodw.rc"
check "N12 --suspectPcsFactor accepted without downweight" 1 R
fresh; echo "FATAL: something else" > "$T/f/r_mode.log"
check "N13 a refusal for another reason" 1 R
fresh; echo 134 > "$T/f/b1.rc"
check "N14 a run the checks read did not exit 0" 1 RUNS
fresh; rm "$T/f/dw1h/$TR"
check "N15 an input missing" 2 PCSF

printf '  G29 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
