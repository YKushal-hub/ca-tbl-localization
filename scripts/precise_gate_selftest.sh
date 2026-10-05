#!/usr/bin/env bash
# G55 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/precise_gate_selftest.sh [<dir written by precise_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N3: a target stamped precise by
# a UWB fix without a grant -- the rule before the flag.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/precise_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g55self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/precise_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/grant/session_ca_tbl_full_seed7.csv" ] || { echo "  G55 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G55 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G55 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G55 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G55 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G55 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
S="$T/f/grant/session_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$S" '
r = next(r for r in rows if r["stateAfter"] == "LOCATED_PRECISE" and r["stateBefore"] != "LOCATED_PRECISE")
r["grantedBy"] = "0"; r["uwbActive"] = "0"'
check "N1 the session went precise on a result with no grant" 1 GRANT
fresh; sed -i 's/ m1 \([0-9.]*\)\/\([0-9.]*\)/ m1 \1\/9\2/' "$T/f/grant.log"
check "N2 a first-precise time that is not the first granted result" 1 GRANT
fresh; python3 - "$T/f" <<'PY'
import csv, re, sys
d = sys.argv[1]
p = d + "/grant/session_ca_tbl_full_seed7.csv"
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
r = next(r for r in rows if r["event"] == "result" and r["manifest"] == "1" and r["suspect"] == "0")
r["tierSource"] = "2"; r["grantedBy"] = "0"; r["uwbActive"] = "0"
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
lp = d + "/grant.log"
s = open(lp).read()
s = re.sub(r" m1 ([0-9.]+)/([0-9.]+)", lambda m: " m1 %s/%.1f" % (m.group(1), float(r["t"])), s, count=1)
open(lp, "w").write(s)
PY
check "N3 a target stamped precise by a UWB fix without a grant (the old rule)" 1 NOTIER
fresh; sed -i 's/batch precision spread \([0-9.]*\) s/batch precision spread 1\1 s/' "$T/f/grant.log"
check "N4 a batch spread that is not max - min" 1 SPREAD
fresh; sed -i '0,/ m1 \([0-9.]*\)\/-1.0/s// m1 \1\/85.4/' "$T/f/tier.log"
check "N5 the default rule claiming a precise time no UWB fix gave" 1 CONTROL
fresh; echo 0 > "$T/f/r_nogate.rc"
check "N6 --sessionPreciseOnGrant accepted without --grantGatedUwb" 1 R
fresh; echo 134 > "$T/f/grant.rc"
check "N7 a run the checks read did not exit 0" 1 RUNS

printf '  G55 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
