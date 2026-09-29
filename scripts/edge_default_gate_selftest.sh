#!/usr/bin/env bash
# G21 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# Every fixture is built outside the tree, in a mktemp directory, from REAL rows: the
# scenario is run twice, short, at defaults and with --gatewayEdgeServer=false, and each
# fixture changes ONE thing in that pair. N0 matters most: the node-side run offered as
# both sides -- the state of the tree before 2026-09-29, and of any later tree where the
# default is reverted or the edge path goes dead -- must fail, because nothing moved.
# N6 is the one only the sect 6 check can see: an unused tier's energy moves on a row
# whose other energy legitimately moved too.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/edge_default_gate.py"
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
D=$(mktemp -d "${TMPDIR:-/tmp}/g21edge.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT

pass=0
fail=0
check() {  # check <name> <expected exit> <defaults csv> <node-side csv>
    python3 "$GATE" "$3" "$4" > "$D/out.txt" 2>&1
    local rc=$?
    if [ "$rc" -eq "$2" ]; then
        printf '  G21 ok    %-60s exit=%d\n' "$1" "$rc"
        # A negative must fail for ITS reason: show it.
        [ "$2" -ne 0 ] && grep -m1 'FAIL' "$D/out.txt" | sed 's/^/            /'
        pass=$((pass + 1))
    else
        printf '  G21 FAIL  %-60s exit=%d expected %d\n' "$1" "$rc" "$2"
        sed 's/^/            /' "$D/out.txt" | head -6
        fail=$((fail + 1))
    fi
}
# mut <in> <out> <arm> <column> <value> -- one cell of one arm's row changed
mut() {
    python3 - "$@" <<'PY'
import csv, sys
src, dst, arm, colname, val = sys.argv[1:6]
rows = list(csv.reader(open(src, newline="")))
i = rows[0].index(colname)
hit = 0
for r in rows[1:]:
    if r and r[0] == arm:
        r[i] = val
        hit += 1
assert hit == 1, f"{arm} rows found: {hit}"
csv.writer(open(dst, "w", newline=""), lineterminator="\n").writerows(rows)
PY
}

if [ ! -x "$BIN" ]; then
    printf '  G21 ERROR no binary at %s\n' "$BIN"
    exit 1
fi
mkdir -p "$D/def" "$D/off"
"$BIN" --seed=7 --duration=300 --trace=false --outdir="$D/def" > "$D/def.log" 2>&1 \
    || { printf '  G21 ERROR defaults run exited non-zero\n'; exit 1; }
"$BIN" --seed=7 --duration=300 --trace=false --gatewayEdgeServer=false --outdir="$D/off" \
    > "$D/off.log" 2>&1 || { printf '  G21 ERROR node-side run exited non-zero\n'; exit 1; }
DEF="$D/def/arm_summary.csv"
OFF="$D/off/arm_summary.csv"

# P0 -- positive control: the real pair.
check "P0 defaults vs --gatewayEdgeServer=false, real rows" 0 "$DEF" "$OFF"

# N0 -- the flag off on both sides: no row moved, so the edge path never ran.
check "N0 node-side run offered as both sides (edge path dead)" 1 "$OFF" "$OFF"

# N1 -- header only.
head -1 "$DEF" > "$D/n1.csv"
check "N1 header only, zero rows" 1 "$D/n1.csv" "$D/n1.csv"

# N2 -- an accuracy column moved.
mut "$DEF" "$D/n2.csv" ca_tbl_full rmseM 999.0000
check "N2 rmseM moved on ca_tbl_full" 1 "$D/n2.csv" "$OFF"

# N3 -- a transmission appeared: tx_ is energy, and is still not movable.
mut "$DEF" "$D/n3.csv" static_868 tx_868 99.000000000
check "N3 tx_868 moved on static_868" 1 "$D/n3.csv" "$OFF"

# N4 -- energy moved on a row with no fix (static_uwb never fixes).
mut "$DEF" "$D/n4.csv" static_uwb totalJ 1.000000000
check "N4 totalJ moved on static_uwb, fixCycles 0" 1 "$D/n4.csv" "$OFF"

# N5 -- a row missing.
grep -v '^static_2p4,' "$DEF" > "$D/n5.csv"
check "N5 static_2p4 row dropped" 1 "$D/n5.csv" "$OFF"

# N6 -- an unused tier charged, on a row whose tier-1 energy legitimately moved.
mut "$DEF" "$D/n6.csv" static_868 j_uwb 5.000000000
check "N6 j_uwb moved on static_868, cycles_uwb 0 (sect 6)" 1 "$D/n6.csv" "$OFF"

# N7 -- the same rows in a different order: not the same arm on the same line.
{ head -1 "$DEF"; tail -n +2 "$DEF" | tac; } > "$D/n7.csv"
check "N7 rows reordered" 1 "$D/n7.csv" "$OFF"

printf '  G21 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
