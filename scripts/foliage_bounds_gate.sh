#!/usr/bin/env bash
# G154: T12 --foliageScaleBounds (TECH_DEBT N.41). usage: bash scripts/foliage_bounds_gate.sh [<workdir>]
# DEFAULT explicit false reproduces the default probe; FIRES UWB mean foliage at 10 m rises by > 2 dB (the clamp
# lifted); SCOPE every 868 MHz probe row is identical; FIXTURE the default probe compared with itself must fail FIRES.
set -u
cd "$(dirname "$0")/.." || exit 2
. scripts/preset_pins.sh  # PIN_PRE_DD19: the preset this gate was written against (DESIGN_DECISIONS 19-24)
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g154.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
P="--specProfile=true $PIN_PRE_DD19 --probe=true --probeTerms=true --seed=7"
for n in base off on; do mkdir -p "$D/$n"; done
"$BIN" $P --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $P --foliageScaleBounds=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $P --foliageScaleBounds=true --outdir="$D/on" > "$D/on.log" 2>&1 &
wait
bad=0
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G154 PASS DEFAULT  explicit false reproduces the default probe"; else echo "G154 FAIL DEFAULT  $n outputs differ"; bad=1; fi
python3 - "$D" <<'PY' || bad=1
import csv, sys
d = sys.argv[1]
f = lambda r: {(x["tier"], float(x["distance_m"])): x for x in csv.DictReader(open(f"{d}/{r}/probe_terms.csv"))}
def check(a, b, label):
    rise = float(b[("UWB", 10.0)]["mean_foliage"]) - float(a[("UWB", 10.0)]["mean_foliage"])
    same868 = all(a[k] == b[k] for k in a if k[0] == "868MHz")
    return rise > 2.0 and same868, f"{label}: UWB foliage at 10 m +{rise:.2f} dB, 868 MHz rows identical {same868}"
a, b = f("base"), f("on")
ok, msg = check(a, b, "flag on")
print(f"G154 {'PASS' if ok else 'FAIL'} FIRES    {msg}")
okf, msgf = check(a, a, "fixture (default vs itself)")
print(f"G154 {'PASS' if not okf else 'FAIL'} FIXTURE  {msgf} -- must not pass FIRES")
sys.exit(0 if ok and not okf else 1)
PY
exit $bad
