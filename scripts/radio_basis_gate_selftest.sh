#!/usr/bin/env bash
# G61 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/radio_basis_gate_selftest.sh [<dir written by radio_basis_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N3 and N5: an account left on
# the SX currents (a tier, or a subject, the basis never reached) -- the failure a basis
# applied in one place and not another would leave on disk.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/radio_basis_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g61self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/radio_basis_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/lr/arm_summary.csv" ] || { echo "  G61 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G61 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G61 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G61 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G61 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G61 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -5
        fail=$((fail + 1))
    fi
}
# edit <csv> <python statement over rows (list of dicts)>: rewrite a CSV in place
edit() {
    python3 - "$1" "$2" <<'PY'
import csv, sys
p, stmt = sys.argv[1], sys.argv[2]
rows = list(csv.DictReader(open(p)))
cols = list(rows[0].keys())
exec(stmt)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=cols, lineterminator="\n")
w.writeheader()
w.writerows(rows)
PY
}
A="$T/f/lr/arm_summary.csv"
S="$T/f/sx/arm_summary.csv"

fresh
check "P0 the real run set passes" 0 -
fresh; edit "$A" 'rows[1]["fixCycles"] = str(int(rows[1]["fixCycles"]) + 1)'
check "N1 a non-energy column moved with the basis" 1 SAME
fresh; f=$(ls "$T"/f/lr/trace_*.csv | head -1); edit "$f" 'rows[5]["errM"] = "999.0"'
check "N2 a trace's error moved with the basis" 1 SAME
fresh; python3 - "$S" "$A" <<'PY'
import csv, sys
sx = list(csv.DictReader(open(sys.argv[1])))
rows = list(csv.DictReader(open(sys.argv[2])))
for a, b in zip(sx, rows):
    b["tx_868"] = a["tx_868"]
w = csv.DictWriter(open(sys.argv[2], "w", newline=""), fieldnames=list(rows[0].keys()), lineterminator="\n")
w.writeheader(); w.writerows(rows)
PY
check "N3 868 MHz TX left on the SX current" 1 RATIO
fresh; edit "$A" 'r = next(r for r in rows if float(r["rx_uwb"]) > 0); r["rx_uwb"] = "%.9f" % (float(r["rx_uwb"]) * 1.01)'
check "N4 the UWB account moved" 1 RATIO
fresh; python3 - "$T/f/sub_sx/subject_summary.csv" "$T/f/sub_lr/subject_summary.csv" <<'PY'
import csv, sys
sx = list(csv.DictReader(open(sys.argv[1])))
rows = list(csv.DictReader(open(sys.argv[2])))
for a, b in zip(sx, rows):
    if b["subjectId"] == "3":
        for c in ("tx_868", "sleep_868"):
            b[c] = a[c]
w = csv.DictWriter(open(sys.argv[2], "w", newline=""), fieldnames=list(rows[0].keys()), lineterminator="\n")
w.writeheader(); w.writerows(rows)
PY
check "N5 a subject never given the basis" 1 SUBJ
fresh; python3 - "$T/f/gos_sx.log" "$T/f/gos_lr.log" <<'PY'
import re, sys
sx = re.findall(r"gossip energy \(tier 1, gateway account\): tx (\S+) J", open(sys.argv[1]).read())
t = open(sys.argv[2]).read()
t = re.sub(r"(gossip energy \(tier 1, gateway account\): tx )(\S+)( J)", lambda m: m.group(1) + sx[0] + m.group(3), t, count=1)
open(sys.argv[2], "w").write(t)
PY
check "N6 a gateway's gossip account left on the SX current" 1 GOSSIP
fresh; grep -m1 'SY-2 radio basis' "$T/f/lr.log" >> "$T/f/sx.log"
check "N7 the SY-2 line printed without the basis" 1 LINE
fresh; sed -i '/^radioBasis=/d' "$T/f/pre/spec_profile.txt"
check "N8 the preset not setting the basis" 1 PRESET
fresh; echo 0 > "$T/f/r_bad.rc"
check "N9 an unknown basis accepted" 1 R
fresh; echo 134 > "$T/f/lr.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS
fresh; edit "$A" 'rows[0]["totalJ"] = "%.9f" % (float(rows[0]["totalJ"]) + 0.001)'
check "N11 totalJ not the sum of its tiers" 1 RATIO

printf '  G61 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
