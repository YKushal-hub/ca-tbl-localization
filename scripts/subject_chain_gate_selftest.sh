#!/usr/bin/env bash
# G62 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_chain_gate_selftest.sh [<dir written by subject_chain_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N1: a depth-2 range through a
# subject whose depth-1 fuse never happened -- depth from nowhere, the defect the first
# draft of L.1.2 shipped.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_chain_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g62self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_chain_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/relay_chain_static_2p4_seed7.csv" ] || { echo "  G62 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G62 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G62 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G62 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G62 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G62 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
# edit <csv> <python statement over rows>: rewrite a CSV in place, header kept
edit() {
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
C="$T/f/on/relay_chain_static_2p4_seed7.csv"
SUBJ='{2, 3, 4}'

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$C" "rows[:] = [r for r in rows if not (r['leg'] == 'fuse' and int(r['consumerId']) in $SUBJ)]"
check "N1 a depth-2 range through a subject that never fused at depth 1" 1 PROV
fresh; edit "$C" "r = next(r for r in rows if r['verdict'] == 'ADMIT' and r['leg'] == 'tag'); r['a2'] = 'budget'"
check "N2 an A2 label its row's numbers do not give" 1 A2
fresh; edit "$C" "r = next(r for r in rows if r['leg'] == 'relay' and int(r['consumerId']) in $SUBJ); r['relayId'] = str(int(r['consumerId']) + 1)"
check "N3 a subject leaning on a higher-id subject, not yet stepped" 1 HOLDER
fresh; edit "$C" "r = next(r for r in rows if r['leg'] == 'relay' and int(r['consumerId']) in $SUBJ); r['tier'] = '0'"
check "N4 a subject exchange on 868 MHz" 1 HOLDER
fresh; python3 - "$C" "$T/f/on" <<'PY'
import csv, glob, sys
r = next(r for r in csv.DictReader(open(sys.argv[1])) if r['leg'] == 'relay' and int(r['consumerId']) in (2, 3, 4))
p = glob.glob(f"{sys.argv[2]}/subjtrace_*_subj{r['consumerId']}.csv")[0]
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
for x in rows:
    if abs(float(x['t']) - float(r['t'])) < 1e-3:
        x['activeTier'] = 'UWB'
w = csv.DictWriter(open(p, 'w', newline=''), fieldnames=cols, lineterminator='\n'); w.writeheader(); w.writerows(rows)
PY
check "N5 a subject exchange on a tier the subject was not on" 1 SIX
fresh; python3 - "$C" "$T/f/on" <<'PY'
import csv, glob, sys
r = next(r for r in csv.DictReader(open(sys.argv[1])) if r['leg'] == 'relay' and int(r['consumerId']) in (2, 3, 4))
p = glob.glob(f"{sys.argv[2]}/subjtrace_*_subj{r['relayId']}.csv")[0]
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
for x in rows:
    if abs(float(x['t']) - float(r['t'])) < 1e-3:
        x['activeTier'] = 'UWB'
w = csv.DictWriter(open(p, 'w', newline=''), fieldnames=cols, lineterminator='\n'); w.writeheader(); w.writerows(rows)
PY
check "N6 a holder asleep on the exchange's tier" 1 SIX
fresh; O="$T/f/off/relay_chain_static_2p4_seed7.csv"; edit "$O" "rows.append(dict(next(r for r in rows), consumerId='3', relayId='2', leg='fuse', hopDepth='1'))"
check "N7 the control with a subject fuse" 1 CONTROL
fresh; sed -i -E '0,/(L\.1\.2 subject chain: .*, fused )([0-9]+)/s//\199/' "$T/f/on.log"
check "N8 a subject-chain line the rows do not support" 1 COUNTS
fresh; sed -i -E 's/(fixes by depth of the fused set 0:[0-9]+ 1:[0-9]+ 2:)[0-9]+/\10/' "$T/f/on.log"
check "N9 no fix at depth 2" 1 DEPTH2
fresh; sed -i -E 's/(on 868 MHz, where A1 admits nothing, )[0-9]+/\10/' "$T/f/full.log"
check "N10 the tier-1 skip never reached" 1 TIER1
fresh; echo 0 > "$T/f/r_nochain.rc"
check "N11 the flag accepted without --relayChain" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N12 a run the checks read did not exit 0" 1 RUNS
fresh; edit "$C" "rows[:] = [r for r in rows if not (r['leg'] in ('relay', 'fuse') and int(r['consumerId']) in $SUBJ)]"
check "N13 no subject ever in the chain" 1 ROWS

printf '  G62 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
