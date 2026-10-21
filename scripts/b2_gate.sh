#!/usr/bin/env bash
# G136 runner: B2 --dualBaseOnDemand. usage: bash scripts/b2_gate.sh [<workdir>]; checks in scripts/b2_gate.py
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_DD19_22: the preset this gate was written against (DESIGN_DECISIONS 19-22)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g136.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
# PIN (DESIGN_DECISIONS 18, 2026-10-20): this gate's thresholds were set before the preset adopted the receiver
# corrections; the two flags restore the receiver it was written against, as G63/G87/G89 pin theirs.
B="--specProfile=true $PIN_DD19_22 --arms=ca_tbl_full --seed=7 --duration=900 --trace=true --imuQuScale=1 --uwbSensitivityDbm=-104 --iqOverloadOnly=false"  # pinned: ENERGY was set before DESIGN_DECISIONS 15 (under DD 15 on-demand costs +0.38 J; TECH_DEBT O.11)
run base $B &
run off $B --dualBaseOnDemand=false &
run on $B --dualBaseOnDemand=true &
wait
python3 scripts/b2_gate.py "$D"
