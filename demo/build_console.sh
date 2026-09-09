#!/bin/bash
#
# Turn a run directory into the single-file console. Run from the repository root.
#
#   ./demo/build_console.sh [run-dir] [out.html]
#
# Two steps, and they are separate on purpose. pack_demo.py reduces a run (which is tens of
# megabytes of trace and event CSV) to one compact JSON; the inline step drops that JSON
# into the template's placeholder. Keeping them apart means a page can be rebuilt from
# demo/run.json without re-running the simulator, which takes minutes.
set -eu

RUN="${1:-demo/out}"
OUT="${2:-demo/console.html}"
ARM=ca_tbl_full
SEED=7
JSON="${RUN%/}/packed.json"

python3 scripts/pack_demo.py "$RUN" "$ARM" "$SEED" "$JSON"

python3 - "$JSON" "$OUT" <<'PY'
import sys
j_path, out = sys.argv[1], sys.argv[2]
h = open("viz/console_template.html", encoding="utf-8").read()
j = open(j_path, encoding="utf-8").read().strip()

# FAIL LOUDLY RATHER THAN SHIP A PAGE WHOSE DATA IS STILL null. That is exactly how the
# previous console died silently: it rendered, and nothing worked, and nothing said so.
MARK = "/*__DATA__*/ null"
if h.count(MARK) != 1:
    sys.exit("placeholder matched %d times, expected 1" % h.count(MARK))
h = h.replace(MARK, j)
if "/*__DATA__*/" in h:
    sys.exit("DATA still unpopulated after substitution")

open(out, "w", encoding="utf-8").write(h)
print("wrote %s  %.2f MB" % (out, len(h.encode("utf-8")) / 1e6))
PY

echo
echo "Preview it BEFORE publishing -- the in-app browser cannot drive file:// pages,"
echo "and a previous console shipped visibly broken because nobody loaded it first:"
echo "    python3 -m http.server 8899 --directory $(dirname "$OUT")"
echo "    open http://localhost:8899/$(basename "$OUT")"
