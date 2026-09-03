#!/usr/bin/env bash
# G16 -- concurrency detector fixture, both directions.
#
# The legitimate direction is the one that gets forgotten, so it is tested first and
# by name. A fixture that fails by crashing is not a fixture: the failing direction
# must fail with a message that NAMES the pair and the count, and that is asserted
# here rather than assumed.

set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2

BIN=./build/scratch/ns3.47-ca_tbl_causal_sim-optimized
GATE=scripts/catbl_concurrency_gate.py
TMP=$(mktemp -d); trap 'rm -rf "$TMP"' EXIT
RC=0
COMMON="--seed=1 --duration=200 --boxHalf=2000 --gwGrid=5 --nlosSource=propagation
        --pcsForm=v2 --mahalUseR=true --arms=ca_tbl_full --trace=true
        --traceDiagnostic=true --anchorSurveyed=false --anchorRangePeriodS=5"

[ -x "$BIN" ] || { printf 'FIXTURE BROKEN: no binary at %s\n' "$BIN"; exit 2; }

printf 'G16 fixture -- two directions\n\n'

# ---- direction 1: a correctly serialised schedule must PASS -------------------
mkdir -p "$TMP/ok"
$BIN $COMMON --anchorPairsPerEpoch=1 --anchorMaxConcurrent=1 --outdir="$TMP/ok" \
    >"$TMP/ok.log" 2>&1
rc=$?
T="$TMP/ok/trace_anchor_ranges_ca_tbl_full_seed1.csv"
if [ "$rc" -ne 0 ] || [ ! -s "$T" ]; then
    printf '  BAD  serialised schedule: run failed (rc=%s) or produced no trace\n' "$rc"
    tail -3 "$TMP/ok.log" | sed 's/^/         /'
    RC=1
elif python3 "$GATE" "$T" 1 >"$TMP/ok.gate" 2>&1; then
    printf '  OK   serialised schedule            expected pass, got pass  (%s exchanges)\n' \
        "$(( $(wc -l < "$T") - 1 ))"
else
    printf '  BAD  serialised schedule            expected pass, got fail\n'
    sed 's/^/         /' "$TMP/ok.gate"; RC=1
fi

# ---- direction 2: two concurrent where the schedule claims one must FAIL ------
# The simulator aborts at the violation, so the check is on the abort and its message.
mkdir -p "$TMP/bad"
$BIN $COMMON --anchorPairsPerEpoch=2 --anchorMaxConcurrent=1 --outdir="$TMP/bad" \
    >"$TMP/bad.log" 2>&1
rc=$?
if [ "$rc" -eq 0 ]; then
    printf '  BAD  deliberate 2-concurrent        expected fail, got pass\n'; RC=1
elif ! grep -q 'concurrency violation' "$TMP/bad.log"; then
    printf '  BAD  deliberate 2-concurrent        failed, but not with the concurrency message\n'
    tail -3 "$TMP/bad.log" | sed 's/^/         /'; RC=1
elif ! grep -qE 'pair=[0-9]+-[0-9]+' "$TMP/bad.log" || ! grep -q 'concurrent=2' "$TMP/bad.log"; then
    printf '  BAD  deliberate 2-concurrent        message does not name the pair AND the count\n'
    grep -m1 'concurrency violation' "$TMP/bad.log" | sed 's/^/         /'; RC=1
else
    printf '  OK   deliberate 2-concurrent        expected fail, got fail, pair and count named\n'
    grep -m1 -o 'pair=[0-9]*-[0-9]* concurrent=[0-9]*' "$TMP/bad.log" | sed 's/^/         /'
fi

# ---- direction 2b: the OFFLINE gate must also catch it, not only the runtime --
# Raise the runtime bound so the run completes, then let the gate judge the trace.
mkdir -p "$TMP/bad2"
$BIN $COMMON --anchorPairsPerEpoch=2 --anchorMaxConcurrent=9 --outdir="$TMP/bad2" \
    >"$TMP/bad2.log" 2>&1
T2="$TMP/bad2/trace_anchor_ranges_ca_tbl_full_seed1.csv"
if [ ! -s "$T2" ]; then
    printf '  BAD  offline gate on a bad trace    no trace produced\n'; RC=1
elif python3 "$GATE" "$T2" 1 >"$TMP/bad2.gate" 2>&1; then
    printf '  BAD  offline gate on a bad trace    expected fail, got pass\n'; RC=1
else
    printf '  OK   offline gate on a bad trace    expected fail, got fail\n'
fi

printf '\n'
[ "$RC" -eq 0 ] && printf 'G16 FIXTURE PASS\n' || printf 'G16 FIXTURE FAIL\n'
exit "$RC"
