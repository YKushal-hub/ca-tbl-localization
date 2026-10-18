#!/usr/bin/env bash
# G145: V5 --c2bEscalation (master plan v3; claim C2b). usage: bash scripts/c2b_gate.sh [<workdir>]
#  DEFAULT  explicit false reproduces the default byte for byte
#  FIRES    ca_tbl_full's escalation count rises with the flag (need, not low confidence, triggers it)
#  SCOPE    static_2p4 (no escalation) is untouched
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g145.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
B="--specProfile=true --arms=static_2p4,ca_tbl_full --seed=7 --duration=900 --trace=true --rngKeyed=true --boxHalf=750 --finderMode=1"
for n in base off on; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --c2bEscalation=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --c2bEscalation=true --outdir="$D/on" > "$D/on.log" 2>&1 &
wait
bad=0
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G145 PASS DEFAULT  explicit false reproduces the default"; else echo "G145 FAIL DEFAULT  $n outputs differ"; bad=1; fi
python3 - "$D" <<'PY' || bad=1
import csv, sys
d = sys.argv[1]
g = lambda run, arm, k: next(float(r[k]) for r in csv.DictReader(open(f"{d}/{run}/arm_summary.csv")) if r["arm"] == arm)
e0, e1 = g("base", "ca_tbl_full", "escalations"), g("on", "ca_tbl_full", "escalations")
ok = e1 > e0
print(f"G145 {'PASS' if ok else 'FAIL'} FIRES    ca_tbl_full escalations {e0:.0f} -> {e1:.0f}")
sys.exit(0 if ok else 1)
PY
if cmp -s "$D/base/trace_static_2p4_seed7.csv" "$D/on/trace_static_2p4_seed7.csv"; then echo "G145 PASS SCOPE    static_2p4 untouched"; else echo "G145 FAIL SCOPE    static_2p4 changed"; bad=1; fi
exit $bad
