#!/usr/bin/env bash
# G112 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/acc_b_gate_selftest.sh [<dir written by acc_b_gate.sh>]
#   Without a directory it makes its own real run set first.
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/acc_b_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g112self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/acc_b_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/a_lone.rc" ] || { echo "  G112 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G112 FAIL +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G112 ok    %-66s exit=%d\n' "$1" "$rc"
        pass=$((pass + 1))
    else
        printf '  G112 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G112 (FAIL|MISSING)" "$T/out.txt" | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv> <python body over rows (list of dicts)>
edit() {
    python3 - "$@" <<'PY'
import csv, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}
TR=trace_ca_tbl_full_seed1.csv
RC=relay_chain_ca_tbl_full_seed1.csv
CD=cased_ca_tbl_full_seed1.csv

fresh
check "P0  the real run set passes" 0 -

fresh
echo 1 > "$T/f/scale.rc"
check "N1  a run exited non-zero" 1 RC

fresh
edit "$T/f/off/$TR" 'rows[5]["errM"] = "%.4f" % (float(rows[5]["errM"]) + 0.0001)'
check "N2  flags off differ from absent in one trace value" 1 NULL

fresh
rm "$T/f/off/$CD"
check "N3  flags off lose an output file" 1 NULL

fresh
edit "$T/f/relay/$RC" 'for r in rows:
    r["tier"] = "0" if r["leg"] == "tag" else r["tier"]'
check "N4  the relay flag forms no 2.4 GHz leg on an 868 MHz cycle" 1 RELAY

fresh
python3 - "$T/f/base" <<'PY'
import csv, sys
d = sys.argv[1]
tr = list(csv.DictReader(open(f"{d}/trace_ca_tbl_full_seed1.csv")))
t868 = next(r["t"] for r in tr if r["activeTier"] == "868MHz")
rd = csv.DictReader(open(f"{d}/relay_chain_ca_tbl_full_seed1.csv")); h = rd.fieldnames; rows = list(rd)
x = dict(rows[0]); x.update(t=t868, leg="tag", tier="1"); rows.append(x)
w = csv.DictWriter(open(f"{d}/relay_chain_ca_tbl_full_seed1.csv", "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N5  flags absent, a 2.4 GHz tag leg on an 868 MHz cycle" 1 RELAY

fresh
edit "$T/f/scale/$RC" 'first = next(r for r in rows if r["leg"] == "tag")
first["carriedVarM2"] = "%.6f" % (float(first["carriedVarM2"]) / 2.0)'
check "N6  the covariance scale applied by 2, not 4" 1 SCALE

fresh
cp "$T/f/a_base/$CD" "$T/f/a_lone/$CD"
cp "$T/f/a_base/$TR" "$T/f/a_lone/$TR"
check "N7  under the lone-868 flag a Case D row on an 868 MHz cycle" 1 LONE

fresh
edit "$T/f/a_lone/$TR" 'for r in rows:
    r["nRangesEkf"] = "0" if r["fixTier"] == "868MHz" and r["nRangesEkf"] == "1" else r["nRangesEkf"]'
check "N8  the zero is an absence: no one-range 868 MHz cycle exists" 1 LONE

fresh
edit "$T/f/a_base/$CD" 'del rows[:]'
check "N9  flags absent, no lone 868 MHz Case D row to suppress" 1 LONE

fresh
edit "$T/f/a_info/$CD" 'for r in rows:
    r["applied"] = "1" if float(r["nis"]) <= 6.63 else r["applied"]'
check "N10 under the ratio no NIS-passing row refused (path not reached)" 1 INFO

fresh
edit "$T/f/a_base/$CD" 'rows[0]["applied"] = "0"; rows[0]["nis"] = "1.0"'
check "N11 flags absent, a NIS-passing row refused" 1 INFO

fresh
sed -i 's/(sigma ratio > 3): [0-9]*/(sigma ratio > 3): 999/' "$T/f/a_info.log"
check "N12 the N.38 stdout count disagrees with the CSV" 1 INFO

fresh
edit "$T/f/a_info/$CD" 'r = next(r for r in rows if r["applied"] == "1"); r["nis"] = "7.5"'
check "N13 under the ratio a row applied with NIS above 6.63" 1 INFO

printf '  G112 selftest: %d ok, %d FAIL\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
