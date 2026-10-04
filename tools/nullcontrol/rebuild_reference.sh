#!/usr/bin/env bash
# Rebuild the frozen null-control reference on THIS machine.
#
# usage: bash tools/nullcontrol/rebuild_reference.sh [<refdir>]     (default ~/catbl_ref_spec0)
#
# Why rebuilt rather than copied: the build uses -march=native, so byte-identity of outputs
# holds on one machine and is not guaranteed across machines (TECH_DEBT 4.2). The reference
# is the binary at e6b3da9 (engine 57225b8, frozen 2026-09-30), run with NC1..NC9's arguments
# (tools/nullcontrol/args/). Every engine change since keeps all nine byte-identical with its
# new flags absent; tools/nullcontrol/nc.sh checks that against this directory.
#
# Takes a git worktree of the reference commit, builds it optimized (gcc-11/g++-11 unless
# CC/CXX say otherwise), copies the binary and its libraries into <refdir>, and runs the nine
# cases into <refdir>/nc/NCk. About 20-40 min. Never touches the working tree.
set -eu
ROOT=$(cd "$(dirname "$0")/../.." && pwd)
REF=${1:-$HOME/catbl_ref_spec0}
COMMIT=e6b3da9d8789c1f2ed34a53254a1b395a5645702
[ -e "$REF/nc" ] && { echo "refusing: $REF/nc exists; move it aside first"; exit 2; }
WT=$(mktemp -d "${TMPDIR:-/tmp}/catbl_ref_wt.XXXXXX")
trap 'git -C "$ROOT" worktree remove --force "$WT/src" >/dev/null 2>&1 || true' EXIT
git -C "$ROOT" worktree add --detach "$WT/src" "$COMMIT"
(
    cd "$WT/src"
    CC=${CC:-gcc-11} CXX=${CXX:-g++-11} ./ns3 configure --enable-examples --build-profile=optimized > "$WT/configure.log" 2>&1
    CC=${CC:-gcc-11} CXX=${CXX:-g++-11} ./ns3 build scratch/ca_tbl_causal_sim > "$WT/build.log" 2>&1
)
mkdir -p "$REF/lib" "$REF/nc"
cp "$WT/src/build/scratch/ns3.47-ca_tbl_causal_sim-optimized" "$REF/"
cp "$WT"/src/build/lib/libns3.47-*-optimized.so "$REF/lib/"
echo "LD_LIBRARY_PATH=$REF/lib $REF/ns3.47-ca_tbl_causal_sim-optimized" > "$REF/RUN.txt"
for a in "$ROOT"/tools/nullcontrol/args/NC*.args; do
    n=$(basename "$a" .args)
    cp "$a" "$REF/nc/"
    mkdir -p "$REF/nc/$n"
    # shellcheck disable=SC2046
    ( cd "$WT/src" && env LD_LIBRARY_PATH="$REF/lib" "$REF/ns3.47-ca_tbl_causal_sim-optimized" \
        $(cat "$a") --outdir="$REF/nc/$n" > "$REF/nc/$n.stdout" 2> "$REF/nc/$n.stderr" )
    echo "$n rc=$?"
done
cp "$ROOT/tools/nullcontrol/nc.sh" "$REF/nc.sh"
{
    echo "rebuilt $(date -u +%FT%TZ) on $(hostname)"
    echo "repo HEAD $COMMIT (the null-control reference of docs/SPEC_CONFORMANCE_PLAN.md)"
    echo "compiler ${CXX:-g++-11}, build profile optimized"
    echo "usage: bash tools/nullcontrol/nc.sh \"\$PWD/build/scratch/ns3.47-ca_tbl_causal_sim-optimized\" <tag>"
    echo "--- sha256"
    (cd "$REF" && sha256sum ns3.47-ca_tbl_causal_sim-optimized lib/*.so)
} > "$REF/PROVENANCE.txt"
echo "reference rebuilt in $REF"
