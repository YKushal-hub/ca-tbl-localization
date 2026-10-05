#!/usr/bin/env bash
# G88 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/lora_fusion_gate_selftest.sh [<dir written by lora_fusion_gate.sh>]
#   Without a directory it makes its own real run set first (about 3 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N1 and N2: the comparator
# disturbing the arms it is compared with, or joining every default run; N4/N5 are the
# arm quietly becoming Poucet again (UWB ranged or a fix labelled UWB).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/lora_fusion_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g88self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/lora_fusion_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/with/trace_lora_fusion_seed7.csv" ] || { echo "  G88 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G88 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G88 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G88 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G88 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G88 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
P="$T/f/with/trace_lora_fusion_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; f="$T/f/with/trace_static_868_seed7.csv"; sed -i '2s/,/,9/' "$f"
check "N1 the comparator moving the arm beside it" 1 INDEP
fresh; edit "$T/f/all/arm_summary.csv" "r = dict(rows[0]); r['arm'] = 'lora_fusion'; rows.append(r)"
check "N2 the comparator joining --arms=all" 1 ALL
fresh; edit "$P" "rows[5]['activeTier'] = '2.4GHz'"
check "N3 the comparator escalating" 1 STATIC
fresh; edit "$T/f/with/arm_summary.csv" "r = next(r for r in rows if r['arm'] == 'lora_fusion'); r['tx_uwb'] = '0.001200000'"
check "N4 a UWB exchange billed" 1 UWBOFF
fresh; edit "$P" "r = next(r for r in rows if r['fixTier'] == '868MHz'); r['fixTier'] = 'UWB'"
check "N5 a fix labelled UWB" 1 UWBOFF
fresh; edit "$T/f/with/arm_summary.csv" "s = next(r for r in rows if r['arm'] == 'static_868'); r = next(r for r in rows if r['arm'] == 'lora_fusion'); r['j_2p4'] = s['j_2p4']; r['cad_2p4'] = s['cad_2p4']"
check "N6 the 2.4 GHz radio left asleep" 1 AWAKE
fresh; edit "$P" "[r.__setitem__('nRangesEkf', r['nbr2p4']) for r in rows]"
check "N7 the two bands never in one set" 1 FUSED
fresh; echo 134 > "$T/f/without.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G88 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
