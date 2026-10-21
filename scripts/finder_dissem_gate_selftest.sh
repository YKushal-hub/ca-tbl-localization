#!/usr/bin/env bash
# G165 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/finder_dissem_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g165self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/finder_dissem_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/bce" ] || { echo "  G165 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G165 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G165 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G165 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/need.rc"; check "N1 a flag's run crashed" 1 RUNS
fresh; f=$(ls "$T/f/bce"/events_ca_tbl_full_seed*.csv); sed -i 's/;entry=[^,]*//' "$f"; check "N2 beacons carrying no entry" 1 BCE
fresh; f=$(ls "$T/f/off"/events_ca_tbl_full_seed*.csv); cp "$(ls "$T/f/bce"/events_ca_tbl_full_seed*.csv)" "$f"; check "N3 the switched-off run's beacons carrying entries" 1 BCE
fresh; sed -i 's/heard by other radios, not merged/X/' "$T/f/bce.log"; sed -i 's/; [0-9]* X/; 0 heard by other radios, not merged/' "$T/f/bce.log"; check "N4 subjects merging neighbours' beacon entries (none ignored)" 1 BCE
fresh; cp "$(ls "$T/f/off"/tasking_ca_tbl_full_seed*.csv)" "$(ls "$T/f/bce"/tasking_ca_tbl_full_seed*.csv)"; check "N5 no freshness gained" 1 BCE
fresh; sed -i 's/digest answers, [0-9]* of them/digest answers, 0 of them/' "$T/f/dof.log"; check "N6 the own-first rule never taken" 1 DOF
fresh; f=$(ls "$T/f/need"/tasking_ca_tbl_full_seed*.csv); awk -F, 'BEGIN{OFS=","} NR>1 && $3==5 {$3=1} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N7 a subject with no finder at dispatch" 1 NEED
fresh; sed -i 's/stale-entry search (ca_tbl_full): [0-9]*/stale-entry search (ca_tbl_full): 0/' "$T/f/ss.log"; check "N8 the stale search never entered" 1 SS
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N9 a flag not off by default" 1 OFF
fresh; echo 0 > "$T/f/r_task.rc"; check "N10 an unknown tasking accepted" 1 REFUSE
echo "  G165 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
