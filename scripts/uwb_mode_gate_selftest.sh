#!/usr/bin/env bash
# G164 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/uwb_mode_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g164self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/uwb_mode_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -d "$REAL/p_6m8" ] || { echo "  G164 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G164 FAIL +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G164 ok    %-62s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G164 FAIL  %-62s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
# edit one column of a probe csv on the rows whose tier matches (or does not, with !)
probe_set() {
    python3 - "$@" <<'P'
import csv, sys
p, tier, col, val = sys.argv[1:5]
rs = list(csv.DictReader(open(p))); f = list(rs[0].keys())
for r in rs:
    if (r["tier"] == tier) if not tier.startswith("!") else (r["tier"] != tier[1:]):
        r[col] = val if not val.startswith("+") else f"{float(r[col]) + float(val[1:]):.6f}"
w = csv.DictWriter(open(p, "w", newline=""), fieldnames=f); w.writeheader(); w.writerows(rs)
P
}
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/e_6m8.rc"; check "N1 a mode's run crashed" 1 RUNS
fresh; probe_set "$T/f/p_6m8/probe_range_sweep.csv" UWB mean_nominal_margin_db +10.8; check "N2 6m8 not setting its sensitivity" 1 SENS
fresh; probe_set "$T/f/p_850/probe_range_sweep.csv" '!UWB' mean_nominal_margin_db +1.0; check "N3 850k leaking into the other tiers" 1 SENS
fresh; probe_set "$T/f/p_6m8/probe_range_sweep.csv" UWB coherent_gain_db 30.1; check "N4 6m8 still integrating 1024 symbols" 1 RANGE
fresh; sed -i 's/a 20-byte frame bills [0-9.eE+-]* s/a 20-byte frame bills 0.00019129 s/' "$T/f/e_850.log"; check "N5 850k frames billed at the legacy frame time" 1 BILL
fresh; sed -i 's/a 20-byte frame bills [0-9.eE+-]* s//' "$T/f/e_6m8.log"; check "N6 the billed frame time not printed" 1 BILL
fresh; f=$(ls "$T/f/def" | grep -v run_id.txt | head -1); echo x >> "$T/f/def/$f"; check "N7 legacy not being the default" 1 OFF
fresh; echo 0 > "$T/f/r_both.rc"; check "N8 a mode accepted with a contradicting sensitivity" 1 REFUSE
echo "  G164 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
