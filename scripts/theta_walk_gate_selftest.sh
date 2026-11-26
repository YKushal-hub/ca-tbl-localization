#!/usr/bin/env bash
# G187 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
GATE="$(pwd)/scripts/theta_walk_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g187self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash scripts/theta_walk_gate.sh "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/walk" ] || { echo "  G187 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; mkdir "$T/f"; cp "$REAL"/*.rc "$REAL"/*.log "$T/f/"; for n in off false; do cp -r "$REAL/$n" "$T/f/"; done; for n in base walk; do mkdir "$T/f/$n"; cp "$REAL/$n"/subjtrace_* "$T/f/$n/"; done; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G187 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G187 ok    %-58s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G187 FAIL  %-58s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/walk.rc"; check "N1 a run crashed" 1 RUNS
fresh; echo 0 > "$T/f/r_cv.rc"; check "N2 accepted without the smoother" 1 FATAL
fresh; echo x >> "$T/f/false/trace_ca_tbl_full_seed732.csv"; check "N3 an explicit default changes an output" 1 OFFSAME
fresh; rm "$T/f/false/trace_ca_tbl_full_seed732.csv"; check "N4 an output missing" 1 OFFSAME
fresh; sed -i '/DD59 ekf5WindowThetaWalk/d' "$T/f/walk.log"; check "N5 not announced" 1 LINES
fresh; echo "DD59 ekf5WindowThetaWalk" >> "$T/f/base.log"; check "N6 on in the control" 1 LINES
fresh; f=$(ls "$T"/f/walk/subjtrace_* | head -1); sed -i '$d' "$f"; check "N7 a truncated crew trace" 1 ROWS
fresh; cp "$T"/f/base/subjtrace_* "$T/f/walk/"; check "N8 coverage does not rise" 1 COVER
fresh; awk -F, -v OFS=, 'NR>1{$11=400;$13=1e6}1' "$(ls "$T"/f/walk/subjtrace_* | head -1)" > "$T/z"; for g in "$T"/f/walk/subjtrace_*; do cp "$T/z" "$g"; done; check "N9 coverage bought with accuracy" 1 ACC
fresh; rm "$T"/f/walk/subjtrace_*; check "N10 no crew traces (vacuous)" 1 COVER
echo "  G187 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
