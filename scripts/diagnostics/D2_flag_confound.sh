#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
python3 scripts/diagnostics/d2_flag_confound.py > "$DIAG_ROOT/D2.md" 2>&1
exit 0
