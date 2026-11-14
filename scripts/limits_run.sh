#!/usr/bin/env bash
# LIMITS runner (report/PREREG_LIMITS_2026-11-14.md). usage: bash scripts/limits_run.sh <L1|L2> <outdir> <binary-dir>
#   <binary-dir>  holds ns3.47-ca_tbl_causal_sim-optimized and lib/, built from a clean committed tree; copied to
#                 <outdir>/bin on launch and verified by sha256 before every cell, as scripts/envsweep_run.sh does.
#   <outdir>      outside the repository. Resumable: a cell whose rc is 0 is skipped.
# L1 reach sensitivity: --tier1ExtraLossDb in {15, 30}, configs A and B, --env=1, seeds 361-380, 4 arms, 1800 s.
#    (0 dB is not re-run: it is the ENV-SWEEP's E1 cells, byte-identity of the flag-off binary being checked first.)
# L2 two-tier: --arms=ca_tbl_two_tier, configs A and B, --env 0..3, seeds 361-380, 1800 s.
# Verification cells (VERIFY_*) are not measurement cells.
# Never edit this file while it runs (bash reads a script by byte offset).
set -u
STUDY=${1:?usage: limits_run.sh <L1|L2> <outdir> <binary-dir>}
OUT=${2:?outdir}
SRCBIN=${3:?binary-dir}
cd "$(dirname "$0")/.." || exit 2
case "$(cd "$(dirname "$OUT")" 2>/dev/null && pwd)/" in "$(pwd)"/*) echo "refusing: inside the repository"; exit 2;; esac
gitst() { local n; n=$(git status --porcelain --untracked-files=no 2>&1) || { echo "GIT-ERROR"; return; }; printf '%s' "$n" | grep -c . ; }
if [ -e "$OUT/build.sha256" ]; then
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "refusing resume: manifest mismatch"; exit 2; }
    echo "$(date -u +%FT%TZ) resume; source $(git rev-parse --short HEAD) dirty $(gitst)" >> "$OUT/RUN.md"
else
    [ -e "$OUT" ] && { echo "refusing: $OUT exists without a manifest"; exit 2; }
    mkdir -p "$OUT/bin"; cp "$SRCBIN/ns3.47-ca_tbl_causal_sim-optimized" "$OUT/bin/" && cp -r "$SRCBIN/lib" "$OUT/bin/lib" || exit 2
    ( cd "$OUT/bin" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so > ../build.sha256 )
    echo "$(date -u +%FT%TZ) launch $STUDY; source $(git rev-parse --short HEAD) dirty $(gitst); binary copied from $SRCBIN" > "$OUT/RUN.md"
fi
BIN="$OUT/bin/ns3.47-ca_tbl_causal_sim-optimized"
export LD_LIBRARY_PATH="$OUT/bin/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
J=${LIMITS_JOBS:-12}
cell() {
    local d=$1; shift
    [ -f "$d/rc" ] && [ "$(cat "$d/rc")" = 0 ] && return
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
    ( cd "$OUT/bin" && sha256sum -c --quiet ../build.sha256 ) || { echo "manifest mismatch before $d" >> "$OUT/RUN.md"; exit 3; }
    rm -rf "$d"; mkdir -p "$d"
    # shellcheck disable=SC2068
    ( "$BIN" $@ --outdir="$d" > "$d/run.log" 2>&1; echo $? > "$d/rc" ) &
}
C="--specProfile=true --duration=1800 --trace=true --traceCovariance=true --reportAirtime=true"
declare -A G
G[A]=""
G[B]="--boxHalf=750 --finderMode=1"
if [ "$STUDY" = L1 ]; then
    A4="--arms=ca_tbl_full,fixed_threshold,static_868,static_2p4"
    for s in $(seq 361 380); do
        for cfg in A B; do
            for o in 30 15; do
                # shellcheck disable=SC2086
                cell "$OUT/${cfg}_O${o}/s$s" $C $A4 ${G[$cfg]} --env=1 --tier1ExtraLossDb=$o --seed=$s
            done
        done
    done
    # verification: flag off, the sweep's own cell (A, E1, seed 361) with all five arms, to be diffed against the sweep
    cell "$OUT/VERIFY_A_E1_s361_flagoff" $C --arms=ca_tbl_full,fixed_threshold,static_868,static_2p4,poucet_fusion --env=1 --seed=361
elif [ "$STUDY" = L2 ]; then
    # verification: the pair, to show a filtered run's ca_tbl_full row equals the sweep's and the two-tier row equals the filtered one
    cell "$OUT/VERIFY_A_E1_s361_pair" $C --arms=ca_tbl_full,ca_tbl_two_tier --env=1 --seed=361
    for s in $(seq 361 380); do
        for cfg in A B; do
            for e in 1 0 2 3; do
                # shellcheck disable=SC2086
                cell "$OUT/${cfg}_E${e}/s$s" $C --arms=ca_tbl_two_tier ${G[$cfg]} --env=$e --seed=$s
            done
        done
    done
else
    echo "unknown study $STUDY"; exit 2
fi
wait
echo "$(date -u +%FT%TZ) done $STUDY" >> "$OUT/RUN.md"
