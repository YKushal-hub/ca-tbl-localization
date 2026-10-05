#!/usr/bin/env bash
# G81 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/reqcarry_gate_selftest.sh [<dir written by reqcarry_gate.sh>]
#   Without a directory it makes its own real run set first (about 5 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N4 and N5: an arbiter scoring a
# request no frame delivered, or at the instant it was handed over -- the instant, unbilled
# delivery this flag replaces.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/reqcarry_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g81self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/reqcarry_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/reqcarry_ca_tbl_full_seed7.csv" ] || { echo "  G81 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G81 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G81 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G81 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G81 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G81 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
C="$T/f/on/reqcarry_ca_tbl_full_seed7.csv"
Q="$T/f/on/reqesc_ca_tbl_full_seed7.csv"
A="$T/f/on/arb_ca_tbl_full_seed7.csv"
E="$T/f/on/events_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$C" "r = next(r for r in rows if r['outcome'] == 'carried'); r['carrier'] = r['arbiter']"
check "N1 a request carried by its own arbiter" 1 ROUTE
fresh; edit "$C" "r = next(r for r in rows if r['outcome'] == 'carried'); r['heardAgeS'] = '-1.000000'"
check "N2 a carrier that never heard the arbiter" 1 ROUTE
fresh; edit "$Q" "r = next(r for r in rows if r['outcome'] in ('carried', 'unheard')); r['outcome'] = 'granted'"
check "N3 a finder arbiter reached with no frame (the old path)" 1 LINK
fresh; edit "$C" "rows.pop(next(k for k, r in enumerate(rows) if r['outcome'] == 'carried'))"
check "N4 a carried request with no carriage row" 1 LINK
fresh; python3 - "$E" <<'PY'
import sys
p = sys.argv[1]
ls = open(p).read().splitlines(True)
k = next(i for i, l in enumerate(ls) if ',ESC_REQUEST,' in l and ';for=' in l and (',delivered,' in l or ',capture,' in l))
del ls[k]
open(p, 'w').write(''.join(ls))
PY
check "N5 an arbiter scoring a request no frame delivered" 1 ACT
fresh; python3 - "$C" "$A" <<'PY'
import csv, sys
c = next(r for r in csv.DictReader(open(sys.argv[1])) if r['outcome'] == 'carried')
p = sys.argv[2]
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
x = dict(next(a for a in rows if a['event'] == 'obo_submit'))
x['t'], x['arbiter'], x['requester'] = c['t'], c['arbiter'], c['subject']
rows.append(x)
w = csv.DictWriter(open(p, 'w', newline=''), fieldnames=cols, lineterminator='\n'); w.writeheader(); w.writerows(rows)
PY
check "N6 a request scored at the instant it was handed over" 1 ACT
fresh; sed -i -E 's/(N\.10 request over mesh \(\S+\): carried )([0-9]+)/\1999/' "$T/f/on.log"
check "N7 an N.10 line the file does not support" 1 COUNTS
fresh; cp "$C" "$T/f/off/"
check "N8 the control writing carriages" 1 CONTROL
fresh; echo 0 > "$T/f/r_noreq.rc"
check "N9 the flag accepted without --requesterEscalation" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS
fresh; sed -i -E '/,1[0-9][0-9],[0-9]+,ESC_REQUEST,.*;for=/d' "$E" "$T/f/on_hd/events_ca_tbl_full_seed7.csv"
check "N11 no gateway-carried request ever delivered" 1 ROWS

printf '  G81 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
