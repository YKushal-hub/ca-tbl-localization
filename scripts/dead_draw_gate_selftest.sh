#!/usr/bin/env bash
# G71 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/dead_draw_gate_selftest.sh [<dir written by dead_draw_gate.sh>]
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N2: a dead node still drawing
# energy after its death -- what this flag exists to stop.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/dead_draw_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g71self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/dead_draw_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on.log" ] || { echo "  G71 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G71 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G71 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G71 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G71 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G71 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
# add <J> to cumJ on the last row of a trace
bump() {  # bump <csv> <J>
    python3 - "$1" "$2" <<'PY'
import csv, sys
p, dj = sys.argv[1], float(sys.argv[2])
rows = list(csv.DictReader(open(p)))
cols = list(csv.reader(open(p)))[0]
rows[-1]["cumJ"] = "%.9f" % (float(rows[-1]["cumJ"]) + dj)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=cols, lineterminator="\n")
w.writeheader()
w.writerows(rows)
PY
}
# flatten cumJ after the death on every trace of a run (the defect no longer visible)
flatten() {  # flatten <run>
    python3 - "$T/f/$1" "$T/f/$1.log" <<'PY'
import csv, glob, os, re, sys
d, log = sys.argv[1], open(sys.argv[2]).read()
dead = {1: float(re.search(r"FAULT sect 49: battery exhausted at t=([0-9.]+) s", log).group(1))}
for m in re.finditer(r"FAULT sect 49 \(subject (\d+)\): battery exhausted at t=([0-9.]+) s", log):
    dead[int(m.group(1))] = float(m.group(2))
for k, td in dead.items():
    f = glob.glob(os.path.join(d, "trace_*.csv" if k == 1 else f"subjtrace_*_subj{k}.csv"))[0]
    rows = list(csv.DictReader(open(f)))
    cols = list(csv.reader(open(f)))[0]
    base = None
    for r in rows:
        if float(r["t"]) <= td + 1e-9:
            base = r["cumJ"]
        elif base is not None:
            r["cumJ"] = base
    w = csv.DictWriter(open(f, "w", newline=""), fieldnames=cols, lineterminator="\n")
    w.writeheader()
    w.writerows(rows)
PY
}

fresh
check "P0 the real run set passes" 0 -
fresh; flatten off
check "N1 the defect not reproduced (no dead node draws by default)" 1 DEFECT
fresh; bump "$(ls "$T"/f/on/trace_*.csv | head -1)" 0.5
check "N2 a dead tag 0 still drawing after its death" 1 FIX
fresh; f=$(ls "$T"/f/on/subjtrace_*_subj*.csv | head -1); bump "$f" 0.5
check "N3 a dead subject still drawing (or a live one counted dead)" 1 FIX
fresh; sed -i -E 's/(radio-cycles unbilled, tag 0 )[0-9]+/\199999/' "$T/f/on.log"
check "N4 an N.20 line the traces do not support" 1 COUNTS
fresh; printf '    N.20 dead radios (ca_tbl_full): radio-cycles unbilled, tag 0 3, subjects 0\n' >> "$T/f/off.log"
check "N5 the control not billing a dead node" 1 CONTROL
fresh; echo 0 > "$T/f/r_norelay.rc"
check "N6 the flag accepted without --silenceStopsRelay" 1 R
fresh; sed -i '/NOTE: --deadBatteryNoDraw unused/d' "$T/f/r_nobat.log"
check "N6b the flag silently idle without a battery budget" 1 R
fresh; sed -i '/FAULT sect 49 (subject/d' "$T/f/on.log"
check "N7 no subject died" 1 DEAD
fresh; echo 134 > "$T/f/on.rc"
check "N8 a run the checks read did not exit 0" 1 RUNS

printf '  G71 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
