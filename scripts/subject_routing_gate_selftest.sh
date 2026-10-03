#!/usr/bin/env bash
# G59 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/subject_routing_gate_selftest.sh [<dir written by subject_routing_gate.sh>]
#   Without a directory it makes its own real run set first (about 2 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N1: a subject that sent a
# routed frame on a tier it did not range on that cycle -- the sect 6 breach the control
# commits 671 times.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/subject_routing_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g59self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/subject_routing_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/fix_routing_ca_tbl_full_seed7_tier1.csv" ] || { echo "  G59 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G59 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G59 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G59 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G59 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G59 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <csv> <python body over rows (list of dicts); extra csv paths in argv[3:]>
edit() {
    python3 - "$@" <<'PY'
import csv, glob, sys
p, body = sys.argv[1:3]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
exec(body)
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
}
F1="$T/f/on/fix_routing_ca_tbl_full_seed7_tier1.csv"

fresh
check "P0 the real run set" 0 -

# N1: the subject behind a frame on 2.4 GHz is recorded as on 868 MHz that cycle.
fresh
python3 - "$F1" "$T/f/on" <<'PY'
import csv, glob, sys
f1, on = sys.argv[1:3]
r = next(r for r in csv.DictReader(open(f1)) if r["outcome"] in ("forwarded", "delivered", "lost") and 2 <= int(r["senderId"]) <= 5)
p = glob.glob(f"{on}/subjtrace_*_subj{r['senderId']}.csv")[0]
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
for x in rows:
    if x["t"] == r["t"]:
        x["activeTier"] = "868MHz"
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N1 a subject's routed frame on a tier it did not range on" 1 SIX
# N2: a forward redirected to a subject that ranged on another tier that cycle.
fresh; edit "$F1" '
on = sys.argv[1].rsplit("/", 1)[0]
elsewhere = {}
for f in glob.glob(on + "/subjtrace_*_subj*.csv"):
    for x in csv.DictReader(open(f)):
        if x["activeTier"] != "2.4GHz":
            elsewhere.setdefault(x["t"], []).append(x["subjectId"])
r = next(r for r in rows if r["outcome"] == "forwarded" and any(s not in (r["senderId"], r["originId"]) for s in elsewhere.get(r["t"], [])))
r["chosenNext"] = next(s for s in elsewhere[r["t"]] if s not in (r["senderId"], r["originId"]))'
check "N2 a frame forwarded to a node that was on another tier" 1 SIX
fresh; edit "$F1" '
r = dict(next(r for r in rows if r["outcome"] == "offtier")); r["chosenNext"] = "6"; rows.append(r)'
check "N3 a hop refused as off-tier whose next hop was on the tier" 1 OFFTIER
fresh; edit "$F1" '
i = next(i for i, r in enumerate(rows) if r["outcome"] == "offtier")
x = dict(rows[i]); x["hop"] = str(int(x["hop"]) + 1); x["outcome"] = "noroute"; rows.insert(i + 1, x)'
check "N4 a frame carried on past an off-tier refusal" 1 OFFTIER
fresh
for f in "$T"/f/on/fix_routing_ca_tbl_full_seed7_tier*.csv; do
    edit "$f" 'rows[:] = [r for r in rows if not (r["outcome"] == "delivered" and 2 <= int(r["originId"]) <= 5)]'
done
check "N5 no subject 2..5 ever has a fix delivered" 1 SUBJ
fresh; edit "$F1" 'r = next(r for r in rows if r["confNext"] != "0"); r["confCost"] = "%.6f" % (float(r["confCost"]) + 1.0)'
check "N6 a confidence cost the weights do not give" 1 COST
fresh; sed -i -E 's/NW-2 weights alpha 1 beta 0 /NW-2 weights alpha 1 beta 1 /' "$T/f/w.log"
check "N7 a variance weight the rows were not scored with" 1 COST
fresh; sed -i -E '0,/fixes originated [0-9]+/s//fixes originated 99999/' "$T/f/on.log"
check "N8 a router block the rows do not support" 1 COUNTS
fresh; cp "$F1" "$T/f/off/"
check "N9 the control writing a tier router's file" 1 CONTROL
fresh; echo 0 > "$T/f/r_w.rc"
check "N10 a route weight accepted without --fixRouting" 1 R
fresh; echo 134 > "$T/f/w.rc"
check "N11 a run the checks read did not exit 0" 1 RUNS

printf '  G59 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
