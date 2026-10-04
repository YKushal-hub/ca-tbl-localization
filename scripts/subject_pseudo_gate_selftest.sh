#!/usr/bin/env bash
# G68 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_pseudo_gate_selftest.sh [<dir written by subject_pseudo_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N4: "reported" positions that
# are the truth -- the oracle this flag exists to remove.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_pseudo_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g68self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_pseudo_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/subjpseudo_ca_tbl_full_seed7.csv" ] || { echo "  G68 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G68 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G68 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G68 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G68 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G68 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
edit() {  # edit <csv> <python statement over rows>
    python3 - "$1" "$2" <<'PY'
import csv, sys
p, stmt = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
exec(stmt)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=cols, lineterminator="\n")
w.writeheader()
w.writerows(rows)
PY
}
F="$T/f/on/subjpseudo_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$F" "r = next(r for r in rows if r['kind'] == 'report'); r['predX'] = '%.4f' % (float(r['predX']) + 3.0)"
check "N1 a position not carried forward as DC-13 says" 1 PRED
fresh; edit "$F" "r = next(r for r in rows if r['kind'] == 'report'); r['sigmaEff'] = '%.6f' % (float(r['sigmaRaw']) * 0.5)"
check "N2 a sigma below the raw one" 1 SIGMA
fresh; edit "$F" "r = next(r for r in rows if r['kind'] == 'report'); r['sigmaEff'] = '%.6f' % (float(r['sigmaRaw']) + 1e4)"
check "N3 a sigma past the entry's covariance" 1 SIGMA
fresh; edit "$F" "[r.update(predX=r['trueX'], predY=r['trueY']) for r in rows if r['kind'] == 'report']"
check "N4 reported positions that are the truth" 1 NOTTRUE
fresh; sed -i -E 's/(reported positions used )[0-9]+/\199999/' "$T/f/on.log"
check "N5 an N.21 line the file does not support" 1 COUNTS
fresh; cp "$F" "$T/f/off/"
check "N6 the control writing pseudo-anchor rows" 1 CONTROL
fresh; echo 0 > "$T/f/r_nopa.rc"
check "N7 the flag accepted without --finderPseudoAnchor" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS
fresh; sed -i -E 's/(digests asked )[0-9]+/\10/' "$T/f/on.log"
check "N9 subjects never asking for finder entries" 1 ROWS
fresh; sed -i -E 's/(TABLE\(reverse\): casualty sent [0-9]+ digests, holds )[0-9.]+/\10.0/' "$T/f/on.log"
check "N10 the subjects' questions drowning tag 0's" 1 TAG0

printf '  G68 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
