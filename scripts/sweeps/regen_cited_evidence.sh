#!/usr/bin/env bash
# Reconstruct four results directories that deliverables cite and the repository never held.
#
# results/joint_criterion, siting_repair, claim2_rebase and claim4_src are cited as the
# source of headline results (CA-TBL_Positive_Results.md, Simulation.md, experiments/
# *_RESULT.md). They were never committed on any ref and were deleted with the untracked
# results/ tree after 9a636d7. Their sidecars went with them, so scripts/regenerate_results.sh
# -- which replays a directory's OWN run_id.txt invocation and refuses to emit plausible
# substitutes -- cannot regenerate them. This is the fallback, and it is a different thing:
#
#   A RECONSTRUCTION, NOT A REPRODUCTION. The invocation is rebuilt from each campaign's
#   pre-registration (flags) and from scripts/make_figures.py (directory layout). It
#   re-measures the published numbers under current code; it does not restore the original
#   evidence, and agreement with the published numbers is a finding to be checked -- by
#   scripts/make_figures.py's assertions -- not assumed. Each reconstructed top-level
#   directory carries RECONSTRUCTION.md saying exactly this, which is tracked.
#
# Flags per campaign, verbatim from:
#   joint_criterion  JOINT_CRITERION_REGISTRATION.md "Design"
#   claim2_rebase    CLAIM2_BASELINE_REGISTRATION.md "Design"
#   siting_repair    SITING_REPAIR_REGISTRATION.md "Design"; complementary = --uwbSiting=true,
#                    repaired PCS = --pcsForm=min (the form that registration defines)
#   claim4_src       CLAIM4_NLOS_SOURCE_REGISTRATION.md "Design" + its --arms amendment.
#                    Its directory layout is not recorded anywhere that survives; the
#                    names used here are this script's and say so in RECONSTRUCTION.md.
#
# Not reconstructed HERE: p10_env_v7, p10_env_v8, p10_plane_v7. CORRECTED 2026-10-15: an earlier
# version of this comment said no surviving document states their flags. Wrong --
# WORK_ORDER_PROMPT_ENVELOPE_V7.md §1 and WORK_ORDER_PROMPT_OPERATING_ENVELOPE.md §1 specify the
# two envelope sets (untracked prompt files; instructions, not records of what ran). They are a
# separate, larger reconstruction (320 invocations) listed in report/AUDIT_2026_10_15.md.
# p10_plane_v7's flags are stated nowhere and can only be inferred, so it is not reconstructed.
#
# Refuses a dirty tree and any existing target directory.

set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2
BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
JOBS=${REGEN_JOBS:-11}
ONLY=${REGEN_ONLY:-"joint_criterion claim2_rebase siting_repair claim4_src"}

[ -x "$BIN" ] || { echo "no binary at $BIN"; exit 2; }
dirty=$(git status --porcelain --untracked-files=no -- src scratch | wc -l)
[ "$dirty" -eq 0 ] || { echo "REFUSING: $dirty tracked modification(s) under src/ scratch/"; exit 2; }
HEAD=$(git rev-parse --short HEAD)

JOBFILE=$(mktemp)
trap 'rm -f "$JOBFILE"' EXIT
add() { echo "$1|$2" >> "$JOBFILE"; }

note() {  # note <topdir> <source> <layout>
    cat > "$1/RECONSTRUCTION.md" <<EOF
# $(basename "$1") — reconstructed, not reproduced

Produced by \`scripts/sweeps/regen_cited_evidence.sh\` at \`$HEAD\`. The original directory
was never committed and its sidecars are lost, so the invocation was rebuilt from
$2. Layout: $3.

**What this directory is:** a re-measurement under current code of the quantities the
original campaign published. **What it is not:** the original evidence. Where its numbers
differ from the published ones, the difference is a finding and is recorded in the
document that cites this directory, never smoothed over here.
EOF
}

for top in $ONLY; do
    [ -e "results/$top" ] && { echo "REFUSING: results/$top exists"; exit 2; }
done

for top in $ONLY; do
    mkdir -p "results/$top"
    case $top in
    joint_criterion)
        note "results/$top" "\`src/catbl-propagation-study/experiments/JOINT_CRITERION_REGISTRATION.md\` §Design" \
             "\`seed_<N>\`, N = 21..72, as \`scripts/make_figures.py\` reads it"
        for s in $(seq 21 72); do
            add "results/$top/seed_$s" "--seed=$s --duration=3600 --boxHalf=2000 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --pcsForm=min --trace=true --arms=ca_tbl_full,static_2p4"
        done ;;
    claim2_rebase)
        note "results/$top" "\`src/catbl-propagation-study/experiments/CLAIM2_BASELINE_REGISTRATION.md\` §Design" \
             "\`<nlosSource>_ev<escalationRequiresEvidence>/seed_<N>\`, as \`scripts/make_figures.py\` reads it"
        for n in legacy propagation; do for ev in false true; do for s in $(seq 1 20); do
            add "results/$top/${n}_ev${ev}/seed_$s" "--seed=$s --duration=3600 --boxHalf=2000 --gwGrid=5 --trace=true --arms=ca_tbl_full --escalationRequiresEvidence=$ev --nlosSource=$n"
        done; done; done ;;
    siting_repair)
        note "results/$top" "\`src/catbl-propagation-study/experiments/SITING_REPAIR_REGISTRATION.md\` §Design; complementary siting is \`--uwbSiting=true\`, the repaired PCS is \`--pcsForm=min\`" \
             "\`<colocated|complementary>_<asis|min>/seed_<N>\`; the two colocated names are as \`scripts/make_figures.py\` reads them, the complementary names are this script's"
        for si in colocated complementary; do for pc in asis min; do for s in $(seq 1 20); do
            f=""; [ "$si" = complementary ] && f="$f --uwbSiting=true"; [ "$pc" = min ] && f="$f --pcsForm=min"
            add "results/$top/${si}_${pc}/seed_$s" "--seed=$s --duration=3600 --boxHalf=2000 --gwGrid=5 --nlosSource=propagation --escalationRequiresEvidence=true --arms=ca_tbl_full,static_2p4 --trace=true$f"
        done; done; done ;;
    claim4_src)
        note "results/$top" "\`src/catbl-propagation-study/experiments/CLAIM4_NLOS_SOURCE_REGISTRATION.md\` §Design and its \`--arms=ca_tbl_full\` amendment" \
             "\`od<floorOnDemand>_<nlosSource>/seed_<N>\` — **this script's naming**; the original layout is not recorded in any surviving document"
        for od in true false; do for n in legacy propagation; do for s in $(seq 1 20); do
            add "results/$top/od${od}_${n}/seed_$s" "--seed=$s --duration=3600 --boxHalf=1500 --gwGrid=5 --escalationRequiresEvidence=true --floorSkipK=1 --floorHeartbeatN=0 --trace=true --arms=ca_tbl_full --floorOnDemand=$od --nlosSource=$n"
        done; done; done ;;
    *) echo "unknown target $top"; exit 2 ;;
    esac
done

echo "$(wc -l < "$JOBFILE") invocations at $HEAD, $JOBS in parallel"
run_one() {
    d=${1%%|*}
    args=${1#*|}
    mkdir -p "$d"
    # shellcheck disable=SC2086
    if "$BIN" $args --outdir="$d" > "$d.stdout" 2> "$d.stderr"; then echo "ok   $d"; else echo "FAIL $d"; fi
}
export -f run_one
export BIN
xargs -P "$JOBS" -I{} bash -c 'run_one "$@"' _ {} < "$JOBFILE"
echo "done. Reconcile with: python3 scripts/make_figures.py  (its assertions compare against the documents)"
