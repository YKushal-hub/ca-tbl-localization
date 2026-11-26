#!/usr/bin/env bash
# Prune finished runs (rc present) of a campaign dir to what the analysers and figures read, after recording the
# SHA-256 of every file into <campaign>/RAW_MANIFEST.sha256. Usage: prune_runs.sh <campaign dir> [--loop]
C=${1:?}; LOOP=${2:-}
prune_once() {
    for rc in "$C"/*/s*/rc; do
        [ -f "$rc" ] || continue
        d=$(dirname "$rc")
        [ -f "$d/.pruned" ] && continue
        ( cd "$C" && find "${d#$C/}" -type f -print0 | sort -z | xargs -0 sha256sum ) >> "$C/RAW_MANIFEST.sha256" || continue
        find "$d" -type f ! -name 'trace_*' ! -name 'nodes_*' ! -name 'subjtrace_*' ! -name 'subject_summary.csv' \
             ! -name 'arm_summary.csv' ! -name 'run.log' ! -name 'run_id.txt' ! -name 'rc' ! -name 'deployment.json' \
             ! -name 'spec_profile.txt' -delete
        touch "$d/.pruned"
    done
}
prune_once
if [ "$LOOP" = --loop ]; then
    while true; do sleep 60; prune_once; grep -q done "$C/RUN.md" 2>/dev/null && { prune_once; break; }; done
fi
