#!/usr/bin/env bash
# G12b -- negative fixture, four directions.
#
# G12b asserts no tracked file's CONTENT carries a vendor identity. A gate that has
# only ever been observed passing is untested, and this one passes on the real tree
# from the first run, which is exactly the condition under which a gate that cannot
# fail goes unnoticed.
#
# Four directions, because two is not enough for this gate:
#   1. real tree                          -> PASS   (the gate is not vacuous)
#   2. a tracked file naming a vendor      -> FAIL   (it detects the thing)
#   3. prose naming a vendor inside the
#      gate file, which the scan skips     -> FAIL   (the exemption is paid for)
#   4. a vendor token inside a larger
#      identifier, `Gdk.Cursor`-style      -> PASS   (form, not token -- no false failure)
#
# Direction 4 is the one that matters most. G12's first version failed on the commit
# that documented the rule, and a gate that fails on its own policy is a gate that
# gets disabled. Synthetic trees live in a mktemp directory and are removed on exit.

set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2

GATE=scripts/catbl_gates.sh
TMP=$(mktemp -d)
trap 'rm -rf "$TMP"' EXIT
RC=0

# Extract just G12b's logic into a runnable probe against an arbitrary repo, so the
# fixture tests the gate's own text rather than a paraphrase of it.
sed -n '/^# ------------- G12b/,/^finish G12b$/p' "$GATE" \
  | sed 's/^\[ "\$G12B_BAD" -eq 0 \] || fail G12b$/[ "$G12B_BAD" -eq 0 ] || exit 1/' \
  | sed '/^finish G12b$/d' > "$TMP/probe.sh"
if ! grep -q 'G12B_PAT=' "$TMP/probe.sh"; then
    printf 'FIXTURE BROKEN: could not extract G12b from %s\n' "$GATE"; exit 2
fi

mkrepo() {  # mkrepo <dir> -- a git repo with the gate file present
    rm -rf "$1"; mkdir -p "$1/scripts"
    cp "$GATE" "$1/scripts/catbl_gates.sh"
    ( cd "$1" && git init -q . && git config user.email f@f && git config user.name f )
}

run_probe() {  # run_probe <dir> -- 0 = gate passes, 1 = gate fails
    ( cd "$1" && bash "$TMP/probe.sh" >"$TMP/out" 2>&1 ) && return 0 || return 1
}

check() {  # check <label> <expect pass|fail> <dir>
    local label=$1 expect=$2 dir=$3 got
    if run_probe "$dir"; then got=pass; else got=fail; fi
    if [ "$got" = "$expect" ]; then
        printf '  OK   %-58s expected %s, got %s\n' "$label" "$expect" "$got"
    else
        printf '  BAD  %-58s expected %s, got %s\n' "$label" "$expect" "$got"
        sed 's/^/         /' "$TMP/out" | head -6
        RC=1
    fi
}

printf 'G12b fixture -- four directions\n\n'

# 1. real tree
printf '  ...direction 1: the real repository\n'
# Captured to a file, not piped: `grep -q` exits early, and under `pipefail` the
# SIGPIPE it sends the gate turns a passing gate into a failing pipeline.
bash "$GATE" >"$TMP/suite" 2>&1 || true
if grep -q '=== G12b PASS ===' "$TMP/suite"; then
    printf '  OK   %-58s expected pass, got pass\n' 'real tree'
else
    printf '  BAD  %-58s expected pass, got fail\n' 'real tree'; RC=1
fi

# 2. a tracked file naming a vendor
mkrepo "$TMP/r2"
printf 'This document was written by %s at the request of the team.\n' 'an assistant' > "$TMP/r2/NOTES.md"
( cd "$TMP/r2" && git add scripts/catbl_gates.sh NOTES.md && git commit -qm x )
check 'tracked file asserting vendor authorship' fail "$TMP/r2"

# 3. prose smuggled into the gate file itself, which the content scan skips
mkrepo "$TMP/r3"
printf '# a note about %s that is prose, not a matcher\n' 'OpenAI' >> "$TMP/r3/scripts/catbl_gates.sh"
( cd "$TMP/r3" && git add scripts/catbl_gates.sh && git commit -qm x )
check 'prose inside the gate file the scan exempts' fail "$TMP/r3"

# 4. the token inside a larger identifier -- must NOT fail
mkrepo "$TMP/r4"
{ printf 'cursor = Gdk.Cursor.new_for_display(display)\n'
  printf 'double vendorThreshold = 1.55;\n'
  printf 'cfg = load(x.openai_compat)\n'; } > "$TMP/r4/upstream.py"
( cd "$TMP/r4" && git add scripts/catbl_gates.sh upstream.py && git commit -qm x )
check 'vendor token inside a larger identifier' pass "$TMP/r4"

printf '\n'
if [ "$RC" -eq 0 ]; then printf 'G12b FIXTURE PASS -- all four directions\n'
else printf 'G12b FIXTURE FAIL\n'; fi
exit "$RC"
