#!/usr/bin/env bash
# G70 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/silence_relay_gate_selftest.sh [<dir written by silence_relay_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N2: a frame from the dead node
# heard long after its death -- the relay this flag exists to stop.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/silence_relay_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g70self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/silence_relay_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/events_ca_tbl_full_seed7.csv" ] || { echo "  G70 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G70 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G70 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G70 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G70 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G70 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
EV=events_ca_tbl_full_seed7.csv
# a relayed frame "from node 1" at the last row's time (hop limit 1)
late_row() {  # late_row <run>
    local f="$T/f/$1/$EV"
    local last
    last=$(tail -1 "$f" | cut -d, -f1)
    printf '%s,0,1,2,DISCOVERY,delivered,-90.0,pkt=1;hops=1\n' "$last" >> "$f"
}

fresh
check "P0 the real run set passes" 0 -
fresh; python3 - "$T/f/off/$EV" <<'PY'
import csv, sys
p = sys.argv[1]
rows = list(csv.reader(open(p)))
keep = [rows[0]] + [r for r in rows[1:] if r[2] != "1"]
csv.writer(open(p, "w", newline=""), lineterminator="\n").writerows(keep)
PY
check "N1 the defect not reproduced (the dead node never transmits)" 1 DEFECT
fresh; late_row on
check "N2 a frame from the dead node long after its death" 1 FIX
fresh; sed -i -E 's/(relays refused )[0-9]+/\10/' "$T/f/on.log"
check "N3 the flag on and nothing refused" 1 FIX
fresh; python3 - "$T/f/on/$EV" <<'PY'
import csv, sys
p = sys.argv[1]
rows = list(csv.reader(open(p)))
keep = [rows[0]] + [r for r in rows[1:] if not (r[2] == "1" and r[4] == "TRIGGERED_UPDATE")]
csv.writer(open(p, "w", newline=""), lineterminator="\n").writerows(keep)
PY
check "N4 the last gasp no longer leaving" 1 GASP
fresh; printf '450.0,0,1,2,DISCOVERY,delivered,-90.0,pkt=1;hops=1\n' >> "$T/f/sil/$EV"
check "N5 a mute node relaying inside its window" 1 SIL
fresh; printf '    N.1 silenced radios (ca_tbl_full): relays refused 3\n' >> "$T/f/off.log"
check "N6 the control refusing relays" 1 CONTROL
fresh; echo 0 > "$T/f/r_none.rc"
check "N7 the flag accepted with neither fault" 1 R
fresh; echo 134 > "$T/f/sil.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G70 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
