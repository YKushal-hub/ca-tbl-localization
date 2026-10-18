#!/usr/bin/env bash
# G120 negative fixtures (rule 6). Each is a copy of a real run set with ONE thing changed.
# usage: bash scripts/tof_field_gate_selftest.sh [<dir written by tof_field_gate.sh>]
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/tof_field_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g120self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/tof_field_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/base.rc" ] || { echo "  G120 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <label that must FAIL, or ->
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1; local rc=$?; local ok=1
    if [ "$3" != - ]; then grep -Eq "G120 (FAIL|MISSING) +$3 " "$T/out.txt" || ok=0; fi
    if [ "$rc" -eq "$2" ] && [ "$ok" -eq 1 ]; then
        printf '  G120 ok    %-64s exit=%d\n' "$1" "$rc"; pass=$((pass + 1))
    else
        printf '  G120 FAIL  %-64s exit=%d expected %d (%s)\n' "$1" "$rc" "$2" "$3"; grep -E "G120 (FAIL|MISSING)" "$T/out.txt" | head -3; fail=$((fail + 1))
    fi
}
edit_trace() {  # edit_trace <run> <python expression on the list c of row 5's cells>
    python3 - "$T/f/$1" "$2" <<'PY'
import glob, sys
p = sorted(glob.glob(sys.argv[1] + "/trace_*.csv"))[0]
L = open(p).read().splitlines()
c = L[5].split(",")
exec(sys.argv[2])
L[5] = ",".join(c)
open(p, "w").write("\n".join(L) + "\n")
PY
}
fresh; check "P0 the real run set" 0 -
fresh; edit_trace cov 'c[11] = "%.4f" % (float(c[11]) + 1.0)'
check "N1 the covariance run changed a pre-existing column (errM)" 1 COV
fresh; edit_trace cov 'c[-3] = "%.12f" % (float(c[-3]) + 1.0)'
check "N2 a pxx that is not the filter's (pxx+pyy != traceSigma)" 1 TRACE
fresh; for f in "$REAL"/base/trace_*; do cp "$f" "$T/f/cov/"; done
check "N3 the covariance flag on, but no columns written" 1 COV
fresh; for f in "$REAL"/cov/trace_*; do cp "$f" "$T/f/base/"; done
check "N4 a control that already carries the columns" 1 REACH
fresh; for f in "$REAL"/base/trace_*; do cp "$f" "$T/f/sig/"; done
check "N5 the sigma floor flag that moved nothing" 1 SIGMA
fresh; cp "$REAL/probe_off/probe_range_sweep.csv" "$T/f/probe_bias/"
check "N6 the bias flag that moved nothing" 1 BIAS
fresh; python3 - "$T/f/probe_bias/probe_range_sweep.csv" <<'PY'
import csv, sys
p = sys.argv[1]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
r = next(r for r in rows if r["tier"] == "2.4GHz" and float(r["pdr"]) > 0)
r["mean_snr_db"] = "%.2f" % (float(r["mean_snr_db"]) + 0.5)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N7 the bias flag that moved a random draw (SNR changed)" 1 BIAS
fresh; f=$(cd "$T/f/zero" && find . -type f ! -name 'run_id.txt' ! -name 'trace_*' | head -1); echo x >> "$T/f/zero/$f"
check "N8 explicit defaults that change an output ($f)" 1 DEFAULT
fresh; echo 134 > "$T/f/sig.rc"; check "N9 a run did not exit 0" 1 RUNS
printf '  G120 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
