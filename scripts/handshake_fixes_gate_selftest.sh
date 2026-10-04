#!/usr/bin/env bash
# G33 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/handshake_fixes_gate_selftest.sh [<dir written by handshake_fixes_gate.sh>]
#   Without a directory it makes its own real run set first (about 25 s).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The ones that matter most:
#   N2  a request answered by a bare ACK under the flag -- the arbitrate-after-ACK defect;
#   N3  an original request on a de-escalation cycle under the flag;
#   N5  the previous cycle's T_deg on a request under the flag.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/handshake_fixes_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g33self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/handshake_fixes_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/fix.log" ] || { echo "  G33 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G33 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G33 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G33 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G33 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G33 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
        fail=$((fail + 1))
    fi
}
# edit <events csv> <python body over rows (list of lists, header first)>
edit() {
    python3 - "$@" <<'PY'
import csv, sys
p, body = sys.argv[1:3]
rows = list(csv.reader(open(p)))
exec(body)
csv.writer(open(p, "w", newline=""), lineterminator="\n").writerows(rows)
PY
}
EF="$T/f/fix/events_ca_tbl_full_seed1.csv"
ED="$T/f/def/events_ca_tbl_full_seed1.csv"

fresh
check "P0 the real run set" 0 -

fresh; sed -i '2s/,/;/' "$T/f/off/arm_summary.csv"
check "N0 --handshakeFixes=false moves an output" 1 NULL
fresh; grep "MC-1" "$T/f/fix.log" >> "$T/f/def.log"
check "N1 the default prints the MC-1 line" 1 NULL
# N2: a node-1 request answered by an anchor's bare ACK naming the request itself.
fresh; edit "$EF" '
r = next(r for r in rows[1:] if r[4] == "ESC_REQUEST" and r[2] == "1" and 101 <= int(r[3]) <= 199)
pkt = r[7].split(";")[0].split("=")[1]
rows.append([str(float(r[0]) + 0.01), r[1], r[3], "1", "ACK", "delivered", "-90", "pkt=999;hops=1;acks=" + pkt])'
check "N2 a request answered by a bare ACK under the flag" 1 NOACK
# N3: an original node-1 request on a de-escalation cycle.
fresh; edit "$EF" '
import csv as c
tr = list(c.DictReader(open(sys.argv[1].replace("events_", "trace_"))))
up = {float(x["t"]) for x in tr if x["escalated"] == "1" and x["nextTier"] == "UWB"}
t0 = next(float(x["t"]) for x in tr if x["deescalated"] == "1" and float(x["t"]) - 2.5 not in up
          and all(abs(float(x["t"]) - u) > 7.5 for u in up))
rows.append([str(t0 + 0.01), "1", "1", "501", "ESC_REQUEST", "delivered", "-90", "pkt=1;hops=1;degr=0;batt=0;target=2"])'
check "N3 a request on a de-escalation cycle under the flag" 1 ONLYUWB
fresh; edit "$EF" '
for r in rows[1:]:
    if r[4] == "ESC_REQUEST" and r[2] == "1":
        r[7] = r[7].replace(";target=2", "")
        break'
check "N4 a node-1 request that does not say it wants UWB" 1 TARGET
# N5: the default's stale T_deg on every fix request (only where the two differ matters).
fresh; PYTHONDONTWRITEBYTECODE=1 python3 - "$EF" "$T/f/fix/trace_ca_tbl_full_seed1.csv" <<'PY'
import csv, math, re, sys
ev, trp = sys.argv[1:3]
sys.path.insert(0, "scripts")
import handshake_fixes_gate as g
tr = list(csv.DictReader(open(trp)))
t = [float(r["t"]) for r in tr]
tm = g.terms(tr)
rows = list(csv.reader(open(ev)))
n = 0
for r in rows[1:]:
    if r[4] == "ESC_REQUEST" and r[2] == "1":
        k = g.row_of(t, float(r[0]))
        if k is not None and abs(tm[k][0] - tm[k][1]) > 1:
            r[7] = re.sub(r"degr=\d+", "degr=%d" % tm[k][1], r[7])
            n += 1
assert n
csv.writer(open(ev, "w", newline=""), lineterminator="\n").writerows(rows)
PY
check "N5 the previous cycle's T_deg under the flag" 1 TERMS
fresh; edit "$EF" '
for r in rows[1:]:
    if r[4] == "ESC_GRANT":
        r[7] = r[7].replace("acks=", "acks=7")'
check "N6 a GRANT naming a request nobody sent" 1 GRANT
fresh; edit "$EF" '
for r in rows[1:]:
    if r[4] == "ESC_GRANT":
        r[7] = r[7].replace("granted=0", "granted=1")'
check "N7 no denial ever answered (the default's silence)" 1 GRANT
fresh; edit "$ED" '
rows[:] = [rows[0]] + [r for r in rows[1:] if not (r[4] == "ACK" and 101 <= int(r[2]) <= 199)]'
check "N8 the default with no bare ACK to show" 1 DEFECT
fresh; echo 0 > "$T/f/r_noframes.rc"
check "N9 the flag accepted without handshake frames" 1 R
fresh; echo 134 > "$T/f/fix.rc"
check "N10 a run the checks read did not exit 0" 1 RUNS

printf '  G33 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
