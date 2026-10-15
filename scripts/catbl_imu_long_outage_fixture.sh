#!/usr/bin/env bash
# G20 fixture: moment-matched coasting must pass the long-outage check, and the plain EKF --
# the configuration whose defect G20 exists to catch -- must fail it, on the outage line.
set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2
BIN=./build/scratch/ns3.47-ca_tbl_imu_validator-optimized
CHECK=scripts/imu_long_outage_check.py
TMP=$(mktemp -d "${TMPDIR:-/tmp}/catbl_g20.XXXXXX"); trap 'rm -rf "$TMP"' EXIT
RC=0
[ -x "$BIN" ] || { echo "FIXTURE BROKEN: no binary at $BIN"; exit 2; }
COMMON="--dwellDiag=400 --dwellTransit=400 --mc=10 --filterMc=0"
printf 'G20 fixture -- moment-matched must pass, the plain EKF must fail\n\n'
"$BIN" $COMMON --dwellMM=true --dwellMMNoShift=true > "$TMP/mm.txt" 2>/dev/null
if python3 "$CHECK" "$TMP/mm.txt" > "$TMP/mm.chk"; then
    printf '  OK   moment-matched coasting      expected pass, got pass\n'
else
    printf '  BAD  moment-matched coasting      expected pass, got fail\n'; sed 's/^/       /' "$TMP/mm.chk"; RC=1
fi
"$BIN" $COMMON > "$TMP/ekf.txt" 2>/dev/null
if python3 "$CHECK" "$TMP/ekf.txt" > "$TMP/ekf.chk"; then
    printf '  BAD  plain EKF                    expected fail, got pass\n'; RC=1
elif ! grep -q 'G20 FAILED: outage' "$TMP/ekf.chk"; then
    printf '  BAD  plain EKF                    failed, but not on the outage line\n'; RC=1
else
    printf '  OK   plain EKF                    expected fail, got fail (%s)\n' \
        "$(grep 'end-of-outage' "$TMP/ekf.chk" | sed 's/^ *FAIL *//')"
fi
[ "$RC" -eq 0 ] && printf '\nG20 fixture: all directions behaved\n' || printf '\nG20 fixture: FAILED\n'
exit "$RC"
