#!/usr/bin/env bash
# G10 fixture: scripts/citation_audit.py must pass a clean tree and fail each way a citation can
# be unresolved -- including an untracked file NOT on the enumerated allowance list beside one
# that is, which proves the list is an enumeration and not a pattern. (G10 had no negative
# fixture, against working rule 6; ported from the local line 2026-10-16.) Each case is a throwaway git repo
# under $TMPDIR; the real tree is never touched (working rule 7).
set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2
AUDIT="$PWD/scripts/citation_audit.py"
TMP=$(mktemp -d "${TMPDIR:-/tmp}/catbl_g10.XXXXXX"); trap 'rm -rf "$TMP"' EXIT
RC=0

mkrepo() {  # mkrepo <dir> <citation line>
    mkdir -p "$1/report" "$1/src"
    ( cd "$1" && git init -q && git config user.email f@x && git config user.name fixture
      echo 'int x;' > src/model.cc
      printf '# fixture\n\nSee `src/model.cc`. %s\n' "$2" > report/NOTE.md
      git add src report && git commit -q -m fixture )
}
expect() {  # expect <pass|fail> <name> <citation> [setup command run inside the repo]
    local want=$1 name=$2 cite=$3 setup=${4:-true} d="$TMP/$2" got
    mkrepo "$d" "$cite"
    ( cd "$d" && eval "$setup" )
    if ( cd "$d" && python3 "$AUDIT" > out.txt 2>&1 ); then got=pass; else got=fail; fi
    if [ "$got" = "$want" ]; then
        printf '  OK   %-28s expected %s, got %s\n' "$name" "$want" "$got"
    else
        printf '  BAD  %-28s expected %s, got %s\n' "$name" "$want" "$got"; sed 's/^/       /' "$d/out.txt"; RC=1
    fi
}
printf 'G10 fixture -- the clean tree and an allow-listed file pass; every unresolved form fails\n\n'
expect pass clean                ''
expect fail untracked_on_disk    'Also `report/local_notes.md`.'          'echo x > report/local_notes.md'
expect fail absent               'Data in `gone_campaign.csv`.'
expect pass allowed_untracked    'See `SESSION_CHANGELOG.md`.'            'echo x > SESSION_CHANGELOG.md'
expect fail unlisted_work_order  'Flags in `WORK_ORDER_PROMPT_OTHER.md`.' 'echo x > WORK_ORDER_PROMPT_OTHER.md'
expect fail untracked_results    'Source: `results/some_campaign`.'       'mkdir -p results/some_campaign'
[ "$RC" -eq 0 ] && printf '\nG10 fixture: all directions behaved\n' || printf '\nG10 fixture: FAILED\n'
exit "$RC"
