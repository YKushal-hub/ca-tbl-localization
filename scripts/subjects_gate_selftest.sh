#!/usr/bin/env bash
# G46 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subjects_gate_selftest.sh [<dir written by subjects_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The plan's five (MS-1 Verification) are N1-N5:
#   a dropped subject row, subject 1 != arm_summary, a charged unused tier,
#   --measuredSubjects=1 offered as a 5-subject run, a relay-subject stepped twice in a cycle.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subjects_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g46self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subjects_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/five/subject_summary.csv" ] || { echo "  G46 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G46 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G46 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G46 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G46 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G46 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
S5="$T/f/five/subject_summary.csv"
SR="$T/f/relay/subject_summary.csv"
sub() {  # sub <subjectId> <arm> <seed>: a python expression selecting that row
    echo "next(r for r in rows if r['subjectId'] == '$1' and r['arm'] == '$2' and r['seed'] == '$3')"
}

fresh
check "P0 the real run set" 0 -

fresh; edit "$S5" 'del rows[7]'
check "N1 a dropped subject row" 1 ROWS
fresh; edit "$S5" "r = $(sub 1 ca_tbl_full 8); r['rmseM'] = '%.4f' % (float(r['rmseM']) + 0.5)"
check "N2 subject 1 != arm_summary" 1 SUBJ1
fresh; edit "$S5" "r = $(sub 3 static_868 7); r['tx_uwb'] = '0.000100000'; r['j_uwb'] = '%.9f' % (float(r['j_uwb']) + 1e-4)"
check "N3 a charged unused tier" 1 SIX
fresh; rm -rf "$T/f/five"; cp -r "$REAL/one" "$T/f/five"; cp "$REAL/one.log" "$T/f/five.log"
check "N4 --measuredSubjects=1 offered as a 5-subject run" 2 -
grep -q "MISSING" "$T/out.txt" && grep -Eq "G46 FAIL +N " "$T/out.txt" ||
    { echo "  G46 FAIL  N4 did not fail N while reading as missing"; fail=$((fail + 1)); pass=$((pass - 1)); }
fresh; edit "$SR" "r = $(sub 2 ca_tbl_full 7); r['cycles_2p4'] = str(int(r['cycles_2p4']) + int(r['steps']))"
check "N5 a relay-subject stepped twice in a cycle (policy cycles 2x steps)" 1 STEPS

fresh; cp "$REAL/five/subject_summary.csv" "$T/f/one/"
check "N6 the default writes subject_summary.csv" 1 NULL
fresh; sed -i 's/--measuredSubjects=5/--measuredSubjects=4/' "$T/f/five/run_id.txt"
check "N7 the recorded invocation is not a five-subject run" 1 N
fresh; edit "$T/f/five/subjtrace_ca_tbl_full_seed7_subj2.csv" 'del rows[-1]'
check "N8 a subjtrace with one cycle missing" 1 STEPS
fresh; edit "$S5" "r = $(sub 2 ca_tbl_full 7); r['fixCycles'] = str(int(r['fixCycles']) + 1)"
check "N9 fixCycles that the trace's hadFix does not support" 1 RECOUNT
fresh; edit "$T/f/five/subjtrace_static_868_seed8_subj4.csv" 'rows[30]["activeTier"] = "UWB"'
check "N10 a trace tier that the cycle counts do not show" 1 RECOUNT
fresh; edit "$S5" "r = $(sub 2 static_868 7); r['escRequests'] = '3'"
check "N11 a static_868 subject that sent requests" 1 OWN
fresh; sed -i '0,/subj 2 steps 240 fixes [0-9]*/s//subj 2 steps 240 fixes 9999/' "$T/f/five.log"
check "N12 an MS-1 line that disagrees with the summary" 1 OWN
fresh; edit "$T/f/relay/subjtrace_ca_tbl_full_seed7_subj3.csv" 'rows[0]["pcs"] = "0.5"'
check "N13 a subjtrace that is not the file digested" 1 DIGEST
fresh; echo 0 > "$T/f/r_big.rc"
check "N14 N > --nTags accepted" 1 R
fresh; sed -i '/WARNING: --measuredSubjects/d' "$T/f/warn.log"
check "N15 a tag-0-only mechanism not warned about" 1 R
fresh; echo 134 > "$T/f/relay.rc"
check "N16 a run the checks read did not exit 0" 1 RUNS

printf '  G46 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
