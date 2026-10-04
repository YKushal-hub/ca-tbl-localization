#!/usr/bin/env bash
# G31 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/entry_velocity_gate_selftest.sh [<dir written by entry_velocity_gate.sh>]
#   Without a directory it makes its own real run set first (about 50 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N0  every velocity 0 -- the tree before DC-10, the fields billed and published as 0, 0;
#   N1  a decision moved by the velocity -- something started reading it;
#   N5  a finder velocity that is not its displacement over time;
#   N7  an entry whose velocity changed in transit.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/entry_velocity_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g31self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/entry_velocity_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/s_on/entries_ca_tbl_full_seed7.csv" ] || { echo "  G31 selftest: no real run set in $REAL"; exit 2; }
EN=entries_ca_tbl_full_seed7.csv
TR=trace_ca_tbl_full_seed7.csv

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G31 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G31 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G31 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /'
        pass=$((pass + 1))
    else
        printf '  G31 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G31 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
for r in rows:
    if (every or not hit) and eval(pred):
        hit += 1
        r.update(eval(upd))
assert hit, "fixture: no row matched " + pred
with open(p, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=fields, lineterminator="\n")
    w.writeheader()
    w.writerows(rows)
PY
}
Z="'0.000000000'"

fresh
check "P0 the real run set" 0 -

fresh; rowmap "$T/f/s_on/$EN" "True" "{'vx': $Z, 'vy': $Z, 'refVx': (r['refVx'] and $Z), 'refVy': (r['refVy'] and $Z)}" all
check "N0 every velocity 0 (the tree before DC-10)" 1 PUB
fresh; rowmap "$T/f/s_on/$TR" "float(r['t']) > 100" "{'errM': '%.4f' % (float(r['errM']) + 1.0)}"
check "N1 a trace value moved when the flag came on" 1 NULL
fresh; echo x > "$T/f/r_on/stray_ca_tbl_full_seed7.csv"
check "N2 another file appears with the flag" 1 NULL
fresh; cp "$T/f/s_on/$EN" "$T/f/s_off/"
check "N3 the flag-off run writes the record" 1 OFF
fresh; rowmap "$T/f/s_on/$EN" "r['kind'] == 'pub' and r['entryId'] == '1' and float(r['vx']) != 0" "{'vx': '%.9f' % (float(r['vx']) + 0.5)}"
check "N4 a published velocity that is not the EKF's" 1 PUB
fresh; rowmap "$T/f/s_on/$EN" "r['kind'] == 'pub' and r['entryId'] == '502' and float(r['vx']) != 0" "{'vx': '%.9f' % (2 * float(r['vx'])), 'refVx': '%.9f' % (2 * float(r['refVx']))}"
check "N5 a finder velocity that is not its displacement over time" 1 FINDER
fresh; rowmap "$T/f/s_on/$EN" "r['kind'] == 'pub' and r['entryId'] == '501'" "{'vx': '0.100000000', 'refVx': '0.100000000'}"
check "N6 a finder's first publication with a velocity" 1 FINDER
fresh; rowmap "$T/f/s_on/$EN" "r['kind'] == 'held' and r['entryId'] == '1' and float(r['vx']) != 0" "{'vx': '%.9f' % (float(r['vx']) + 0.01)}"
check "N7 an entry whose velocity changed in transit" 1 CARRY
fresh; rowmap "$T/f/s_on/$EN" "r['kind'] == 'held' and r['entryId'] == '1'" "{'seqNo': '999999'}"
check "N8 a held entry with a sequence never published" 1 CARRY
fresh; rowmap "$T/f/s_on/$EN" "r['kind'] == 'held' and int(r['entryId']) < 101" "{'vx': $Z, 'vy': $Z}" all
rowmap "$T/f/s_on/$EN" "r['kind'] == 'pub' and int(r['entryId']) < 101" "{'seqNo': '-' + r['seqNo']}" all
check "N9 no held entry of the subject carries a velocity" 1 CARRY
fresh; rowmap "$T/f/s_on/$EN" "101 <= int(r['entryId']) <= 199" "{'vx': '0.250000000'}"
check "N10 a gateway entry with a velocity" 1 GW
fresh; echo 134 > "$T/f/r_on.rc"
check "N11 a run the checks read did not exit 0" 1 RUNS

printf '  G31 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
