#!/usr/bin/env bash
# G21 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# Every fixture is built outside the tree, in a mktemp directory, from REAL rows: the
# scenario is run three times, short, at defaults, with --gatewayEdgeServer=false and with
# --edgeDownlinkTier=active, and each fixture changes ONE thing. N0 matters most: the
# node-side run offered as both sides -- the state of the tree before 2026-09-29, and of
# any later tree where the default is reverted or the edge path goes dead -- must fail,
# because nothing moved. N6 is the one only the sect 6 check can see: an unused tier's
# energy moves on a row whose other energy legitimately moved too.
#
# P1 and N8-N14 are the downlink-tier checks (TECH_DEBT M.10, 2026-09-30). N8 is their N0:
# the active-tier run offered as the defaults -- the tree before 2026-09-30, or a later one
# where that default reverted -- must fail, because no downlink moved. N11 is the one only
# the sect 6 check for the move can see.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/edge_default_gate.py"
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
D=$(mktemp -d "${TMPDIR:-/tmp}/g21edge.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT

pass=0
fail=0
check() {  # check <name> <expected exit> <defaults csv> <node-side csv> [--active <csv>]
    python3 "$GATE" "${@:3}" > "$D/out.txt" 2>&1
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
# bump <in> <out> <arm> <column> <delta> -- one numeric cell of one arm's row moved by delta
bump() {
    python3 - "$@" <<'PY'
import csv, sys
src, dst, arm, colname, delta = sys.argv[1:6]
rows = list(csv.reader(open(src, newline="")))
i = rows[0].index(colname)
hit = 0
for r in rows[1:]:
    if r and r[0] == arm:
        r[i] = f"{float(r[i]) + float(delta):.9f}"
        hit += 1
assert hit == 1, f"{arm} rows found: {hit}"
csv.writer(open(dst, "w", newline=""), lineterminator="\n").writerows(rows)
PY
}

if [ ! -x "$BIN" ]; then
    printf '  G21 ERROR no binary at %s\n' "$BIN"
    exit 1
fi
mkdir -p "$D/def" "$D/off" "$D/act"
"$BIN" --seed=7 --duration=300 --trace=false --outdir="$D/def" > "$D/def.log" 2>&1 \
    || { printf '  G21 ERROR defaults run exited non-zero\n'; exit 1; }
"$BIN" --seed=7 --duration=300 --trace=false --gatewayEdgeServer=false --outdir="$D/off" \
    > "$D/off.log" 2>&1 || { printf '  G21 ERROR node-side run exited non-zero\n'; exit 1; }
"$BIN" --seed=7 --duration=300 --trace=false --edgeDownlinkTier=active --outdir="$D/act" \
    > "$D/act.log" 2>&1 || { printf '  G21 ERROR active-tier run exited non-zero\n'; exit 1; }
DEF="$D/def/arm_summary.csv"
OFF="$D/off/arm_summary.csv"
ACT="$D/act/arm_summary.csv"

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

# ---- M.10: the downlink billed on the fix's tier (--active). Every fixture below keeps
# the real defaults/node-side pair, so checks 1-4 pass and only checks 5-9 can refuse it.
# The per-downlink costs used to shape N11/N12 are the ones measured at the change
# (0.000904183 J on 868 MHz, 0.000200508 J on 2.4 GHz); a fixture only needs their shape.

# P1 -- positive control: the real triple.
check "P1 defaults vs node-side vs --edgeDownlinkTier=active, real rows" 0 \
    "$DEF" "$OFF" --active "$ACT"

# N8 -- the default reverted to the active tier: the active-tier run offered as the
# defaults. Checks 1-4 still pass (the edge path is live); nothing moved against --active.
check "N8 active-tier run offered as the defaults (reverted default)" 1 \
    "$ACT" "$OFF" --active "$ACT"

# N9 -- the bill moved an accuracy column.
mut "$ACT" "$D/n9.csv" ca_tbl_full cep95M 999.0000
check "N9 cep95M differs between fix and active bills on ca_tbl_full" 1 \
    "$DEF" "$OFF" --active "$D/n9.csv"

# N10 -- a downlink moved OFF 868 MHz: ca_tbl_full's 868 energy at defaults below active's.
bump "$ACT" "$D/n10.csv" ca_tbl_full j_868 1.0
check "N10 j_868 lower at defaults than at active on ca_tbl_full" 1 \
    "$DEF" "$OFF" --active "$D/n10.csv"

# N11 -- a perfectly shaped transfer onto 868 MHz on static_2p4, which never transmits on
# 868 MHz (tx_868 0). Built on the --active side, so the defaults/node-side pair is the
# real one and checks 1-4 cannot see it: only the sect 6 check can.
bump "$ACT" "$D/n11a.csv" static_2p4 j_868 -0.000904183
bump "$D/n11a.csv" "$D/n11.csv" static_2p4 j_2p4 0.000200508
check "N11 downlink moved onto 868 MHz on static_2p4, tx_868 0 (sect 6)" 1 \
    "$DEF" "$OFF" --active "$D/n11.csv"

# N12 -- 868 MHz gained and no tier lost: a charge from nowhere. ca_tbl_full's 2.4 GHz
# energy on the --active side is set to its value at defaults, so only j_868 moved.
python3 - "$ACT" "$DEF" "$D/n12.csv" <<'PY'
import csv, sys
act, dfl, dst = sys.argv[1:4]
want = {r["arm"]: r for r in csv.DictReader(open(dfl))}["ca_tbl_full"]["j_2p4"]
rows = list(csv.reader(open(act, newline="")))
i = rows[0].index("j_2p4")
for r in rows[1:]:
    if r and r[0] == "ca_tbl_full":
        r[i] = want
csv.writer(open(dst, "w", newline=""), lineterminator="\n").writerows(rows)
PY
check "N12 j_868 rose with no loss on 2.4 GHz or UWB (charge from nowhere)" 1 \
    "$DEF" "$OFF" --active "$D/n12.csv"

# N13 -- energy moved on static_868, whose only active tier is 868 MHz.
bump "$ACT" "$D/n13.csv" static_868 j_868 -0.000703675
check "N13 j_868 moved on static_868 (fix tier IS the active tier)" 1 \
    "$DEF" "$OFF" --active "$D/n13.csv"

# N14 -- the --active file has a row missing.
grep -v '^static_uwb,' "$ACT" > "$D/n14.csv"
check "N14 --active file with the static_uwb row dropped" 1 \
    "$DEF" "$OFF" --active "$D/n14.csv"

printf '  G21 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
