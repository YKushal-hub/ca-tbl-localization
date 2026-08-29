#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
python3 scripts/diagnostics/d6_docs.py > "$DIAG_ROOT/D6.md" 2>&1
exit 0
