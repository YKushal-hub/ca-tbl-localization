#!/usr/bin/env bash
# G148: C4 --foliageP833 (scope 2026-10-17). usage: bash scripts/foliage_p833_gate.sh [<workdir>]
# (1) unit (scratch/c4_p833_unit.cc): ITU-R P.833-10 Table 1 gamma, eq. (2) Am, the band values and the forest-only
#     scope, and its --break=table fixture (red); (2) explicit false reproduces the default (probe and a short run);
# (3) on the probe the flag lowers 868 MHz forest loss (0.20 -> 0.163 dB/m, 25 -> 23.5 dB) and raises 2.4 GHz
#     (0.434 -> 0.388 dB/m but 25 -> 36.3 dB Am), and leaves UWB's foliage unchanged.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
UNIT="$(pwd)/build/scratch/ns3.47-c4_p833_unit-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g148.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
bad=0
"$UNIT" || bad=1
if "$UNIT" --break=table > "$D/fix.txt" 2>&1; then echo "G148 FAIL FIXTURE  --break=table did not turn the unit checks red"; bad=1; else echo "G148 PASS FIXTURE  --break=table is red"; fi
P="--specProfile=true --probe=true --probeTerms=true --seed=7"
R="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=600"
for n in pbase poff pon rbase roff; do mkdir -p "$D/$n"; done
"$BIN" $P --outdir="$D/pbase" > "$D/pbase.log" 2>&1 &
"$BIN" $P --foliageP833=false --outdir="$D/poff" > "$D/poff.log" 2>&1 &
"$BIN" $P --foliageP833=true --outdir="$D/pon" > "$D/pon.log" 2>&1 &
"$BIN" $R --outdir="$D/rbase" > "$D/rbase.log" 2>&1 &
"$BIN" $R --foliageP833=false --outdir="$D/roff" > "$D/roff.log" 2>&1 &
wait
n=0; for p in pbase:poff rbase:roff; do a=${p%%:*}; b=${p##*:}; for f in $(cd "$D/$a" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/$a/$f" "$D/$b/$f" || n=$((n + 1)); done; done
if [ "$n" -eq 0 ]; then echo "G148 PASS DEFAULT  explicit false reproduces the default (probe and 600 s run)"; else echo "G148 FAIL DEFAULT  $n outputs differ"; bad=1; fi
python3 - "$D" <<'PY' || bad=1
import csv, sys
d = sys.argv[1]
def fol(run, tier, lo):
    r = [float(x["mean_foliage"]) for x in csv.DictReader(open(f"{d}/{run}/probe_terms.csv")) if x["tier"] == tier and float(x["distance_m"]) >= lo]
    return sum(r) / len(r)
a8, b8 = fol("pbase", "868MHz", 500), fol("pon", "868MHz", 500)
a2, b2 = fol("pbase", "2.4GHz", 300), fol("pon", "2.4GHz", 300)
au, bu = fol("pbase", "UWB", 0), fol("pon", "UWB", 0)
ok = b8 < a8 - 0.5 and b2 > a2 + 2.0 and abs(bu - au) < 1e-9
print(f"G148 {'PASS' if ok else 'FAIL'} FIRES    mean foliage 868 MHz {a8:.1f} -> {b8:.1f} dB; 2.4 GHz {a2:.1f} -> {b2:.1f} dB; UWB {au:.2f} -> {bu:.2f} dB")
sys.exit(0 if ok else 1)
PY
exit $bad
