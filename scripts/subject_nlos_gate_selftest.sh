#!/usr/bin/env bash
# G78 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_nlos_gate_selftest.sh [<dir written by subject_nlos_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_nlos_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g78self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_nlos_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G78 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G78 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G78 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G78 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G78 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G78 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}

fresh
check "P0 the real run set passes" 0 -
fresh; rm "$T"/f/on/subjnlos_summary_*_subj3.csv
check "N1 a subject without its summary file" 1 FILES
fresh; for f in "$T"/f/on/subjnlos_links_*.csv; do head -1 "$f" > "$f.h"; mv "$f.h" "$f"; done
check "N2 no subject link scored" 1 SCORED
fresh; sed -i -E 's/(NLOS PCS term 3 \(subject [0-9]+\) = .*fed to PCS )[0-9]+/\10/' "$T/f/on.log"
check "N3 no subject's policy fed its score to PCS" 1 FED
fresh; sed -i -E '0,/(NLOS PCS term 3 \(subject [0-9]+\) = mean\(1 - NLOS_score\): fix cycles )[0-9]+/s//\199999/' "$T/f/on.log"
check "N4 a subject's fix cycles its trace does not support" 1 FIXES
fresh; sed -i -E '0,/NLOS classifier \(threshold/s//NLOS classifier (subject 1) (threshold/' "$T/f/on.log"
check "N5 tag 0's classifier line relabelled" 1 TAG0
fresh; cp "$T"/f/on/subjnlos_links_*_subj2.csv "$T/f/off/"
check "N6 the control writing subject scores" 1 CONTROL
fresh; echo 0 > "$T/f/r_nonlos.rc"
check "N7 the flag accepted without --nlosClassifier" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G78 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
