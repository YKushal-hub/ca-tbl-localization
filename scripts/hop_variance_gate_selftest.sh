#!/usr/bin/env bash
# G58 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/hop_variance_gate_selftest.sh [<dir written by hop_variance_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most are N2 and N4: a leg that kept
# the engine's sigma although it had a variance term, and an NLOS leg left uninflated --
# the two terms the flag exists to add.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/hop_variance_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g58self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/hop_variance_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/relay_chain_static_2p4_seed7.csv" ] || { echo "  G58 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G58 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G58 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G58 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G58 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G58 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv> <python body over rows (list of dicts)>; legs(rows) are the decision rows with terms
edit() {
    python3 - "$@" <<'PY'
import csv, math, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
def legs():
    return [r for r in rows if r["leg"] in ("relay", "tag") and r["verdict"] != "COMPOSE_REFUSED" and r["tier"] in ("1", "2")]
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}
C="$T/f/on/relay_chain_static_2p4_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$C" 'r = next(r for r in legs() if int(r["legN"]) >= 3); r["legSDb"] = "%.6f" % (float(r["legSDb"]) + 0.5)'
check "N1 a variability the leg's readings do not give" 1 VAR
fresh; edit "$C" '
r = next(r for r in legs() if float(r["legVarSigmaM"]) > 0 and r["legNlos"] == "0")
r["legVarSigmaM"] = "0.000000"; r["sigmaLegM"] = r["sigmaRawM"]'
check "N2 a leg that kept the engine's sigma although it had a variance term" 1 CONV
fresh; edit "$C" 'r = next(r for r in legs() if float(r["legScore"]) >= 0 and r["legNlos"] == "0"); r["legNlos"] = "1"'
check "N3 an NLOS verdict the score does not give" 1 NLOS
fresh; edit "$C" '
r = next(r for r in legs() if r["legNlos"] == "1")
r["sigmaLegM"] = "%.6f" % math.hypot(float(r["sigmaRawM"]), float(r["legVarSigmaM"]))'
check "N4 an NLOS leg left uninflated" 1 SIGMA
fresh; edit "$C" 'r = next(r for r in legs() if r["verdict"] == "ADMIT"); r["sigmaEffM"] = "%.6f" % (float(r["sigmaLegM"]) * 0.5)'
check "N5 sigma_eff composed from something smaller than the leg" 1 EFF
fresh
for f in "$T"/f/on/relay_chain_*_seed7.csv; do
    edit "$f" '
for r in legs():
    if r["legNlos"] == "1":
        r["legNlos"] = "0"; r["legScore"] = "0.000000"
        r["sigmaLegM"] = "%.6f" % math.hypot(float(r["sigmaRawM"]), float(r["legVarSigmaM"]))'
done
sed -i -E 's/inflated [0-9]+ \(x/inflated 0 (x/' "$T/f/on.log"
check "N6 no leg ever inflated as NLOS (the term never fires)" 1 REACH
fresh; sed -i -E '0,/NW-1 hop variance terms \(static_2p4\): legs [0-9]+/s//NW-1 hop variance terms (static_2p4): legs 9999/' "$T/f/on.log"
check "N7 an NW-1 line the rows do not support" 1 COUNTS
fresh; cp "$T/f/on/relay_chain_static_2p4_seed7.csv" "$T/f/off/relay_chain_static_2p4_seed7.csv"
check "N8 the control carrying the leg columns" 1 CONTROL
fresh; echo 0 > "$T/f/r_noclass.rc"
check "N9 --hopVarianceTerms accepted without --nlosClassifier" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS

printf '  G58 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
