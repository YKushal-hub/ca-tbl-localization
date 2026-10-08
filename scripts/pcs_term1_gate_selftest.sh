#!/usr/bin/env bash
# G90 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/pcs_term1_gate_selftest.sh [<dir written by pcs_term1_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/pcs_term1_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g90self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/pcs_term1_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/on.rc" ] || { echo "  G90 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <label that must FAIL, or ->
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G90 (FAIL|MISSING) +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G90 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G90 FAIL  %-60s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G90 (FAIL|MISSING)" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
fresh; check "P0 the real run set" 0 -
# N1: the flag's run swapped for the control's -- the literal form no longer explains it.
fresh; for f in "$T"/f/off/trace_*; do cp "$f" "$T/f/on/"; done; check "N1 a run that used the bounded term offered as the literal one" 1 ON
# N2: the control swapped for the flag's run -- the bounded form no longer explains it.
fresh; for f in "$REAL"/on/trace_*; do cp "$f" "$T/f/off/"; done; check "N2 a run that used the literal term offered as the control" 1 OFF
# N3: a pcs column shifted by 0.5 on one row of the flag's run.
fresh; python3 - "$T/f/on" <<'PY'
import csv, glob, sys
p = glob.glob(sys.argv[1] + "/trace_ca_tbl_full_seed*.csv")[0]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
r = next(r for r in rows if r["hadFix"] == "1" and float(r["gdop"]) != 0.0)
r["pcs"] = "%.4f" % (float(r["pcs"]) + 1.5)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N3 a score the literal term does not give" 1 ON
fresh; echo 0 > "$T/f/r_v2.rc"; check "N4 the flag accepted with --pcsForm=v2" 1 R
fresh; echo 134 > "$T/f/on.rc"; check "N5 a run did not exit 0" 1 RUNS
printf '  G90 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
