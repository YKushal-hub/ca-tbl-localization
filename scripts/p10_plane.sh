#!/usr/bin/env bash
# Claim 1a: the availability-accuracy plane, all six arms, 20 seeds, boxHalf=2000.
# Current code, escalationRequiresEvidence defaulting OFF -- byte-identical to the
# pre-change binary (null control verified). Pre-registration b49df46 + a059203.
set -u
cd /home/kushal/ns/ns-3-dev
run_one() {
    s=$1; o=results/p10_plane/seed_${s}; mkdir -p "$o"
    ./ns3 run "ca_tbl_causal_sim --seed=$s --duration=3600 --boxHalf=2000 --gwGrid=5 \
--trace=true --outdir=$o" >/dev/null 2>&1 && echo "ok $s" || echo "FAIL $s"
}
export -f run_one
rm -rf results/p10_plane; mkdir -p results/p10_plane
seq 1 20 | xargs -P 6 -n 1 bash -c 'run_one "$@"' _
