#!/usr/bin/env bash
# Regenerate per-cycle traces from the sidecars that record how they were made.
#
# WHY THIS EXISTS. `results/` is 18 GB, of which 11 MB is tracked in git and the rest
# is per-cycle `trace_*.csv` that the .gitignore deliberately excludes. Deleting the
# directory loses only the untracked part, and this regenerates what can be
# regenerated -- which is NOT all of it, and the exclusions are enumerated in
# RESULTS_REPRODUCIBILITY.md rather than discovered at run time.
#
#   bash scripts/regenerate_results.sh --list          what would run, and what cannot
#   bash scripts/regenerate_results.sh --dir <path>    one directory
#   bash scripts/regenerate_results.sh --all           every reproducible directory
#   bash scripts/regenerate_results.sh --verify <path> re-run and DIFF against what is on disk
#
# Each directory's `run_id.txt` line 2 holds the full invocation, including the build
# date and the source commit. The invocation is replayed verbatim with only --outdir
# rewritten.
#
# REPRODUCIBILITY IS NOT UNIFORM, and the governing fact is measured rather than
# assumed: directories built BEFORE the D1 sensitivity-table correction (0d5e747,
# 2026-08-26) do not reproduce under the current code, because D1 changed the receiver
# sensitivity tables that every packet outcome depends on. Directories built after it
# reproduce byte-identically even when their sidecar records a non-zero dirty counter,
# provided those uncommitted changes were subsequently committed.

set -u
cd "$(dirname "$0")/.." || exit 2
BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
D1=0d5e747

[ -x "$BIN" ] || { echo "no binary at $BIN -- run: ./ns3 build ca_tbl_causal_sim"; exit 2; }

classify() {  # classify <dir> -> prints CLASS
    local d=$1 p="$1/run_id.txt"
    [ -f "$p" ] || { echo "NO_SIDECAR"; return; }
    local src; src=$(grep -m1 '^source ' "$p" 2>/dev/null)
    [ -n "$src" ] || { echo "NO_SOURCE_LINE"; return; }
    local sha; sha=$(echo "$src" | awk '{print $2}')
    grep -q 'ns3.47-ca_tbl_causal_sim-optimized' "$p" || { echo "NO_INVOCATION"; return; }
    if git merge-base --is-ancestor "$sha" "$D1" 2>/dev/null; then echo "PRE_D1"; else echo "OK"; fi
}

replay() {  # replay <dir> <outdir>
    local inv
    inv=$(sed -n '2p' "$1/run_id.txt" | sed 's/.*ns3.47-ca_tbl_causal_sim-optimized//')
    inv=$(echo "$inv" | sed "s|--outdir=[^ ]*|--outdir=$2|")
    mkdir -p "$2"
    eval "$BIN $inv" > "$2/stdout.txt" 2>&1
}

dirs_with_traces() {
    find results -name 'trace_*.csv' -printf '%h\n' 2>/dev/null | sort -u
}

case "${1:---list}" in
  --list)
    declare -A n
    while read -r d; do c=$(classify "$d"); n[$c]=$(( ${n[$c]:-0} + 1 )); done < <(dirs_with_traces)
    echo "trace-bearing directories, by regeneration class:"
    for k in OK PRE_D1 NO_SIDECAR NO_SOURCE_LINE NO_INVOCATION; do
        printf "   %-16s %5d %s\n" "$k" "${n[$k]:-0}" \
          "$([ "$k" = OK ] && echo '<- regenerable' || echo '<- NOT regenerable')"
    done
    ;;
  --dir)
    d=$2; c=$(classify "$d")
    [ "$c" = OK ] || { echo "$d: $c -- not regenerable, see RESULTS_REPRODUCIBILITY.md"; exit 1; }
    replay "$d" "$d" && echo "regenerated $d"
    ;;
  --verify)
    d=$2; c=$(classify "$d")
    [ "$c" = OK ] || { echo "$d: $c -- not regenerable"; exit 1; }
    t=$(mktemp -d); trap 'rm -rf "$t"' EXIT
    replay "$d" "$t"
    a=$(sha256sum "$d/arm_summary.csv" | cut -d' ' -f1)
    b=$(sha256sum "$t/arm_summary.csv" 2>/dev/null | cut -d' ' -f1)
    if [ "$a" = "$b" ]; then echo "$d  IDENTICAL"; else echo "$d  DIFFERS"; exit 1; fi
    ;;
  --all)
    ok=0; skip=0
    while read -r d; do
      if [ "$(classify "$d")" = OK ]; then replay "$d" "$d"; ok=$((ok+1)); else skip=$((skip+1)); fi
    done < <(dirs_with_traces)
    echo "regenerated $ok, skipped $skip (not regenerable)"
    ;;
  *) echo "unknown option: $1"; exit 2 ;;
esac
