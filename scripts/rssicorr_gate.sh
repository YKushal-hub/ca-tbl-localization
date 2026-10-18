#!/usr/bin/env bash
# G149: B2 --rssiCorrInflate (scope 2026-10-17). usage: bash scripts/rssicorr_gate.sh [<workdir>]
# DEFAULT  explicit false reproduces the preset byte for byte;
# FIRES    on: static_868's trace changes, and the printed mean variance factor is > 1 over > 100 inflated ranges;
# SCOPE    on: static_2p4 (no 868 MHz range) is byte-identical;
# STATIC   fixture: a node that does not move (--speed=0) gets a factor at the cap region (> 20), a walking one less --
#          the correlation tracks movement, not time.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g149.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
B="--specProfile=true --arms=static_868,static_2p4 --seed=7 --duration=600 --trace=true"
for n in base off on still; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --rssiCorrInflate=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --rssiCorrInflate=true --outdir="$D/on" > "$D/on.log" 2>&1 &
"$BIN" $B --rssiCorrInflate=true --speed=0 --outdir="$D/still" > "$D/still.log" 2>&1 &
wait
bad=0
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G149 PASS DEFAULT  explicit false reproduces the preset"; else echo "G149 FAIL DEFAULT  $n outputs differ"; bad=1; fi
fac() { grep -h "B2 rssiCorrInflate (static_868)" "$1" | sed -E 's/.*inflated ([0-9]+), mean variance factor ([0-9.]+).*/\1 \2/'; }
read -r non fon <<< "$(fac "$D/on.log")"; read -r nst fst <<< "$(fac "$D/still.log")"
if ! cmp -s "$D/base/trace_static_868_seed7.csv" "$D/on/trace_static_868_seed7.csv" && [ "${non:-0}" -gt 100 ] && awk -v f="${fon:-0}" 'BEGIN{exit !(f > 1)}'; then
    echo "G149 PASS FIRES    static_868 changes; $non ranges inflated, mean factor $fon"; else echo "G149 FAIL FIRES    n=${non:-?} factor=${fon:-?}"; bad=1; fi
if cmp -s "$D/base/trace_static_2p4_seed7.csv" "$D/on/trace_static_2p4_seed7.csv"; then echo "G149 PASS SCOPE    static_2p4 untouched"; else echo "G149 FAIL SCOPE    static_2p4 changed"; bad=1; fi
if awk -v a="${fst:-0}" -v b="${fon:-0}" 'BEGIN{exit !(a > 20 && a > b)}'; then echo "G149 PASS STATIC   still node factor $fst > walking $fon"; else echo "G149 FAIL STATIC   still ${fst:-?} walking ${fon:-?}"; bad=1; fi
exit $bad
