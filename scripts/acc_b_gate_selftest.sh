#!/usr/bin/env bash
# G110 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/acc_b_gate_selftest.sh [<dir written by acc_b_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/acc_b_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g110self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/acc_b_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/off.rc" ] || { echo "  G110 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G110 FAIL +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G110 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G110 FAIL  %-62s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G110 FAIL" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set" 0 -
fresh; rm -rf "$T/f/on"; cp -r "$REAL/off" "$T/f/on"; cp "$REAL/off.log" "$T/f/on.log"
check "N1 two runs without the flag labelled off and on" 1 MOVED
fresh; f=$(ls "$T"/f/on/trace_static_2p4_seed*.csv | head -1); echo "x" >> "$f"
check "N2 the flag leaking into static_2p4" 1 ISOLATION
fresh; f=$(cd "$T/f/zero" && find . -type f ! -name 'run_id.txt' ! -name 'trace_*' | head -1); echo x >> "$T/f/zero/$f"
check "N3 the explicit default changes an output ($f)" 1 DEFAULT
fresh; python3 - "$T/f" <<'PY'
import csv, sys
d = sys.argv[1]
off = {r["arm"]: r for r in csv.DictReader(open(d + "/off/arm_summary.csv"))}
rd = csv.DictReader(open(d + "/on/arm_summary.csv")); h = rd.fieldnames; rows = list(rd)
for r in rows:
    if r["arm"] == "ca_tbl_full":
        r["j_2p4"] = off["ca_tbl_full"]["j_2p4"]
w = csv.DictWriter(open(d + "/on/arm_summary.csv", "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N4 legs formed but not billed on 2.4 GHz" 1 BILLED
fresh; echo 134 > "$T/f/on.rc"; check "N5 a run did not exit 0" 1 RUNS
printf '  G110 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
