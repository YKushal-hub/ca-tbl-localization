#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
python3 - <<'PY' > "$DIAG_ROOT/D3.md" 2>&1
import glob, os, sys
sys.path.insert(0, "scripts/diagnostics")
from degeneracy_check import check
dirs = sorted(set(os.path.dirname(p) for p in glob.glob("results/**/arm_summary.csv", recursive=True)))
print("# D3 — degeneracy pre-screen over every existing result directory\n")
print("Tool: `scripts/diagnostics/degeneracy_check.py`. Rules are D1 §3.2.\n")
tot = deg = 0
bykind = {}
for d in dirs:
    for arm, v in check(d):
        tot += 1
        if v.startswith("DEGENERATE"):
            deg += 1
            k = v.split(":", 1)[1].strip()
            bykind.setdefault(k, []).append((d, arm))
print("- arm-cells examined: **%d** across %d directories" % (tot, len(dirs)))
print("- degenerate: **%d** (%.1f%%)\n" % (deg, 100.0 * deg / tot if tot else 0))
print("## By reason\n")
print("| reason | count | example |")
print("|---|---|---|")
for k in sorted(bykind, key=lambda x: -len(bykind[x])):
    print("| %s | %d | `%s` (`%s`) |" % (k, len(bykind[k]), bykind[k][0][0], bykind[k][0][1]))
print("\nSTATUS: %s" % ("FINDING" if deg else "OK"))
PY
exit 0
