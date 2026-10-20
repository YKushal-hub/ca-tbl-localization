#!/usr/bin/env bash
# INTEG2 runner (report/PREREG_INTEG2.md). usage: bash scripts/integ2_run.sh cal|eval <outdir> (outside the repo)
set -u
cd "$(dirname "$0")/.." || exit 2
PHASE=${1:?cal|eval}; OUT=${2:?outdir}
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
if [ ! -e "$OUT/build.sha256" ]; then
    mkdir -p "$OUT/bin"; cp build/scratch/ns3.47-ca_tbl_causal_sim-optimized "$OUT/bin/" && cp -r build/lib "$OUT/bin/lib" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
fi
( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch"; exit 2; }
echo "$(date -u +%FT%TZ) $PHASE launch; source $(git rev-parse --short HEAD) dirty $(git status --porcelain | wc -l)" >> "$OUT/RUN.md"
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=$(nproc 2>/dev/null || echo 4)
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
P="--specProfile=true --duration=1800 --trace=true --unknownStart=true --fixedVarThreshold=12"
END="--boxHalf=750 --finderMode=1"
if [ "$PHASE" = cal ]; then
    for s in $(seq 21 40); do
        # shellcheck disable=SC2086
        cell "$OUT/cal/CAL_main/s$s" --arms=ca_tbl_full $P --seed=$s
        # shellcheck disable=SC2086
        cell "$OUT/cal/CAL_end/s$s" --arms=ca_tbl_full $P $END --seed=$s
    done
else
    [ -f "$OUT/fit.json" ] || { echo "no fit.json: run integ2_analyse.py fit first"; exit 2; }
    CHI=$(python3 -c "import json;print(json.load(open('$OUT/fit.json'))['integrityChi2'])")
    echo "$(date -u +%FT%TZ) eval with integrityChi2 fit $CHI (base 1992.9 = the preset)" >> "$OUT/RUN.md"
    for f in "rb_common --injectRangeBias=30 --injectFaultAnchors=0" "rb_single --injectRangeBias=30 --injectFaultAnchors=1" \
             "txdrop --injectTxPowerDropDb=6" "antenna --injectAntennaLossDb=10"; do
        # shellcheck disable=SC2086
        set -- $f; name=$1; shift
        for s in $(seq 1 10); do
            # shellcheck disable=SC2068,SC2086
            cell "$OUT/base/E_${name}_downweight/s$s" --arms=ca_tbl_full,ca_tbl_no_integrity $P --seed=$s --injectBiasFraction=0.25 --suspectRecovery=downweight $@
            # shellcheck disable=SC2068,SC2086
            cell "$OUT/fit/E_${name}_downweight/s$s" --arms=ca_tbl_full,ca_tbl_no_integrity $P --seed=$s --injectBiasFraction=0.25 --suspectRecovery=downweight --integrityChi2=$CHI $@
        done
    done
fi
wait
echo "$(date -u +%FT%TZ) $PHASE done" >> "$OUT/RUN.md"
