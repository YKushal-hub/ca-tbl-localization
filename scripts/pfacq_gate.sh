#!/usr/bin/env bash
# G142: V4 --acquireParticles (master plan v3; TECH_DEBT N.54). usage: bash scripts/pfacq_gate.sh [<workdir>]
# (1) unit (scratch/v4_acquire_unit.cc): two anchors hold both mirror roots; a third resolves to the true one;
#     its --break=force fixture must be red;
# (2) scenario: explicit false reproduces the default byte for byte;
# (3) scenario: the flag fires -- one PFACQ_HANDOVER line or more, and the tag's trace changes.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
UNIT="$(pwd)/build/scratch/ns3.47-v4_acquire_unit-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g142.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
bad=0
"$UNIT" || bad=1
if "$UNIT" --break=force > "$D/fix.txt" 2>&1; then echo "G142 FAIL FIXTURE  --break=force did not turn the unit checks red"; bad=1; else echo "G142 PASS FIXTURE  --break=force is red"; fi
B="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --rngKeyed=true --unknownStart=true --boxHalf=750 --finderMode=1 --acquireParticles=false"  # pinned: the preset turns it on (DESIGN_DECISIONS 10); later arguments win
for n in base off on; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --acquireParticles=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --acquireParticles=true --outdir="$D/on" > "$D/on.log" 2>&1 &
wait
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G142 PASS DEFAULT  explicit false reproduces the default"; else echo "G142 FAIL DEFAULT  $n outputs differ"; bad=1; fi
h=$(grep -c PFACQ_HANDOVER "$D/on.log"); hb=$(grep -c PFACQ_HANDOVER "$D/base.log")
if [ "$h" -ge 1 ] && [ "$hb" -eq 0 ] && ! cmp -s "$D/base/trace_ca_tbl_full_seed7.csv" "$D/on/trace_ca_tbl_full_seed7.csv"; then
    echo "G142 PASS FIRES    $h handovers with the flag, none without; the trace changes"
else echo "G142 FAIL FIRES    handovers on $h / off $hb, or the trace is unchanged"; bad=1; fi
exit $bad
