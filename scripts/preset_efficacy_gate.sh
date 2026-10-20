#!/usr/bin/env bash
# G157: T6 (TECH_DEBT H.3 / I.5) -- no adopted preset mechanism fires into a zero.
# usage: bash scripts/preset_efficacy_gate.sh [<workdir>]
# Runs the preset's endgame (unknown start, seed 7, 600 s, ca_tbl_full + static_868) once as is, and once per adopted
# preset setting (DESIGN_DECISIONS 10, 11, 13, 14, 15) turned back to its code default. Every such run must differ from
# the preset run: a mechanism that is switched on and changes nothing is the edf73a5/c236834 defect class.
# NEGATIVE FIXTURE (rule 6): --agcPerReceiver=true, inert in this scenario (no link above the AGC knee; G147), must be
# called INERT by the same comparison -- so the check is shown able to say "no effect".
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g157.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
B="--specProfile=true --arms=ca_tbl_full,static_868 --seed=7 --duration=600 --trace=true --unknownStart=true --boxHalf=750 --finderMode=1"
declare -A OFF
OFF[rngKeyed]="--rngKeyed=false"; OFF[rangeErrorNoLinkLeak]="--rangeErrorNoLinkLeak=false"
OFF[rssiRangeFromShadowing]="--rssiRangeFromShadowing=false"; OFF[nlosRangeBias]="--nlosRangeBias=false"
OFF[rangeReplyPosition]="--rangeReplyPosition=false"; OFF[acquireParticles]="--acquireParticles=false"
OFF[foliageDepthClamp]="--foliageDepthClamp=false"; OFF[radioBasis]="--radioBasis=lr1121 --halfDuplexChip=true"
OFF[fadingPerBand]="--fadingPerBand=false"; OFF[relayNisInflate]="--relayNisInflate=false"
OFF[rssiSigmaLogNormal]="--rssiSigmaLogNormal=false"; OFF[imuQuScale]="--imuQuScale=1"
J=$(nproc 2>/dev/null || echo 4)
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" $B "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
run preset &
run fixture_agc --agcPerReceiver=true &
for k in "${!OFF[@]}"; do
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    # shellcheck disable=SC2086
    run "off_$k" ${OFF[$k]} &
done
wait
bad=0
same() { cmp -s "$D/preset/arm_summary.csv" "$D/$1/arm_summary.csv" && cmp -s "$D/preset/trace_ca_tbl_full_seed7.csv" "$D/$1/trace_ca_tbl_full_seed7.csv"; }
for n in preset fixture_agc $(for k in "${!OFF[@]}"; do echo "off_$k"; done); do
    [ "$(cat "$D/$n.rc")" = 0 ] || { echo "G157 FAIL RUN     $n exit $(cat "$D/$n.rc")"; bad=1; }
done
if same fixture_agc; then echo "G157 PASS FIXTURE  --agcPerReceiver (inert here) is called INERT by the same comparison"
else echo "G157 FAIL FIXTURE  the inert fixture was called effective: the comparison cannot see 'no effect'"; bad=1; fi
for k in $(printf '%s\n' "${!OFF[@]}" | sort); do
    if same "off_$k"; then echo "G157 FAIL EFFECT   $k: turning it off changes nothing -- it fires into a zero"; bad=1
    else echo "G157 PASS EFFECT   $k changes the output"; fi
done
exit $bad
