#!/usr/bin/env bash
# G89 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/n26_fixes_gate_selftest.sh [<dir written by n26_fixes_gate.sh>]
#   Without a directory it makes its own real run set first (about 4 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names: each fix silently not firing (N4-N8), a fix leaking into the
# off runs (N2) or into the static arm (N3).
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/n26_fixes_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g89self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/n26_fixes_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/e_dual/trace_ca_tbl_full_seed7.csv" ] || { echo "  G89 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G89 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G89 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G89 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G89 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G89 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
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
TD="$T/f/e_dual/trace_ca_tbl_full_seed7.csv"
TC="$T/f/e_cross/trace_ca_tbl_full_seed7.csv"
TQ="$T/f/e_chi/trace_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/e_chi.rc"
check "N1 a run the checks read did not exit 0" 1 RUNS
fresh; grep -m1 'N.26 fixes' "$T/f/e_dual.log" >> "$T/f/e_off.log"
check "N2 a fix firing with every flag off" 1 OFF
fresh; sed -i '2s/,/,9/' "$T/f/e_dual/trace_static_868_seed7.csv"
check "N3 the dual base moving the static arm" 1 STATIC
fresh; sed -i 's/dual LoRa base node-cycles [0-9]*/dual LoRa base node-cycles 0/' "$T/f/e_dual.log"
check "N4 the dual base not ranging both tiers" 1 DUAL
fresh; edit "$TD" "[r.__setitem__('nRangesEkf', r['nbr2p4']) for r in rows if r['fixTier'] == '2.4GHz']"
check "N5 the two bands never fused" 1 DUAL
fresh; edit "$TC" "[r.__setitem__('polled', r['nbrUwb']) for r in rows]"
check "N6 cross-tier polls never addressed" 1 CROSS
fresh; edit "$TQ" "r = next(r for r in rows if r['suspect'] == '1'); r['mahalanobis'] = '15.0'"
check "N7 a SUSPECT judged against the old threshold" 1 CHI
fresh; sed -i 's/TDoA innovation gate refused [0-9]*/TDoA innovation gate refused 0/' "$T/f/k_gate.log"
check "N8 the TDoA gate never refusing" 1 GATE

printf '  G89 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
