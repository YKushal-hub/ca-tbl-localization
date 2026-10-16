#!/usr/bin/env bash
# G92 fixture -- the cited-evidence gate, every direction.
#
# Builds a throwaway git tree in a temp directory (never the repository: verification
# must not mutate its subject) holding one tracked results directory with a sidecar, one
# results directory present but untracked, one recorded as lost, and documents citing
# each. Each direction asserts the verdict AND that a failure names the path it failed on.

set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2
GATE="$(pwd)/scripts/cited_evidence_gate.py"
TMP=$(mktemp -d "${TMPDIR:-/tmp}/catbl_g18.XXXXXX"); trap 'rm -rf "$TMP"' EXIT
RC=0

T="$TMP/tree"
mkdir -p "$T/results/good_run" "$T/results/stray_run" "$T/report"
(
    cd "$T" || exit 2
    git init -q . && git config user.email fixture@localhost && git config user.name fixture
    printf 'arm,seed\nx,1\n' > results/good_run/arm_summary.csv
    printf 'id\nsource abc dirty 0 profile optimized\n' > results/good_run/run_id.txt
    printf 'arm,seed\nx,1\n' > results/stray_run/arm_summary.csv      # on disk, never tracked
    printf '# path\treason\nresults/gone_run\tPRE_D1\n' > RESULTS_NONREGENERABLE.tsv
    printf 'ok: `results/good_run`\n' > report/cites_good.md
    printf 'bad: `results/never_existed`\n' > report/cites_absent.md
    printf 'bad: `results/stray_run`\n' > report/cites_stray.md
    printf 'known: `results/gone_run`\n' > report/cites_lost.md
    printf 'pattern: `results/good_*`\n' > report/cites_pattern.md
    printf 'not a citation: `src/study/results/never_existed`\n' > report/cites_nested.md
    git add results/good_run report RESULTS_NONREGENERABLE.tsv
    git commit -q -m fixture
) || { echo "FIXTURE BROKEN: could not build the temp tree"; exit 2; }

expect() {  # expect <label> <want-exit> <must-contain-or-empty> <docs...>
    label=$1; want=$2; needle=$3; shift 3
    out=$(python3 "$GATE" --root "$T" --docs "$@" 2>&1); got=$?
    if [ "$got" -ne "$want" ]; then
        printf '  BAD  %-34s expected exit %s, got %s\n' "$label" "$want" "$got"
        printf '%s\n' "$out" | sed 's/^/         /'; RC=1; return
    fi
    if [ -n "$needle" ] && ! printf '%s\n' "$out" | grep -q -- "$needle"; then
        printf '  BAD  %-34s exit right, but output does not name %s\n' "$label" "$needle"
        RC=1; return
    fi
    printf '  OK   %-34s exit %s%s\n' "$label" "$got" "${needle:+, names $needle}"
}

printf 'G92 fixture -- six directions\n\n'
expect "tracked with sidecar passes"     0 ""                                report/cites_good.md
expect "glob citation resolves"          0 ""                                report/cites_pattern.md
expect "nested results/ path ignored"    0 ""                                report/cites_nested.md
expect "recorded loss reported, passes"  0 "RECORDED_LOST"                   report/cites_lost.md
expect "absent directory fails"          1 "ABSENT            results/never_existed" report/cites_absent.md
expect "untracked on disk fails"         1 "UNTRACKED_ON_DISK results/stray_run"     report/cites_stray.md

if [ "$RC" -eq 0 ]; then printf '\nG92 fixture: all directions behaved\n'; else printf '\nG92 fixture: FAILED\n'; fi
exit "$RC"
