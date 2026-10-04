#!/usr/bin/env bash
# G50 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/query_v2_gate_selftest.sh [<dir written by query_v2_gate.sh>]
#   Without a directory it makes its own real run set first (about 30 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most: N5, a node that answered every
# listed manifest relaying the query anyway (the behaviour before the flag), and N6, a
# blanket relay of the whole list after answering part of it.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/query_v2_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g50self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/query_v2_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/v2/responses_ca_tbl_full_seed7.csv" ] || { echo "  G50 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G50 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G50 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G50 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G50 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G50 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
RS="$T/f/v2/responses_ca_tbl_full_seed7.csv"
QL="$T/f/v2/queryv2_ca_tbl_full_seed7.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$T/f/v1/events_ca_tbl_full_seed7.csv" '
r = next(r for r in rows if r["msgType"] == "QUERY"); r["payload"] += ";q2=1;prio=0;list=1"'
check "N0 a frame with v2 fields without the flag" 1 LEGACY
fresh; edit "$RS" 'r = rows[0]; r["entryNode"] = "5" if r["manifest"] != "5" else "4"'
check "N1 an answer whose entry is not the named subject" 1 CONTENT
fresh; edit "$RS" 'rows[1]["x"] = "%.4f" % (float(rows[1]["x"]) + 10.0)'
check "N2 an entry that is not the subject's published estimate" 1 CONTENT
fresh; edit "$RS" '
r = next(r for r in rows if 10 <= int(r["pcsQ"]) <= 240 or r["pcsQ"] == "0"); r["pcsQ"] = str(int(r["pcsQ"]) + 5)'
check "N3 a PCS the subject did not publish" 1 CONTENT
fresh; edit "$RS" '
r = next(r for r in rows if r["entryNode"] == "1"); r["integrity"] = "1" if r["integrity"] == "0" else "0"'
check "N4 tag 0's integrity verdict misreported" 1 CONTENT
fresh; edit "$QL" 'r = next(r for r in rows if r["what"] == "suppress"); r["what"] = "relay"'
check "N5 a node that answered the whole list relays it anyway" 1 LIST
fresh; python3 - "$T/f/v2" <<'PY'
import csv, sys
d = sys.argv[1]
ev = list(csv.DictReader(open(d + "/events_ca_tbl_full_seed7.csv")))
first = {}
for r in ev:
    p = dict(kv.split("=", 1) for kv in r["payload"].split(";") if "=" in kv)
    if r["msgType"] == "QUERY" and "q2" in p and r["outcome"] in ("delivered", "capture"):
        first.setdefault((r["rxId"], r["tier"], p["q2"]), p["list"])
q = d + "/queryv2_ca_tbl_full_seed7.csv"
rd = csv.DictReader(open(q)); h = rd.fieldnames; rows = list(rd)
answered = {(r["node"], r["tier"], r["queryId"]) for r in rows if r["what"] in ("answer", "proxy")}
for r in rows:
    k = (r["node"], r["tier"], r["queryId"])
    if r["what"] == "relay" and k in answered and k in first:
        r["left"] = first[k]  # the whole list, as if nothing had been answered
        break
w = csv.DictWriter(open(q, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N6 a blanket relay of the whole list after answering part of it" 1 LIST
fresh; sed -i 's/answered as target \([0-9]*\),/answered as target 9\1,/' "$T/f/v2.log"
check "N7 an NW-9 line the logs do not support" 1 COUNTS
fresh; edit "$RS" 'rows[0]["seq"] = str(int(rows[0]["seq"]) + 1)'
check "N8 a response log that is not the file digested" 1 DIGEST
fresh; echo 0 > "$T/f/r_noq.rc"
check "N9 --queryV2 accepted without queries" 1 R
fresh; echo 134 > "$T/f/v2.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS

printf '  G50 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
