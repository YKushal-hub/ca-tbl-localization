#!/usr/bin/env bash
# G141: V3 --rangeBiasStates and --rssiSigmaLogNormal (master plan v3). usage: bash scripts/consider_gate.sh [<workdir>]
# (1) unit Monte Carlo (scratch/v3_consider_unit.cc) and its --break=off fixture;
# (2) scenario: explicit false for both flags reproduces the default byte for byte;
# (3) scenario: --rangeBiasStates=true changes a hybrid arm's output and leaves static_2p4's untouched.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
UNIT="$(pwd)/build/scratch/ns3.47-v3_consider_unit-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g141.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
bad=0
"$UNIT" || bad=1
if "$UNIT" --break=off > "$D/fix.txt" 2>&1; then echo "G141 FAIL FIXTURE  --break=off did not turn CONSIST red"; bad=1; else echo "G141 PASS FIXTURE  --break=off is red"; fi
B="--specProfile=true --arms=static_2p4,ca_tbl_full --seed=7 --duration=600 --trace=true --rngKeyed=true --rssiRangeFromShadowing=true --boxHalf=750 --finderMode=1 --rssiSigmaLogNormal=false"  # pinned: the preset turns it on (DESIGN_DECISIONS 14)
for n in base off on; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --rangeBiasStates=false --rssiSigmaLogNormal=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --rangeBiasStates=true --outdir="$D/on" > "$D/on.log" 2>&1 &
wait
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G141 PASS DEFAULT  explicit false reproduces the default"; else echo "G141 FAIL DEFAULT  $n outputs differ"; bad=1; fi
if cmp -s "$D/base/trace_ca_tbl_full_seed7.csv" "$D/on/trace_ca_tbl_full_seed7.csv"; then echo "G141 FAIL FIRES    ca_tbl_full unchanged with the flag on"; bad=1; else echo "G141 PASS FIRES    ca_tbl_full changes with the flag on"; fi
if cmp -s "$D/base/trace_static_2p4_seed7.csv" "$D/on/trace_static_2p4_seed7.csv"; then echo "G141 PASS SCOPE    static_2p4 untouched (no 868 MHz range enters it)"; else echo "G141 FAIL SCOPE    static_2p4 changed"; bad=1; fi
exit $bad
