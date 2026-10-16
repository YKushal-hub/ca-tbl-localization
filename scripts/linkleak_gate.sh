#!/usr/bin/env bash
# G140: V2b --rangeErrorNoLinkLeak and --probeTerms (master plan v3). usage: bash scripts/linkleak_gate.sh [<workdir>]
#  LEAK     without the fix, --rssiRangeFromShadowing changes 868 MHz delivery (the leak exists: negative control)
#  NOLEAK   with the fix, 868 MHz delivery is identical with S1 on and off (the range error no longer reaches reception)
#  TERMS    --probeTerms writes probe_terms.csv with rows for all three tiers, digested in run_id.txt
#  DEFAULT  without --probeTerms no probe_terms.csv is written, and explicit --rangeErrorNoLinkLeak=false
#           reproduces the default probe byte for byte
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g140.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--probe=true --seed=3 --foliageDepthClamp=true"
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" $P "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
run base &
run offx --rangeErrorNoLinkLeak=false &
run leak1 --rssiRangeFromShadowing=true &
run fix0 --rangeErrorNoLinkLeak=true --probeTerms=true &
run fix1 --rangeErrorNoLinkLeak=true --rssiRangeFromShadowing=true --probeTerms=true &
wait
python3 scripts/linkleak_gate.py "$D"
