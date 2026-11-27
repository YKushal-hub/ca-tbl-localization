#!/usr/bin/env bash
# G192 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
GATE="$(pwd)/scripts/finder_both_lora_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g192self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash scripts/finder_both_lora_gate.sh "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G192 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; mkdir "$T/f"; cp "$REAL"/*.rc "$REAL"/*.log "$T/f/"; for n in off false on; do mkdir "$T/f/$n"; cp "$REAL/$n"/*.csv "$T/f/$n/"; done; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G192 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G192 ok    %-58s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G192 FAIL  %-58s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 a run crashed" 1 RUNS
fresh; echo x >> "$T/f/false/arm_summary.csv"; check "N2 an explicit default changes an output" 1 OFFSAME
fresh; sed -i '/DD78 finderDigestBothLora (ca_tbl_full)/d' "$T/f/on.log"; check "N3 the switch not announced for ours" 1 LINES
fresh; sed -i 's/DD78 finderDigestBothLora (static_2p4): one LoRa radio, unused/DD78 finderDigestBothLora (static_2p4): on/' "$T/f/on.log"; check "N4 a single-radio arm switched on" 1 LINES
fresh; f=$(ls "$T"/f/on/trace_static_2p4_*.csv | head -1); sed -i '$d' "$f"; check "N5 the 2.4 GHz-only arm changed" 1 SINGLE
fresh; sed -i 's/(ca_tbl_full): on, searcher polls on 2.4 GHz [0-9]*/(ca_tbl_full): on, searcher polls on 2.4 GHz 0/' "$T/f/on.log"; check "N6 no polls on 2.4 GHz" 1 POLLS
fresh; for x in "$T"/f/off/*ca_tbl_full*.csv; do cp "$x" "$T/f/on/"; done; check "N7 no effect on ours" 1 EFFECT
fresh; rm "$T"/f/off/*.csv "$T"/f/false/*.csv; check "N8 no outputs (vacuous)" 1 OFFSAME
echo "  G192 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
