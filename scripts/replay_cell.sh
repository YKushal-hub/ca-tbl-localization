#!/usr/bin/env bash
# Re-run a tracked result cell from its run_id.txt with the current build and compare every
# output file's sha256 against the digests the cell recorded. Used to prove a merge keeps old
# results byte for byte.
#   usage: scripts/replay_cell.sh results/<campaign>/<cell>/run_id.txt WORKDIR [EXTRA FLAGS...]
# EXTRA FLAGS are appended after the recorded ones, e.g. "$PIN_PRE_DD27" for a cell run under the
# preset before DESIGN_DECISIONS 27 (scripts/preset_pins.sh).
set -u
cd "$(dirname "$0")/.." || exit 2
RID=$1
W=$2
shift 2
EXTRA="$*"
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
[ -f "$RID" ] || { echo "no $RID"; exit 2; }
ARGS=$(sed -n 2p "$RID" | sed 's/^build [^|]*| [^ ]* //' | sed 's/ --outdir=[^ ]*//')
mkdir -p "$W"
# shellcheck disable=SC2086
"$BIN" $ARGS $EXTRA --outdir="$W" > "$W.log" 2>&1 || { echo "REPLAY FAIL: exit $? ($RID)"; exit 1; }
awk '/^digest sha256/{f=1; next} f && NF==2 {print}' "$RID" | sort -k2 > "$W.want"
(cd "$W" && find . -type f ! -name run_id.txt | sed 's|^\./||' | sort | xargs sha256sum) | sort -k2 > "$W.got"
if diff <(awk '{print $2, $1}' "$W.want") <(awk '{print $2, $1}' "$W.got") > "$W.diff"; then
    echo "REPLAY OK: $(wc -l < "$W.want") files identical ($RID)"
else
    echo "REPLAY DIFF ($RID):"; head -20 "$W.diff"; exit 1
fi
