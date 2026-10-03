#!/usr/bin/env bash
# G24 runner: the preset is conformant (docs/SPEC_CONFORMANCE_PLAN.md sect 3.1-3.2,
# --specProfile). What each run is for, and what each check reads, is in
# scripts/spec_profile_gate.py.
#
# usage: bash scripts/spec_profile_gate.sh [<workdir>]
#   With <workdir> the runs are left there for the self-test; without one they go to a
#   mktemp directory removed on exit.
#
# Three registered preset runs (full arm, seed 7, 900 s):
#   main   the preset as it is (5 km box, four finders searching);
#   end    the endgame: --boxHalf=750 --finderMode=1, finders converging (the owner's
#          2026-10-03 decision after MS-3: UWB is evaluated where finders reach subjects);
#   gasp   --batteryDepleteJ=2, the only way the last-gasp population exists.
# Then the preset with one mechanism's flag forced off, per mechanism (flag efficacy), the
# preset against its own flags written out (equivalence, 300 s), and two refusals. The
# preset with --fixRoutingSubjects=false is one of them since 2026-10-03: --fixRouting
# with subjects and without it is refused (TECH_DEBT N.12), so that flag has no
# efficacy run.
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then
    D=$1
    mkdir -p "$D" || exit 2
else
    D=$(mktemp -d "${TMPDIR:-/tmp}/g24.XXXXXX") || exit 2
    trap 'rm -rf "$D"' EXIT
fi
B="--arms=ca_tbl_full --seed=7 --duration=900 --trace=true --specProfile=true"
END="--boxHalf=750 --finderMode=1"
run() {  # run <name> <args...>: outputs in $D/<name>/, log in <name>.log, exit code in <name>.rc
    local n=$1
    shift
    mkdir -p "$D/$n"
    # shellcheck disable=SC2068
    "$BIN" $@ --outdir="$D/$n" > "$D/$n.log" 2>&1
    echo $? > "$D/$n.rc"
}
J=$(nproc 2>/dev/null || echo 4)
lim() {  # keep at most $J runs going
    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done
}
lim; run main $B &
lim; run end $B $END &
lim; run gasp $B --batteryDepleteJ=2 &
# flag efficacy: name, extra args (the config the mechanism is read in)
while read -r name args; do
    [ -z "$name" ] && continue
    lim; run "off_$name" $B $args &
done <<'LIST'
tier1Tdoa --tier1Tdoa=false
pcsNlosClassifier --pcsNlosClassifier=false
integrityNlosFromScore --integrityNlosFromScore=false
caseD --caseD=false
caseCRelative --caseCRelative=false --boxHalf=750 --finderMode=1
entryPrediction --entryPrediction=false
triggeredUpdates --triggeredUpdates=false
mobilityFromRange --mobilityFromRange=false
tableFuseCI --tableFuseCI=false
queryTriggeredEscalation --queryTriggeredEscalation=false
requesterEscalation --requesterEscalation=false --boxHalf=750 --finderMode=1
edgeUplinkRanges --edgeUplinkRanges=false
subjectRelayChain --subjectRelayChain=false --boxHalf=750 --finderMode=1
LIST
lim; run eq_preset --arms=ca_tbl_full --seed=7 --duration=300 --trace=true --specProfile=true &
# routing turned off explicitly, as CAMP-SPEC's single-hop cells do: the preset's routing
# weights (CAMP-SPEC's pick, 2026-10-03) are then unused, not refused (RUNS).
lim; run x_froff --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --specProfile=true --relayedRanging=false --relayChain=false --hopVarianceTerms=false --hopProvenance=false --fixRouting=false --fixRoutingConfidence=false --fixRoutingSubjects=false &
lim; run eq_flags --arms=ca_tbl_full --seed=7 --duration=300 --trace=true $(grep -oE '^ *\{"[A-Za-z0-9]+", "[^"]*"' scratch/ca_tbl_causal_sim.cc | sed -E 's/^ *\{"([A-Za-z0-9]+)", "([^"]*)"/--\1=\2/' | tr '\n' ' ') &
wait
run r_topo --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --specProfile=true --topology=1
run r_n12 --arms=ca_tbl_full --seed=1 --duration=30 --trace=false --specProfile=true --fixRoutingSubjects=false

python3 scripts/spec_profile_gate.py "$D"
