#!/usr/bin/env bash
# G167 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/two_tier_gate_selftest.sh [<dir written by two_tier_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/two_tier_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g159self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/two_tier_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/pair.rc" ] || { echo "  G167 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
# set <csv> <arm> <column> <value>: rewrite one cell of arm_summary.csv
setcell() {
    python3 - "$1" "$2" "$3" "$4" <<'PY'
import csv, sys
p, arm, col, val = sys.argv[1:5]
rs = list(csv.DictReader(open(p)))
fn = list(rs[0].keys())
for r in rs:
    if r["arm"] == arm:
        r[col] = val
w = csv.DictWriter(open(p, "w"), fieldnames=fn, lineterminator="\n")
w.writeheader()
w.writerows(rs)
PY
}
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G167 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G167 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G167 FAIL  %-62s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G167 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set" 0 -
fresh; cp "$T/f/pair/arm_summary.csv" "$T/f/all/arm_summary.csv"; check "N1 the arm present in the default-arms run" 1 ARM
fresh; setcell "$T/f/pair/arm_summary.csv" ca_tbl_two_tier cycles_2p4 3; check "N2 two-tier with 3 tier-2 cycles" 1 TIER2
fresh; setcell "$T/f/pair/arm_summary.csv" ca_tbl_two_tier tx_2p4 0.5; check "N3 two-tier transmitting on tier 2" 1 TIER2
fresh; setcell "$T/f/pair/arm_summary.csv" ca_tbl_two_tier j_2p4 9.0; check "N4 two-tier billed more than tier-2 sleep" 1 TIER2
fresh; setcell "$T/f/pair/arm_summary.csv" ca_tbl_two_tier cycles_uwb 0; check "N5 two-tier that never reaches UWB (vacuous zero)" 1 REACH
fresh; setcell "$T/f/pair/arm_summary.csv" ca_tbl_full cycles_2p4 0; check "N6 the control arm shows no tier-2 use" 1 REACH
fresh; python3 - "$T/f/pair/subject_summary.csv" <<'PY'
import csv, sys
p = sys.argv[1]
rs = list(csv.DictReader(open(p)))
fn = list(rs[0].keys())
for r in rs:
    if r["arm"] == "ca_tbl_two_tier":
        r["tx_2p4"] = "1"
        break
w = csv.DictWriter(open(p, "w"), fieldnames=fn, lineterminator="\n")
w.writeheader()
w.writerows(rs)
PY
check "N7 one subject transmitting on tier 2" 1 TIER2
fresh; setcell "$T/f/pair/arm_summary.csv" ca_tbl_full rmseM 123.0; check "N8 the pair moves ca_tbl_full's row" 1 DEFAULT
fresh; f=$(find "$T/f/zero" -maxdepth 1 -type f ! -name 'run_id.txt' ! -name 'trace_*' | head -1); echo x >> "$f"
check "N9 the flag at 0 changes an output ($(basename "$f"))" 1 FLAG
fresh; echo 134 > "$T/f/zero.rc"; check "N10 a run did not exit 0" 1 RUNS
fresh; cp "$T/f/probe0.log" "$T/f/probe30.log"; check "N11 the flag on but the 868 MHz r80 unchanged" 1 PROBE
fresh; awk '/usable range: r80=/ {n++; if (n==2) sub(/r80=[0-9.]+/, "r80=1.0")} {print}' "$T/f/probe30.log" > "$T/p30" && mv "$T/p30" "$T/f/probe30.log"; check "N12 the flag moving the 2.4 GHz r80" 1 PROBE
printf '  G167 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
