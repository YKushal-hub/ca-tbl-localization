#!/usr/bin/env bash
# G164 runner: --uwbMode (DW3000 Data Sheet v1.1 Table 9). Checks in scripts/uwb_mode_gate.py.
# p_leg/p_6m8/p_850  the probe with --probeNominalMargin in each mode (UWB sensitivity and ranging gain)
# e_leg/e_6m8/e_850  the endgame preset, seed 7, 300 s, ca_tbl_full, --reportAirtime (UWB frames billed)
# def/leg            code defaults, 300 s, flag absent and given legacy: byte-identical
# r_*                refusals: an unknown mode; a mode with an explicit --uwbSensitivityDbm
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g164.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
E="--specProfile=true --seed=7 --duration=300 --boxHalf=750 --finderMode=1 --arms=ca_tbl_full --reportAirtime=true"
run p_leg --probe=true --probeNominalMargin=true &
run p_6m8 --probe=true --probeNominalMargin=true --uwbMode=6m8 &
run p_850 --probe=true --probeNominalMargin=true --uwbMode=850k &
# shellcheck disable=SC2086
run e_leg $E &
# shellcheck disable=SC2086
run e_6m8 $E --uwbMode=6m8 &
# shellcheck disable=SC2086
run e_850 $E --uwbMode=850k &
run def --seed=7 --trace=true --duration=300 --arms=ca_tbl_full &
run leg --seed=7 --trace=true --duration=300 --arms=ca_tbl_full --uwbMode=legacy &
wait
run r_name --arms=ca_tbl_full --seed=1 --duration=10 --uwbMode=2m
run r_both --arms=ca_tbl_full --seed=1 --duration=10 --uwbMode=6m8 --uwbSensitivityDbm=-95
python3 scripts/uwb_mode_gate.py "$D"
