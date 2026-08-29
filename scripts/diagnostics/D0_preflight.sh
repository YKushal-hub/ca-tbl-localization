#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
OUT="$DIAG_ROOT/D0.md"; ST=OK
{
echo "# D0 — preflight"
echo
echo '## Tree state as found'
echo '```'
echo "HEAD            $(git rev-parse HEAD)"
echo "log             $(git log --oneline -1)"
n=$(git status --porcelain --untracked-files=no | wc -l)
echo "tracked-modified $n"
git status --porcelain --untracked-files=no
echo "dirty scope (src scratch) $(git status --porcelain --untracked-files=no -- src scratch | wc -l)"
echo '```'
[ "$n" -ne 0 ] && { echo; echo '**FINDING**: tree is not clean at start.'; ST=FINDING; }
echo
echo '## Concurrency'
r=$(pgrep -c 'ns3.47-ca_tbl_c' 2>/dev/null); r=${r:-0}
echo '```'; echo "sims running by comm: $r"; echo '```'
[ "$r" -ne 0 ] && { echo; echo '**FINDING**: a simulation is already running.'; ST=FINDING; }
echo
echo '## Disk'
echo '```'; df -h . | tail -1; echo '```'
echo
echo '## Binary vs source'
echo '```'
if [ -f "$BIN" ]; then
  echo "binary   $(stat -c '%y' "$BIN")"
  echo "sha256   $(sha256sum "$BIN" | cut -c1-32)"
  newest=$(find scratch/ca_tbl_causal_sim.cc src/lora-physics-engine -name '*.cc' -o -name '*.h' 2>/dev/null | xargs -r stat -c '%Y %n' | sort -rn | head -1)
  echo "newest source  $newest"
  bt=$(stat -c '%Y' "$BIN"); st=$(echo "$newest" | cut -d' ' -f1)
  if [ "${st:-0}" -gt "$bt" ]; then echo "VERDICT: binary PREDATES source"; else echo "VERDICT: binary is newer than all source"; fi
else
  echo "binary ABSENT"
fi
echo '```'
echo; echo "STATUS: $ST"
} > "$OUT" 2>&1
grep -q 'binary PREDATES' "$OUT" && sed -i 's/^STATUS: OK$/STATUS: FINDING/' "$OUT"
exit 0
