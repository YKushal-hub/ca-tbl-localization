#!/usr/bin/env bash
# G17 -- IMU error model against its closed forms, both directions.
#
# Direction 1: the nominal model must PASS scripts/imu_closed_form_check.py.
# Direction 2: each fixture breaks ONE term in the model while the validator prints
# nominal parameters -- an implementation silently disagreeing with its own spec, the
# failure class the prior study's Python IMU exhibited three ways. The last two break
# the FILTER instead: no stride-scale state (the per-run error treated as white
# noise), and zero process noise. Both must push the position NEES out of its band.
# Each must FAIL, and must fail on the case that names the broken term. A check that
# fails for the wrong reason is not evidence that it can see the right one, so the case
# is asserted.
#
# Runs in seconds: no network simulation, only the sensor model and the filter.

set -u
set -o pipefail
cd "$(dirname "$0")/.." || exit 2

BIN=./build/scratch/ns3.47-ca_tbl_imu_validator-optimized
CHECK=scripts/imu_closed_form_check.py
TMP=$(mktemp -d "${TMPDIR:-/tmp}/catbl_imu.XXXXXX"); trap 'rm -rf "$TMP"' EXIT
RC=0

[ -x "$BIN" ] || { printf 'FIXTURE BROKEN: no binary at %s -- build first\n' "$BIN"; exit 2; }

printf 'G17 IMU model and filter -- nominal must pass, ten broken variants must fail on their own term\n\n'

# ---- direction 1 --------------------------------------------------------------
if ! "$BIN" --adevOut="$TMP/adev_none.txt" >"$TMP/none.txt" 2>"$TMP/none.err"; then
    printf '  BAD  nominal: validator exited non-zero\n'; sed 's/^/         /' "$TMP/none.err"; exit 1
fi
if python3 "$CHECK" "$TMP/none.txt" "$TMP/adev_none.txt" >"$TMP/none.chk" 2>&1; then
    printf '  OK   nominal model                 expected pass, got pass  (%s statistics)\n' \
        "$(grep -cE '^  OK ' "$TMP/none.chk")"
else
    printf '  BAD  nominal model                 expected pass, got fail\n'
    grep -E 'FAIL' "$TMP/none.chk" | sed 's/^/         /'; RC=1
fi

# ---- direction 2 --------------------------------------------------------------
# fixture : the case whose failure proves the check saw THAT term
for spec in \
    "arw_var_x2:arw/" \
    "bias_drop:bias0/" \
    "bias_x2:bias0/" \
    "gm_white:gm/" \
    "align_drop:align/" \
    "stride_scale_drop:sscale/" \
    "stride_step_x2var:sstep/" \
    "gscale_drop:gscale/" \
    "filter_no_stride_state:filter/" \
    "filter_q_zero:filter/"
do
    fx=${spec%%:*}
    want=${spec#*:}
    if ! "$BIN" --fixture="$fx" --adevOut="$TMP/adev_$fx.txt" >"$TMP/$fx.txt" 2>"$TMP/$fx.err"; then
        printf '  BAD  %-28s validator exited non-zero -- a crash is not a detection\n' "$fx"
        RC=1; continue
    fi
    if ! grep -q "^fixture,$fx\$" "$TMP/$fx.txt"; then
        printf '  BAD  %-28s validator did not report the fixture -- it may not have applied\n' "$fx"
        RC=1; continue
    fi
    if python3 "$CHECK" "$TMP/$fx.txt" "$TMP/adev_$fx.txt" >"$TMP/$fx.chk" 2>&1; then
        printf '  BAD  %-28s expected fail, got pass\n' "$fx"; RC=1
    elif ! grep -q "IMU CHECK FAILED:.* $want" "$TMP/$fx.chk" && \
         ! grep -q "IMU CHECK FAILED: $want" "$TMP/$fx.chk"; then
        printf '  BAD  %-28s failed, but not on %s\n' "$fx" "$want"
        grep 'IMU CHECK FAILED' "$TMP/$fx.chk" | sed 's/^/         /'; RC=1
    else
        printf '  OK   %-28s expected fail, got fail on %s\n' "$fx" \
            "$(grep 'IMU CHECK FAILED' "$TMP/$fx.chk" | sed 's/IMU CHECK FAILED: //')"
    fi
done

if [ "$RC" -eq 0 ]; then
    printf '\nG17 fixture: all directions behaved\n'
else
    printf '\nG17 fixture: FAILED\n'
fi
exit "$RC"
