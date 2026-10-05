#!/bin/bash
# Null control against the FROZEN spec0 reference (HEAD e6b3da9; see PROVENANCE.txt).
#
# usage:  bash tools/nullcontrol/nc.sh '<candidate run prefix>' <tag>
#   e.g.  bash tools/nullcontrol/nc.sh "$(cat ~/catbl_ref_spec0/RUN.txt)" self
#         bash tools/nullcontrol/nc.sh "$PWD/build/scratch/ns3.47-ca_tbl_causal_sim-optimized" cand1
#
# Runs NC1 and NC2 (arguments in <ref>/NC{1,2}.args) with the candidate into
# ~/catbl_nc/<tag>/NC{1,2}, then cmp's EVERY output file against the reference.
# run_id.txt is excluded from the verdict (its id / invocation / source lines
# legitimately differ: argv carries the outdir, the build stamp and git HEAD move)
# but it is diffed and the differing lines are shown. Its sha256 block is NOT
# expected to differ: a differing digest line means an output differs, which the
# per-file cmp reports anyway.
#
# Verdict: PASS only if both runs exit 0, the two directories hold the same set of
# files, and every file except run_id.txt is byte-identical.
#
# Env overrides (for the negative fixture and for speed):
#   NC_REF_DIR       reference dir holding NC1/ NC2/ NC{1,2}.args (default ~/catbl_ref_edge/nc)
#   NC_OUT_ROOT      where candidate outputs go (default ~/catbl_nc)
#   NC_CWD           cwd for the run; run_id.txt's "source" line is git HEAD of this
#                    dir (default: the repo)
#   NC_ONLY          NC1 or NC2 to run just one (default both)
#   NC_COMPARE_ONLY  1 = do not run, compare existing ~/catbl_nc/<tag>/NC* outputs
#
# Never writes into the reference directory.
set -u
if [ $# -ne 2 ]; then
    echo "usage: bash $0 '<candidate run prefix>' <tag>" >&2
    exit 2
fi
PREFIX=$1
TAG=$2
REFD=${NC_REF_DIR:-$HOME/catbl_ref_spec0/nc}
OUTROOT=${NC_OUT_ROOT:-$HOME/catbl_nc}
RUNCWD=${NC_CWD:-$(cd "$(dirname "$0")/../.." && pwd)}
ONLY=${NC_ONLY:-both}
CMPONLY=${NC_COMPARE_ONLY:-0}

case "$TAG" in ""|*/*|.|..) echo "bad tag '$TAG'" >&2; exit 2;; esac
OUTD="$OUTROOT/$TAG"
case "$OUTD/" in "$REFD"/*) echo "refusing: candidate outdir $OUTD is inside the reference $REFD" >&2; exit 2;; esac

if [ "$ONLY" = both ]; then CASES=$(cd "$REFD" && ls NC*.args | sed "s/.args$//" | sort -V | tr "\n" " "); else CASES=$(echo "$ONLY" | tr "," " "); fi

fail=0
echo "null control: tag=$TAG ref=$REFD out=$OUTD cwd=$RUNCWD"
echo "candidate prefix: $PREFIX"

for NC in $CASES; do
    ref="$REFD/$NC"
    cand="$OUTD/$NC"
    if [ ! -f "$REFD/$NC.args" ] || [ ! -d "$ref" ]; then
        echo "[$NC] reference missing ($ref or $NC.args)"; fail=1; continue
    fi
    args=$(cat "$REFD/$NC.args")
    if [ "$CMPONLY" != 1 ]; then
        rm -rf "$cand"; mkdir -p "$cand"
        t0=$(date +%s.%N)
        ( cd "$RUNCWD" && env $PREFIX $args --outdir="$cand" > "$OUTD/$NC.stdout" 2> "$OUTD/$NC.stderr" )
        rc=$?
        t1=$(date +%s.%N)
        wall=$(awk -v a="$t0" -v b="$t1" 'BEGIN{printf "%.1f", b-a}')
        echo "[$NC] run rc=$rc wall=${wall}s args: $args"
        if [ $rc -ne 0 ]; then
            echo "[$NC] FAIL: candidate exited $rc; stderr tail:"; tail -5 "$OUTD/$NC.stderr" | sed 's/^/    /'
            fail=1
        fi
    else
        echo "[$NC] compare-only (no run)"
    fi
    if [ ! -d "$cand" ]; then echo "[$NC] FAIL: no candidate dir $cand"; fail=1; continue; fi

    # union of file names, relative paths
    names=$( { (cd "$ref" && find . -type f | sed 's|^\./||'); (cd "$cand" && find . -type f | sed 's|^\./||'); } | LC_ALL=C sort -u )
    nid=0; ndiff=0; nmiss=0; nextra=0
    for f in $names; do
        [ "$f" = run_id.txt ] && continue
        if [ ! -f "$ref/$f" ]; then
            echo "[$NC] EXTRA      $f  (candidate only)"; nextra=$((nextra+1)); continue
        fi
        if [ ! -f "$cand/$f" ]; then
            echo "[$NC] MISSING    $f  (reference only)"; nmiss=$((nmiss+1)); continue
        fi
        if cmp -s "$ref/$f" "$cand/$f"; then
            echo "[$NC] IDENTICAL  $f"; nid=$((nid+1))
        else
            echo "[$NC] DIFFERENT  $f  ($(cmp "$ref/$f" "$cand/$f" 2>&1 | head -1))"; ndiff=$((ndiff+1))
        fi
    done
    echo "[$NC] summary: identical=$nid different=$ndiff missing=$nmiss extra=$nextra (run_id.txt excluded)"
    [ $ndiff -ne 0 ] || [ $nmiss -ne 0 ] || [ $nextra -ne 0 ] && fail=1

    # run_id.txt: informational diff, never part of the verdict
    if [ -f "$ref/run_id.txt" ] && [ -f "$cand/run_id.txt" ]; then
        d=$(diff "$ref/run_id.txt" "$cand/run_id.txt")
        if [ -z "$d" ]; then
            echo "[$NC] run_id.txt: identical"
        else
            echo "[$NC] run_id.txt differs (informational; line numbers from diff):"
            echo "$d" | sed 's/^/    /'
            # a differing sha256 line is not expected unless an output differs
            nsha=$(echo "$d" | grep -E '^[<>] [0-9a-f]{64}  ' | wc -l)
            echo "[$NC] run_id.txt: differing sha256 lines=$nsha (expected 0 when outputs match)"
        fi
    else
        echo "[$NC] run_id.txt missing on one side (ref: $([ -f "$ref/run_id.txt" ] && echo yes || echo no), cand: $([ -f "$cand/run_id.txt" ] && echo yes || echo no))"
    fi
done

if [ $fail -eq 0 ]; then
    echo "NULL CONTROL: PASS"
    exit 0
else
    echo "NULL CONTROL: FAIL"
    exit 1
fi
