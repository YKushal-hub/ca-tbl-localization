#!/usr/bin/env bash
# CAMP-FINAL2 runner (report/PREREG_CAMP_FINAL2.md): CAMP-FINAL's cells on the final preset. usage: bash scripts/camp_final2_run.sh <outdir> (outside the repo)
set -u
cd "$(dirname "$0")/.." || exit 2
OUT=${1:?usage}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
if [ -e "$OUT/build.sha256" ]; then
    # RESUME (after an interruption): the same frozen binary, verified; finished cells are skipped, partial
    # ones are deleted and re-run by cell(). No cell is chosen or dropped.
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "refusing resume: manifest mismatch"; exit 2; }
    echo "$(date -u +%FT%TZ) resume; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" >> "$OUT/RUN.md"
else
    [ -e "$OUT" ] && { echo "refusing: $OUT exists without a manifest"; exit 2; }
    mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
    echo "$(date -u +%FT%TZ) launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
ARMS7="--arms=ca_tbl_full,ca_tbl_no_integrity,fixed_threshold,static_868,static_2p4,static_uwb,poucet_fusion"
P="--specProfile=true --duration=1800 --trace=true --unknownStart=true --imuQuScale=10 --fixedVarThreshold=12"
END="--boxHalf=750 --finderMode=1"
N8="--nGateways=8 --gwLayout=legacy"
SINGLE="--relayedRanging=false --relayChain=false --hopVarianceTerms=false --hopProvenance=false --fixRouting=false --fixRoutingConfidence=false --fixRoutingSubjects=false"
cell "$OUT/T_toa/s1" --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --toaTable=true
# shellcheck disable=SC2086
for s in $(seq 1 20); do
    cell "$OUT/A_main/s$s" $ARMS7 $P --seed=$s --reportAirtime=true
    cell "$OUT/B_end/s$s" $ARMS7 $P $END --seed=$s --reportAirtime=true
    cell "$OUT/C_n8/s$s" --arms=ca_tbl_full,static_868 $P $N8 --seed=$s
    cell "$OUT/G_single/s$s" --arms=ca_tbl_full $P $SINGLE --seed=$s
    cell "$OUT/G_single_end/s$s" --arms=ca_tbl_full $P $SINGLE $END --seed=$s
    cell "$OUT/H_flood/s$s" --arms=ca_tbl_full $P --seed=$s --reportAirtime=true --reversePath=false
    cell "$OUT/W_end/s$s" --arms=ca_tbl_full,static_2p4,static_868 $P $END --seed=$s --tofErrorModel=wolf --tofCalibrate=true
done
for f in "rb_common --injectRangeBias=30 --injectFaultAnchors=0" "rb_single --injectRangeBias=30 --injectFaultAnchors=1" \
         "txdrop --injectTxPowerDropDb=6" "antenna --injectAntennaLossDb=10"; do
    # shellcheck disable=SC2086
    set -- $f
    name=$1
    shift
    for rec in downweight none; do
        for s in $(seq 1 10); do
            # shellcheck disable=SC2068
            cell "$OUT/E_${name}_${rec}/s$s" --arms=ca_tbl_full,ca_tbl_no_integrity $P --seed=$s \
                --injectBiasFraction=0.25 --suspectRecovery=$rec $@
        done
    done
done
wait
echo "$(date -u +%FT%TZ) all cells done" >> "$OUT/RUN.md"
