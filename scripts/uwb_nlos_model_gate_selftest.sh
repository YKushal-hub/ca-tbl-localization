#!/usr/bin/env bash
# G20b negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# Two real runs of the gate's configuration are made here, in a mktemp directory: the model
# ON (--uwbNlosModel=ieee802154a) and OFF (the default, legacy). Each fixture copies the ON
# run with ONE thing falsified, so a gate that only counted rows, or only checked LOS rows,
# or only checked the summary, would pass some of them. Each must give its stated exit code.
#
# N0 matters most: the SAME run with the model OFF must fail. Under legacy no UWB poll draws
# NLOS at 1-2 m outdoors (TECH_DEBT M.2), which is the state the model exists to change.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/uwb_nlos_model_gate.py"
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
D=$(mktemp -d "${TMPDIR:-/tmp}/g20b.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT
# The gate's configuration: legacy topology (the one where UWB links are polled), open
# ground with no vegetation and a 1 m UWB triad, seed 101. Deliberately NOT one of the
# cells report/PREREG_UWB_NLOS.md registers (those use --vegFacility=1 and a 2 m triad, or
# the forest default, at seeds 7-16).
CFG="--topology=1 --arms=static_uwb --envFacility=0 --vegFacility=0 --uwbTriad=1 --nlosClassifier=true --duration=600 --trace=false --seed=101"

pass=0
fail=0
check() {  # check <name> <expected exit> <gate args...>
    local name=$1
    local want=$2
    shift 2
    python3 "$GATE" "$@" > "$D/out.txt" 2>&1
    local rc=$?
    if [ "$rc" -eq "$want" ]; then
        printf '  G20b ok    %-64s exit=%d\n' "$name" "$rc"
        [ "$want" -eq 1 ] && grep -m1 'FAIL' "$D/out.txt" | sed 's/^/             /'
        pass=$((pass + 1))
    else
        printf '  G20b FAIL  %-64s exit=%d expected %d\n' "$name" "$rc" "$want"
        sed 's/^/             /' "$D/out.txt" | head -8
        fail=$((fail + 1))
    fi
}
# edit <in> <out> <python over rows `r` (list of dicts) and `hdr`>
edit() {
    python3 - "$1" "$2" "$3" <<'PY'
import csv, sys
src, dst, expr = sys.argv[1], sys.argv[2], sys.argv[3]
with open(src, newline="") as f:
    rd = csv.reader(f)
    hdr = next(rd)
    r = [dict(zip(hdr, row)) for row in rd]
exec(expr)
with open(dst, "w", newline="") as f:
    w = csv.writer(f, lineterminator="\n")
    w.writerow(hdr)
    for row in r:
        w.writerow([row[k] for k in hdr])
PY
}

if [ ! -x "$BIN" ]; then
    printf '  G20b ERROR no binary at %s\n' "$BIN"
    exit 1
fi
mkdir -p "$D/on" "$D/off"
# shellcheck disable=SC2086
"$BIN" $CFG --uwbNlosModel=ieee802154a --outdir="$D/on" > "$D/on.log" 2>&1 || { echo "  G20b ERROR model-on run"; exit 1; }
# shellcheck disable=SC2086
"$BIN" $CFG --outdir="$D/off" > "$D/off.log" 2>&1 || { echo "  G20b ERROR model-off run"; exit 1; }
L="$D/on/nlos_links_static_uwb_seed101.csv"
S="$D/on/nlos_summary_static_uwb_seed101.csv"

# A decoded NLOS row to falsify, if the run has one; otherwise one is synthesised from a LOS
# row with the formula's own value, so N1 still tampers a row that P2 shows would pass.
edit "$L" "$D/p2.csv" "
import math
if not any(x['tier']=='UWB' and x['truthNlos']=='1' for x in r):
    k = next(i for i, x in enumerate(r) if x['tier']=='UWB')
    d = max(float(r[k]['distM']), 1.0)
    r[k]['truthNlos'] = '1'
    r[k]['nlosDb'] = repr((73.0-45.6) + 10*(2.5-1.76)*math.log10(d) + 20*(0.13-0.12)*math.log10(7.99e9/5e9))"

# P1 -- the real model-on run.
check "P1 model-on run passes" 0 "$L" "$S"
# P2 -- the same with one decoded NLOS row carrying the exact formula value (non-vacuous).
check "P2 a decoded NLOS row at exactly dPL(distM) passes" 0 "$D/p2.csv" "$S"
# N0 -- the same configuration, model OFF.
check "N0 same run, model off (legacy)" 1 "$D/off/nlos_links_static_uwb_seed101.csv" "$D/off/nlos_summary_static_uwb_seed101.csv"
# N1 -- a decoded NLOS row 1e-3 dB off the formula.
edit "$D/p2.csv" "$D/n1.csv" "
k = next(i for i, x in enumerate(r) if x['tier']=='UWB' and x['truthNlos']=='1')
r[k]['nlosDb'] = repr(float(r[k]['nlosDb']) + 1e-3)"
check "N1 an NLOS row 1e-3 dB off dPL" 1 "$D/n1.csv" "$S"
# N2 -- a LOS row carrying an obstruction loss.
edit "$L" "$D/n2.csv" "
k = next(i for i, x in enumerate(r) if x['tier']=='UWB' and x['truthNlos']=='0')
r[k]['nlosDb'] = '0.5'"
check "N2 a LOS row with nlosDb 0.5" 1 "$D/n2.csv" "$S"
# N3 -- the outdoor file checked against the office pair.
check "N3 outdoor rows checked as office (CM3/CM4)" 1 --office "$D/p2.csv" "$S"
# N4 -- no NLOS draw anywhere: rows all LOS and the summary's lostNlos zeroed.
edit "$L" "$D/n4l.csv" "
r[:] = [x for x in r if not (x['tier']=='UWB' and x['truthNlos']=='1')]"
edit "$S" "$D/n4s.csv" "
for x in r:
    if x['kind']=='confusion' and x['tier']=='UWB': x['lostNlos'] = '0'"
check "N4 no UWB poll drew NLOS (population absent)" 1 "$D/n4l.csv" "$D/n4s.csv"
# N5 -- a formula keyed on d_2D instead of d_3D: the row's distM shortened by the height.
edit "$D/p2.csv" "$D/n5.csv" "
import math
k = next(i for i, x in enumerate(r) if x['tier']=='UWB' and x['truthNlos']=='1')
d3 = float(r[k]['distM']); r[k]['distM'] = repr(math.sqrt(max(d3*d3 - 1.0, 1.0)))"
check "N5 an NLOS row whose distance is the horizontal one" 1 "$D/n5.csv" "$S"
# N6 -- usage: one argument.
check "N6 usage error (one file)" 2 "$L"

# The model must also be what moved the population: the ON run drew NLOS, the OFF run none.
on_nl=$(python3 -c "import csv;print(sum(int(r['lostNlos']) for r in csv.DictReader(open('$S')) if r['kind']=='confusion' and r['tier']=='UWB'))")
printf '  G20b note: model on, %s NLOS UWB polls lost in 600 s\n' "$on_nl"

printf '  G20b selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
