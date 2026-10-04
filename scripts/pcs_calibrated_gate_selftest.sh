#!/usr/bin/env bash
# G28 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/pcs_calibrated_gate_selftest.sh [<dir written by pcs_calibrated_gate.sh>]
#   Without a directory it makes its own real run set first (about 30 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# each must fail for ITS reason: the check it names must be the one that reports FAIL.
# The ones that matter most are the states of the tree before DC-6 or after a regression:
#   N4   --pcsW2=0 ignored, i.e. w0 is the weighted default -- the tree before 2026-09-30;
#   N6/N7/N8  a threshold flag that reaches nothing (its run is the baseline's);
#   N7b  --pcsThrUp2p4 landing in the 868 MHz slot;
#   N9   two tiers' slots swapped in what was resolved;
#   N12  an inverted pair accepted because only one side was explicit -- this commit's
#        first draft, which checked explicit pairs only.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/pcs_calibrated_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g28self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/pcs_calibrated_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/legacy/arm_summary.csv" ] || { echo "  G28 selftest: no real run set in $REAL"; exit 2; }
TR=trace_ca_tbl_full_seed7.csv

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G28 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G28 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G28 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /'
        pass=$((pass + 1))
    else
        printf '  G28 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G28 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# setcol <csv> <column> <value>: the single arm_summary row's column set
setcol() {
    python3 - "$@" <<'PY'
import csv, sys
p, col, val = sys.argv[1:4]
rows = list(csv.reader(open(p, newline="")))
rows[1][rows[0].index(col)] = val
csv.writer(open(p, "w", newline=""), lineterminator="\n").writerows(rows)
PY
}
# tracemap <csv> <python expression over r, the row dict, giving the new pcs string>
tracemap() {
    python3 - "$@" <<'PY'
import csv, sys
p, expr = sys.argv[1:3]
with open(p, newline="") as f:
    rd = csv.DictReader(f)
    fields = rd.fieldnames
    rows = list(rd)
for r in rows:
    r["pcs"] = eval(expr)
with open(p, "w", newline="") as f:
    w = csv.DictWriter(f, fieldnames=fields, lineterminator="\n")
    w.writeheader()
    w.writerows(rows)
PY
}

fresh
check "P0 the real run set" 0 -

fresh; rm -rf "$T/f/cal111"; cp -r "$T/f/wdef" "$T/f/cal111"
check "N0 calibrated form dropped (cal111 is the hand-set weighted run)" 1 P1
fresh; sed -i '10s/,1\.[0-9][0-9][0-9][0-9],/,1.9999,/' "$T/f/cal111/$TR"
cmp -s "$T/f/cal111/$TR" "$REAL/cal111/$TR" && echo "  G28 selftest: N1 fixture did not change the trace"
check "N1 one pcs value moved in the calibrated trace" 1 P1
fresh; setcol "$T/f/never/arm_summary.csv" escalations 1
check "N2 thresholds below every score still escalate" 1 P2
fresh; setcol "$T/f/always/arm_summary.csv" escalations 0
check "N3 thresholds above every score never escalate" 1 P3
fresh; rm -rf "$T/f/w0"; cp -r "$T/f/wdef" "$T/f/w0"
check "N4 --pcsW2=0 ignored: w0 is the weighted default (pre-DC-6 tree)" 1 W0
fresh; sed -i '20s/,ca_tbl_full,7,\([^,]*\),/,ca_tbl_full,7,UWB,/' "$T/f/w0/$TR"
cmp -s "$T/f/w0/$TR" "$REAL/w0/$TR" && echo "  G28 selftest: N5 fixture did not change the trace"
check "N5 w0 differs from the default outside pcs (a decision moved)" 1 W0
fresh; cp "$T/f/wdef/$TR" "$T/f/w0/$TR"
tracemap "$T/f/w0/$TR" \
    "r['pcs'] if float(r['pcs']) == 0.0 else '%.4f' % (float(r['pcs']) - 0.25 / max(float(r['gdop']), 1.0))"
check "N5b --pcsW2=0 taken as 0.25, not 0" 1 W0
fresh; rm -rf "$T/f/ev_up868"; cp -r "$T/f/ev_leg" "$T/f/ev_up868"
check "N6 --pcsThrUp868 reaches nothing" 1 T868
fresh; rm -rf "$T/f/ev_up2p4"; cp -r "$T/f/ev_leg" "$T/f/ev_up2p4"
check "N7 --pcsThrUp2p4 reaches nothing" 1 T2P4
fresh; cp "$T/f/ev_up868/arm_summary.csv" "$T/f/ev_up2p4/arm_summary.csv"
check "N7b --pcsThrUp2p4 lands in the 868 MHz slot" 1 T2P4
fresh; rm -rf "$T/f/ev_dn2p4"; cp -r "$T/f/ev_leg" "$T/f/ev_dn2p4"
check "N8 --pcsThrDown2p4 reaches nothing" 1 TD2P4
fresh; sed -i 's|up 868/2p4/uwb 0.11 0.12 |up 868/2p4/uwb 0.12 0.11 |' "$T/f/slots.log"
cmp -s "$T/f/slots.log" "$REAL/slots.log" && echo "  G28 selftest: N9 fixture did not change the log"
check "N9 868 MHz and 2.4 GHz up slots swapped" 1 SLOTS
fresh; sed -i 's|down 2p4/uwb \([^ ]*\) 3$|down 2p4/uwb \1 2.6000000000000001|' "$T/f/slots_band.log"
cmp -s "$T/f/slots_band.log" "$REAL/slots_band.log" && echo "  G28 selftest: N10 fixture did not change the log"
check "N10 the band wins over an explicit down threshold" 1 SLOTS
fresh; sed -i '/^NOTE: PCS form/d' "$T/f/slots_v2.log"
check "N11 nothing resolved is printed" 1 SLOTS
fresh; echo 0 > "$T/f/r_inv_resolved.rc"
check "N12 an inverted pair accepted when only one side is explicit" 1 R
fresh; echo "FATAL: something else" > "$T/f/r_missing_thr.log"
check "N13 a refusal for another reason" 1 R
fresh; echo 0 > "$T/f/r_down868.rc"
check "N14 --pcsThrDown868 accepted (a flag that decides nothing)" 1 R
fresh; echo 1 > "$T/f/r_up868_ok.rc"
check "N15 the inert 868 MHz pair refused" 1 R
fresh; echo 134 > "$T/f/ev_leg.rc"
check "N16 a run the checks read did not exit 0" 1 RUNS
fresh; rm "$T/f/legacy/arm_summary.csv"
check "N17 an input missing" 2 P1

printf '  G28 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
