#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
OUT="$DIAG_ROOT/D8.md"; ST=OK
{
echo "# D8 — build hygiene"; echo
echo '## ignored-but-tracked (must be empty)'
echo '```'
ig=$(git ls-files -z | xargs -0 -r git check-ignore 2>/dev/null)
if [ -z "$ig" ]; then echo "none"; else echo "$ig"; ST=FINDING; fi
echo '```'
echo; echo '## ignored source-extension files not in EXCLUDED.md'
echo '```'
excl=$(awk '/^```reviewed-exclusions$/{f=1;next} /^```$/{f=0} f' EXCLUDED.md 2>/dev/null)
pat=$(printf '%s' "$excl" | sed 's/[].[^$\\*]/\\&/g' | paste -sd'|' -)
hidden=$(git ls-files --others --ignored --exclude-standard 2>/dev/null \
   | grep -E '\.(yml|yaml|cc|cpp|h|hpp|py|sh|cmake|rst|md|json|toml)$' \
   | { [ -n "$pat" ] && grep -vE "^(${pat})" || cat; } )
if [ -z "$hidden" ]; then echo "none"; else echo "$hidden" | head -20; ST=FINDING; fi
echo '```'
echo; echo '## vendored lorawan'
echo '```'
n=$(git ls-files src/lorawan | wc -l)
echo "tracked files under src/lorawan: $n  (expected 140)"
m=$(git ls-files -s src/lorawan | awk '{print $1}' | sort -u | tr '\n' ' ')
echo "modes present: $m  (expected 100644 only; 160000 would be a gitlink)"
[ "$n" -ne 140 ] && ST=FINDING
echo "$m" | grep -q 160000 && ST=FINDING
echo '```'
echo; echo '## oversize tracked files'
echo '```'
big=$(git ls-files -z | xargs -0 -r stat -c '%s %n' 2>/dev/null | awk '$1>45000000 {print $2}')
if [ -z "$big" ]; then echo "none over 45 MB"; else echo "$big"; ST=FINDING; fi
echo '```'
echo; echo "NOTE: a fresh clone + build was NOT performed — it would take ~40 min of the"
echo "budget and D5 already exercises the built binary. Recorded as not done."
echo; echo "STATUS: $ST"
} > "$OUT" 2>&1
exit 0
