#!/usr/bin/env bash
# G20 negative fixtures. Rule 6: no gate is trusted without forced-failure evidence.
#
# Every fixture is built outside the tree, in a mktemp directory, from REAL output: two
# short runs are made here and each fixture copies one of them with ONE thing removed or
# falsified, so a gate that only counted rows, or only checked the matrix, or only checked
# that the term moved, would pass most of them. Each must give the exit code stated for it.
#
# N0 matters most: the SAME run with the flag OFF must fail -- "no NLOS score" is the state
# PCS term 3 has been in since it was written. N8 is the recorded finding, kept as a
# fixture: a UWB-only run (legacy topology, where UWB links exist at all) scores every
# decoded UWB link and still cannot fill a confusion matrix, because no decoded UWB link is
# NLOS in the model.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/nlos_classifier_gate.py"
BIN="$ROOT/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
D=$(mktemp -d "${TMPDIR:-/tmp}/g20nlos.XXXXXX") || exit 2
trap 'rm -rf "$D"' EXIT

pass=0
fail=0
check() {  # check <name> <expected exit> <gate args...>
    local name=$1
    local want=$2
    shift 2
    python3 "$GATE" "$@" > "$D/out.txt" 2>&1
    local rc=$?
    if [ "$rc" -eq "$want" ]; then
        printf '  G20 ok    %-66s exit=%d\n' "$name" "$rc"
        # A negative must fail for ITS reason: show it, so a reader can see each one is
        # the fault it names and not whichever check happened to trip first.
        [ "$want" -ne 0 ] && grep -m1 'FAIL' "$D/out.txt" | sed 's/^/            /'
        pass=$((pass + 1))
    else
        printf '  G20 FAIL  %-66s exit=%d expected %d\n' "$name" "$rc" "$want"
        sed 's/^/            /' "$D/out.txt" | head -8
        fail=$((fail + 1))
    fi
}
# edit <in> <out> <python expression over rows `r` (list of dicts) and `hdr`>
edit() {
    python3 - "$1" "$2" "$3" <<'PY'
import csv, sys
src, dst, expr = sys.argv[1], sys.argv[2], sys.argv[3]
with open(src, newline="") as f:
    rd = csv.reader(f)
    hdr = next(rd)
    r = [dict(zip(hdr, row)) for row in rd]
exec(expr)
with open(dst, "w", newline="") as f:
    w = csv.writer(f, lineterminator="\n")
    w.writerow(hdr)
    for row in r:
        w.writerow([row[k] for k in hdr])
PY
}

if [ ! -x "$BIN" ]; then
    printf '  G20 ERROR no binary at %s\n' "$BIN"
    exit 1
fi
mkdir -p "$D/on" "$D/off" "$D/uwb"
"$BIN" --arms=ca_tbl_full --seed=7 --duration=900 --trace=false --nlosClassifier=true \
    --pcsNlosClassifier=true --outdir="$D/on" > "$D/on.log" 2>&1 || { echo "  G20 ERROR flag-on run"; exit 1; }
"$BIN" --arms=ca_tbl_full --seed=7 --duration=900 --trace=false --outdir="$D/off" \
    > "$D/off.log" 2>&1 || { echo "  G20 ERROR flag-off run"; exit 1; }
"$BIN" --arms=static_uwb --seed=7 --duration=1800 --trace=false --topology=1 \
    --nlosClassifier=true --outdir="$D/uwb" > "$D/uwb.log" 2>&1 || { echo "  G20 ERROR uwb run"; exit 1; }
L="$D/on/nlos_links_ca_tbl_full_seed7.csv"
S="$D/on/nlos_summary_ca_tbl_full_seed7.csv"
UL="$D/uwb/nlos_links_static_uwb_seed7.csv"
US="$D/uwb/nlos_summary_static_uwb_seed7.csv"
# The UWB fixtures need a scored UWB row to falsify; without one they would "fail" on a
# missing file, which is not the fault they name.
nuwb=$(awk -F, '$3=="UWB" && $10+0 >= 0' "$UL" 2>/dev/null | wc -l)
if [ "$nuwb" -lt 1 ]; then
    printf '  G20 ERROR the UWB-only run scored no UWB link; N3/P2/N8 cannot be built\n'
    exit 1
fi

# P1 -- positive control: the feed run, its printed block, the feed required.
check "P1 feed run: matrix full on 868, term varies and is fed" 0 --require-fed --stdout "$D/on.log" "$L" "$S"

# P2 -- positive control for the UWB contract alone: the UWB-only run with the done-when
# and the term checks waived is internally consistent (so N3 below fails for its tamper).
edit "$US" "$D/p2s.csv" "
for x in r:
    if x['kind']=='pcs': x.update(fixCycles='1', termCycles='1', fedCycles='0', distinct='2', changes='1', interior='1', min='0.5', max='1')"
check "P2 UWB rows honour score = 1 - A_le/A_peak (min-cell 0, term waived)" 0 --min-cell 0 "$UL" "$D/p2s.csv"

# N0 -- the same run with the flag OFF: nothing is constructed and no file is written.
check "N0 same run, flag off (no NLOS files)" 1 "$D/off/nlos_links_ca_tbl_full_seed7.csv" "$D/off/nlos_summary_ca_tbl_full_seed7.csv"

# N1 -- the links file exists and holds no rows. A non-empty test would pass this.
head -1 "$L" > "$D/n1.csv"
check "N1 links header only, zero rows" 1 "$D/n1.csv" "$S"

# N2 -- the summary's matrix is not what its rows add up to.
edit "$S" "$D/n2.csv" "
for x in r:
    if x['kind']=='confusion' and x['tier']=='868': x['tp'] = str(int(x['tp']) + 1)"
check "N2 summary 868 tp one more than the rows" 1 "$L" "$D/n2.csv"

# N3 -- a UWB row whose score is not 1 - A_le/A_peak.
edit "$UL" "$D/n3.csv" "
k = next(i for i, x in enumerate(r) if x['tier']=='UWB' and float(x['score']) >= 0)
r[k]['cirALe'] = str(float(r[k]['cirAPeak']) * 0.3)"
check "N3 UWB row with A_le/A_peak = 0.3 but its score unchanged" 1 --min-cell 0 "$D/n3.csv" "$D/p2s.csv"

# N4 -- a verdict that disagrees with its score and the threshold.
edit "$L" "$D/n4.csv" "
k = next(i for i, x in enumerate(r) if x['verdict']=='0')
r[k]['verdict'] = '1'"
check "N4 a score under threshold with verdict NLOS" 1 "$D/n4.csv" "$S"

# N5 -- a score outside [0,1].
edit "$L" "$D/n5.csv" "
k = next(i for i, x in enumerate(r) if x['verdict']=='1')
r[k]['score'] = '1.2'"
check "N5 a score of 1.2" 1 "$D/n5.csv" "$S"

# N6 -- a term that never moves: one distinct value, no change.
edit "$S" "$D/n6.csv" "
for x in r:
    if x['kind']=='pcs': x.update(distinct='1', changes='0', interior='0', min='1', max='1')"
check "N6 PCS term constant over the run" 1 "$L" "$D/n6.csv"

# N7 -- the diagnostic-only run offered as a feed run: the term was never fed.
edit "$S" "$D/n7.csv" "
for x in r:
    if x['kind']=='pcs': x['fedCycles'] = '0'"
check "N7 --require-fed on a summary fed on 0 cycles" 1 --require-fed "$L" "$D/n7.csv"
check "N7b --forbid-fed on the feed run" 1 --forbid-fed "$L" "$S"

# N8 -- the finding: UWB-only, every decoded UWB link LOS, no matrix to fill.
check "N8 UWB-only run: no tier has four non-trivial cells" 1 "$UL" "$US"

# N9 -- the printed block disagrees with the file (rule 3).
sed 's/\(868 rssi_snr_variability: tp \)\([0-9]*\)/\1999999/' "$D/on.log" > "$D/n9.log"
check "N9 printed 868 tp disagrees with the summary" 1 --stdout "$D/n9.log" "$L" "$S"

# N10 -- more cycles fed than carried a term.
edit "$S" "$D/n10.csv" "
for x in r:
    if x['kind']=='pcs': x['fedCycles'] = str(int(x['termCycles']) + 1)"
check "N10 fedCycles > termCycles" 1 "$L" "$D/n10.csv"

# N11 -- G20's classifier-alone identity test (cmp of arm_summary.csv) must be able to fail.
# The feed run changes decisions, so against the flag off it must differ; if cmp called
# these two identical, the test could not see a classifier that moved a decision.
if cmp -s "$D/off/arm_summary.csv" "$D/on/arm_summary.csv"; then
    printf '  G20 FAIL  %-66s cmp found no difference\n' "N11 identity test on the feed run's arm_summary.csv"
    fail=$((fail + 1))
else
    printf '  G20 ok    %-66s differs\n' "N11 identity test on the feed run's arm_summary.csv"
    pass=$((pass + 1))
fi

printf '  G20 selftest: %d as expected, %d not\n' "$pass" "$fail"
[ "$fail" -eq 0 ]
