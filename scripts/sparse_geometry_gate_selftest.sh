#!/usr/bin/env bash
# G184 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
GATE="$(pwd)/scripts/sparse_geometry_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g184self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash scripts/sparse_geometry_gate.sh "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/on" ] || { echo "  G184 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G184 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G184 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G184 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/on.rc"; check "N1 the on run crashed" 1 RUNS
fresh; echo x >> "$T/f/false/trace_ca_tbl_full_seed701.csv"; check "N2 the explicit default changes an output" 1 OFFSAME
fresh; rm "$T/f/false/trace_ca_tbl_full_seed701.csv"; check "N3 the explicit default drops an output" 1 OFFSAME
fresh; sed -i '/DD54/d' "$T/f/on.log"; check "N4 the flag not announced" 1 LINE
fresh; grep DD54 "$T/f/on.log" >> "$T/f/off.log"; check "N5 the flag announced with it off" 1 LINE
fresh; echo 0 > "$T/f/fatal.rc"; check "N6 the flag accepted without --sparseInStep" 1 FATAL
fresh; cp "$T"/f/off/subjtrace_* "$T/f/on/"; check "N7 the jumps still there with it on" 1 JUMP
fresh; cp "$T"/f/on/subjtrace_* "$T/f/off/"; check "N8 no defect to remove (off already clean)" 1 JUMP
fresh; cp "$T"/f/off/subjtrace_* "$T/f/on/"; check "N9 coverage does not rise" 1 COVER
fresh; rm "$T"/f/on/subjtrace_*; check "N10 no crew traces (vacuous)" 1 COVER
echo "  G184 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
