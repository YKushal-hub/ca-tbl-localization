#!/usr/bin/env bash
# G190 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
GATE="$(pwd)/scripts/sigma_shadow_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g190self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash scripts/sigma_shadow_gate.sh "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G190 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; mkdir "$T/f"; cp "$REAL"/*.rc "$REAL"/*.log "$T/f/"; for n in off false on; do mkdir "$T/f/$n"; cp "$REAL/$n"/*.csv "$T/f/$n/"; done; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G190 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G190 ok    %-58s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G190 FAIL  %-58s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
S="$T/f/on/subjtrace_ca_tbl_full_seed731_subj5.csv"
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 a run crashed" 1 RUNS
fresh; echo x >> "$T/f/false/arm_summary.csv"; check "N2 an explicit default changes an output" 1 OFFSAME
fresh; sed -i '/DD75 sigmaShadow/d' "$T/f/on.log"; check "N3 the shadow not announced" 1 LINES
fresh; echo "DD75 sigmaShadow" >> "$T/f/off.log"; check "N4 the shadow on in the control" 1 LINES
fresh; rm "$S"; check "N5 a crew trace missing" 1 ROWS
fresh; sed -i '$d' "$S"; check "N6 a truncated crew trace" 1 ROWS
fresh; python3 - "$S" <<'PY'
import csv,sys; r=list(csv.DictReader(open(sys.argv[1]))); f=r[0].keys(); r[-1]['estX']=str(float(r[-1]['estX'])+1.0)
w=csv.DictWriter(open(sys.argv[1],'w',newline=''),fieldnames=f,lineterminator='\n'); w.writeheader(); w.writerows(r)
PY
check "N7 the estimate moved" 1 ESTSAME
fresh; python3 - "$S" <<'PY'
import csv,sys; r=list(csv.DictReader(open(sys.argv[1]))); f=r[0].keys(); r[-1]['traceSigma']='0.0000'
w=csv.DictWriter(open(sys.argv[1],'w',newline=''),fieldnames=f,lineterminator='\n'); w.writeheader(); w.writerows(r)
PY
check "N8 a published sigma lowered" 1 NOLOWER
fresh; cp "$T"/f/off/*trace_* "$T/f/on/"; check "N9 no effect" 1 EFFECT
fresh; rm "$T"/f/on/*trace_*; rm "$T"/f/off/*trace_*; check "N10 no traces (vacuous)" 1 ROWS
echo "  G190 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
