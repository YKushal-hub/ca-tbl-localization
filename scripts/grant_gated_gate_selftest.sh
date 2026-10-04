#!/usr/bin/env bash
# G49 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# usage: bash scripts/grant_gated_gate_selftest.sh [<dir written by grant_gated_gate.sh>]
#   Without a directory it makes its own real run set first (about 1 min).
#
# Every fixture is a copy of a REAL run set, outside the tree, with ONE thing changed, and
# must fail the check it names. The one that matters most is N3: a node on UWB with no grant
# behind it -- the behaviour before the flag, and the failure MAC sect 8 exists to prevent.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/grant_gated_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g49self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then
    REAL=$1
else
    REAL="$T/real"
    bash "$ROOT/scripts/grant_gated_gate.sh" "$REAL" > "$T/real.out" 2>&1
fi
[ -f "$REAL/on/grants_ca_tbl_full_seed1.csv" ] || { echo "  G49 selftest: no real run set in $REAL"; exit 2; }

pass=0
fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {  # check <name> <expected exit> <check label that must FAIL, or - >
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$?
    local reason=1
    if [ "$3" != - ]; then
        grep -Eq "G49 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0
    fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then
        printf '  G49 ok    %-66s exit=%d\n' "$1" "$rc"
        [ "$2" -ne 0 ] && grep -m1 -E "G49 (FAIL|MISSING) +$3 " "$T/out.txt" | sed 's/^ */            /' | cut -c1-200
        pass=$((pass + 1))
    else
        printf '  G49 FAIL  %-66s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"
        grep -E "G49 (FAIL|MISSING)" "$T/out.txt" | sed 's/^ */            /' | head -4
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
G="$T/f/on/grants_ca_tbl_full_seed1.csv"
TR="$T/f/on/trace_ca_tbl_full_seed1.csv"

fresh
check "P0 the real run set" 0 -

fresh; edit "$G" 'del rows[next(i for i, r in enumerate(rows) if r["event"] == "confirm")]'
check "N1 a UWB entry with no confirm behind it" 1 GRANTED
fresh; edit "$G" 'next(r for r in rows if r["event"] == "confirm")["anchorId"] = "999"'
check "N2 a confirm no delivered GRANT(granted) supports" 1 GRANTED
fresh; edit "$TR" '
for i in range(1, len(rows) - 1):
    if rows[i - 1]["activeTier"] == "2.4GHz" and rows[i]["activeTier"] == "2.4GHz" and rows[i + 1]["activeTier"] == "2.4GHz":
        rows[i]["activeTier"] = "UWB"
        break'
check "N3 a node on UWB with no grant (the behaviour before the flag)" 1 GRANTED
fresh; edit "$G" 'del rows[next(i for i, r in enumerate(rows) if r["event"] == "request")]'
check "N4 a pending cycle with no request logged" 1 REQUESTS
fresh; python3 - "$T/f/on" <<'PY'
import csv, sys
d = sys.argv[1]
g = [r for r in csv.DictReader(open(d + "/grants_ca_tbl_full_seed1.csv"))]
p = d + "/trace_ca_tbl_full_seed1.csv"
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
ev = sorted((float(r["t"]), r["event"]) for r in g if r["subjectId"] == "1")
done = False
for i, (t0, e) in enumerate(ev):
    if e != "request" or done:
        continue
    end = next((te for te, ee in ev[i + 1:] if ee in ("confirm", "deny", "timeout", "coverage")), 1e18)
    for r in rows:
        if t0 < float(r["t"]) <= end:
            r["deescalated"] = "1"; r["hadFix"] = "1"; r["usedFloor"] = "0"
            done = True
            break
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N5 a healthy demotion while a request is pending" 1 FROZEN
fresh; sed -i 's/(ca_tbl_full), tag 0: requests \([0-9]*\), confirmed \([0-9]*\)/(ca_tbl_full), tag 0: requests \1, confirmed 9\2/' "$T/f/on.log"
check "N6 an NW-7 line the grant log does not support" 1 COUNTS
fresh; printf '100.000000,1,request,0\n' >> "$T/f/on/grants_static_uwb_seed1.csv"
check "N7 a static arm that asked for a grant" 1 STATIC
fresh; python3 - "$T/f/off" <<'PY'
import csv, sys
d = sys.argv[1]
tr = list(csv.DictReader(open(d + "/trace_ca_tbl_full_seed1.csv")))
p = d + "/events_ca_tbl_full_seed1.csv"
rd = csv.DictReader(open(p)); h = rd.fieldnames; rows = list(rd)
for a, b in zip(tr, tr[1:]):
    if b["activeTier"] == "UWB" and a["activeTier"] != "UWB":
        rows.append({"t": "%.4f" % (float(b["t"]) - 0.1), "tier": "1", "txId": "101", "rxId": "1",
                     "msgType": "ESC_GRANT", "outcome": "delivered", "rxPowerDbm": "-90",
                     "payload": "pkt=1;hops=1;acks=1;granted=1;qpos=0"})
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=h, lineterminator="\n"); w.writeheader(); w.writerows(rows)
PY
check "N8 a control in which every UWB entry happens to be granted" 1 CONTROL
fresh; edit "$T/f/subj/grants_ca_tbl_full_seed7.csv" '
rows[:] = [r for r in rows if not (r["subjectId"] != "1" and r["event"] == "confirm")]'
check "N9 no subject other than tag 0 confirmed" 1 SUBJECTS
fresh; edit "$G" 'rows[0]["anchorId"] = "7"'
check "N10 a grant log that is not the file digested" 1 DIGEST
fresh; echo 0 > "$T/f/r_nofix.rc"
check "N11 --grantGatedUwb accepted without --handshakeFixes" 1 R
fresh; echo 134 > "$T/f/on.rc"
check "N12 a run the checks read did not exit 0" 1 RUNS

printf '  G49 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
