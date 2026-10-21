#!/usr/bin/env bash
# G161 negative fixtures (rule 6). Each is a copy of a REAL run set with ONE thing changed.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd); GATE="$ROOT/scripts/receiver_gate.py"
T=$(mktemp -d "${TMPDIR:-/tmp}/g161self.XXXXXX") || exit 2
trap 'rm -rf "$T"' EXIT
if [ $# -ge 1 ]; then REAL=$1; else REAL="$T/real"; bash "$ROOT/scripts/receiver_gate.sh" "$REAL" > "$T/real.out" 2>&1; fi
[ -f "$REAL/p_def/probe_range_sweep.csv" ] || { echo "  G161 selftest: no real run set in $REAL"; exit 2; }
pass=0; fail=0
fresh() { rm -rf "$T/f"; cp -r "$REAL" "$T/f"; }
check() {
    python3 "$GATE" "$T/f" > "$T/out.txt" 2>&1
    local rc=$? reason=1
    if [ "$3" != - ]; then grep -Eq "G161 (FAIL|MISSING) +$3 " "$T/out.txt" || reason=0; fi
    if [ "$rc" -eq "$2" ] && [ "$reason" -eq 1 ]; then printf '  G161 ok    %-60s exit=%d\n' "$1" "$rc"; pass=$((pass + 1));
    else printf '  G161 FAIL  %-60s exit=%d expected %d, failing check %s\n' "$1" "$rc" "$2" "$3"; fail=$((fail + 1)); fi
}
# column index of a named column in the probe CSV
col() { head -1 "$1" | tr ',' '\n' | grep -nx "$2" | cut -d: -f1; }
fresh; check "P0 the real run set passes" 0 -
fresh; echo 134 > "$T/f/p_iq.rc"; check "N1 a probe crashed under a flag" 1 RUNS
fresh; f="$T/f/p_def/probe_range_sweep.csv"; c=$(col "$f" mean_iq_degradation_db); cut -d, -f"$c" --complement "$f" > "$f.t" && mv "$f.t" "$f"; check "N2 the IQ column missing" 1 COLUMNS
fresh; f="$T/f/p_def/probe_range_sweep.csv"; c=$(col "$f" mean_iq_degradation_db); awk -F, -v c="$c" 'BEGIN{OFS=","} NR==2{$c="0.00"} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N3 a control-flow zero in the IQ term" 1 REACHED
fresh; cp "$T/f/p_def/probe_range_sweep.csv" "$T/f/p_uwb/"; check "N4 the sensitivity flag not acting" 1 UWBSENS
fresh; f="$T/f/p_uwb/probe_range_sweep.csv"; awk -F, 'BEGIN{OFS=","} $1=="868MHz" && !d{$3="0.1234"; d=1} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N5 the sensitivity flag leaking into tier 1" 1 UWBSENS
fresh; cp "$T/f/p_def/probe_range_sweep.csv" "$T/f/p_iq/"; check "N6 --iqOverloadOnly not removing the term" 1 IQOFF
fresh; f="$T/f/p_iq/probe_range_sweep.csv"; c=$(col "$f" mean_nominal_margin_db); awk -F, -v c="$c" 'BEGIN{OFS=","} NR>1{$c=sprintf("%.2f",$c+3)} {print}' "$f" > "$f.t" && mv "$f.t" "$f"; check "N6b --iqOverloadOnly removing 3 dB more than the IQ term" 1 IQOFF
fresh; f=$(ls "$T/f/zero" | grep -v run_id.txt | head -1); echo x >> "$T/f/zero/$f"; check "N7 a default that is not the old engine" 1 OFF
fresh; sed -i 's/^iqOverloadOnly=true/iqOverloadOnly=false/' "$T/f/pre/spec_profile.txt"; check "N7b the preset not carrying the IQ correction" 1 PRESET
fresh; sed -i 's/^uwbMode=850k/uwbMode=legacy/' "$T/f/pre/spec_profile.txt"; check "N7c the preset not carrying DD 23's UWB row" 1 PRESET
fresh; echo 0 > "$T/f/r_low.rc"; check "N8 accepted a sensitivity below -130" 1 REFUSE
fresh; echo 0 > "$T/f/r_high.rc"; check "N9 accepted a sensitivity above -60" 1 REFUSE
echo "  G161 selftest: $pass as expected, $fail not"
[ "$fail" -eq 0 ]
