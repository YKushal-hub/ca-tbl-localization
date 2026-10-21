#!/usr/bin/env bash
# G167 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/oracle_esc_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g167self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/oracle_esc_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G167 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G167 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G167 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G167 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
# set column $3 to $4 on the rows of trace $1 where deescalated == 1 (or on every row with $2 = all)
trace_set() {
    python3 - "$@" <<'P'
import csv, sys
p, which, col, val = sys.argv[1:5]
rs = list(csv.DictReader(open(p))); f = list(rs[0].keys())
for r in rs:
    if which == "all" or r["deescalated"] == "1":
        r[col] = val
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=f); w.writeheader(); w.writerows(rs)
P
}
TR=trace_fixed_threshold_seed7.csv
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the flag-on run crashed" 1 RUNS
fresh; f=$(ls "$T/f/off" | grep -v run_id.txt | head -1); echo x >> "$T/f/off/$f"; check "N2 --oracleTrigger=false not the default" 1 OFF
fresh; sed -i '/ORACLE TRIGGER/d' "$T/f/on.log"; check "N3 the flag not announced" 1 LINE
fresh; echo "ORACLE TRIGGER (--oracleTrigger): x" >> "$T/f/abs.log"; check "N4 the oracle announced with the flag absent" 1 LINE
fresh; f=$(ls "$T/f/on" | grep ca_tbl_full | head -1); echo x >> "$T/f/on/$f"; check "N5 the flag reaching ca_tbl_full" 1 ARMS
fresh; sed -i 's/(fixed_threshold): decisions on the true error, tag 0 [0-9]*/(fixed_threshold): decisions on the true error, tag 0 0/' "$T/f/on.log"; check "N6 tag 0 never deciding on the true error" 1 COUNT
fresh; sed -i 's/(ca_tbl_full): decisions on the true error, tag 0 0/(ca_tbl_full): decisions on the true error, tag 0 5/' "$T/f/on.log"; check "N7 ca_tbl_full deciding on the true error" 1 COUNT
fresh; cp "$T/f/abs/$TR" "$T/f/on/$TR"; check "N8 the flag-on trigger still reading trace(P)" 1 RULE
fresh; trace_set "$T/f/on/$TR" deesc errM 100.0000; check "N9 a de-escalation with a large true error" 1 RULE
fresh; trace_set "$T/f/abs/$TR" all errM 0.0000; check "N10 a control that cannot tell the triggers apart" 1 RULE
echo "  G167 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
