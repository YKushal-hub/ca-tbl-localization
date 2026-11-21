#!/usr/bin/env bash
# Merge identity check: the same scenario run with main's binary and with this branch's binary
# (plus the preset pin that restores main's preset, PIN_PRE_DD27) must give byte-identical outputs.
# spec_profile.txt (it prints the preset text, which the pin changes) and run_id.txt are excluded.
#   usage: scripts/merge_identity_check.sh MAIN_BINARY WORKDIR
set -u
cd "$(dirname "$0")/.." || exit 2
MAINBIN=$1
W=$2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
. scripts/preset_pins.sh
mkdir -p "$W"
# name | arguments (main's campaigns: LIMITS L1/L2 and ENV-SWEEP, at 600 s)
CASES=(
  "limits_L1|--specProfile=true --duration=600 --trace=true --traceCovariance=true --reportAirtime=true --arms=ca_tbl_full,fixed_threshold,static_868,static_2p4 --env=1 --tier1ExtraLossDb=15 --seed=361"
  "limits_L2|--specProfile=true --duration=600 --trace=true --reportAirtime=true --arms=ca_tbl_two_tier,ca_tbl_full --env=1 --seed=362"
  "envsweep_E0|--specProfile=true --duration=600 --trace=true --traceCovariance=true --reportAirtime=true --arms=ca_tbl_full,fixed_threshold,static_868,static_2p4,poucet_fusion --env=0 --seed=361"
)
bad=0
for c in "${CASES[@]}"; do
    n=${c%%|*}
    a=${c#*|}
    mkdir -p "$W/$n.main" "$W/$n.branch"
    # shellcheck disable=SC2086
    "$MAINBIN" $a --outdir="$W/$n.main" > "$W/$n.main.log" 2>&1 &
    # shellcheck disable=SC2086
    "$BIN" $a $PIN_PRE_DD27 --outdir="$W/$n.branch" > "$W/$n.branch.log" 2>&1 &
    wait
    sums() { (cd "$1" && find . -type f ! -name run_id.txt ! -name spec_profile.txt | sort | xargs sha256sum); }
    nf=$(sums "$W/$n.main" | wc -l)
    if [ "$nf" -lt 10 ]; then
        echo "ERROR $n: main produced $nf files (see $W/$n.main.log)"; bad=1; continue
    fi
    if diff <(sums "$W/$n.main") <(sums "$W/$n.branch") > "$W/$n.diff"; then
        echo "IDENTICAL $n ($(sums "$W/$n.main" | wc -l) files)"
    else
        echo "DIFFERS $n: $(grep -c '^>' "$W/$n.diff") files"; bad=1
    fi
done
exit $bad
