#!/usr/bin/env bash
# G144: V7 --rangeReplyPosition (master plan v3; TECH_DEBT H.2). usage: bash scripts/replypos_gate.sh [<workdir>]
#  DEFAULT  explicit false reproduces the default byte for byte
#  FIRES    with the flag: the V7 line reports > 0 reply-anchored finder ranges, and none is printed without it
#  SHARPER  the median reported sigma of tag 0's finder ranges (sourceKind 2, --traceRanges) drops below half the
#           default's -- the age inflation of a stale entry is gone
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g144.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
B="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=900 --trace=true --traceRanges=true --rngKeyed=true --boxHalf=750 --finderMode=1"
for n in base off on; do mkdir -p "$D/$n"; done
"$BIN" $B --outdir="$D/base" > "$D/base.log" 2>&1 &
"$BIN" $B --rangeReplyPosition=false --outdir="$D/off" > "$D/off.log" 2>&1 &
"$BIN" $B --rangeReplyPosition=true --outdir="$D/on" > "$D/on.log" 2>&1 &
wait
bad=0
n=0; for f in $(cd "$D/base" && ls); do [ "$f" = run_id.txt ] && continue; cmp -s "$D/base/$f" "$D/off/$f" || n=$((n + 1)); done
if [ "$n" -eq 0 ]; then echo "G144 PASS DEFAULT  explicit false reproduces the default"; else echo "G144 FAIL DEFAULT  $n outputs differ"; bad=1; fi
u=$(grep "V7 rangeReplyPosition" "$D/on.log" | awk '{print $NF}'); ub=$(grep -c "V7 rangeReplyPosition" "$D/base.log")
if [ -n "$u" ] && [ "$u" -gt 0 ] && [ "$ub" -eq 0 ]; then echo "G144 PASS FIRES    $u reply-anchored finder ranges; none without the flag"; else echo "G144 FAIL FIRES    on '$u', lines without the flag $ub"; bad=1; fi
python3 - "$D" <<'PY' || bad=1
import csv, statistics as st, sys
d = sys.argv[1]
def med(run):
    v = [float(r["sigmaM"]) for r in csv.DictReader(open(f"{d}/{run}/ranges_ca_tbl_full_seed7.csv")) if r["sourceKind"] == "2"]
    return (st.median(v), len(v)) if v else (float("nan"), 0)
(b, nb), (o, no) = med("base"), med("on")
ok = nb > 20 and no > 20 and o < 0.5 * b
print(f"G144 {'PASS' if ok else 'FAIL'} SHARPER  finder-range median sigma {b:.1f} m (n {nb}) -> {o:.1f} m (n {no})")
sys.exit(0 if ok else 1)
PY
exit $bad
