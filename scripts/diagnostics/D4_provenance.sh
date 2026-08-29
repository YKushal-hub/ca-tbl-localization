#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
python3 scripts/diagnostics/d4_provenance.py > "$DIAG_ROOT/D4.md" 2>&1
exit 0
