#!/usr/bin/env bash
. scripts/diagnostics/lib.sh
OUT="$DIAG_ROOT/D5.md"; RAW="$DIAG_ROOT/raw"; ST=OK
{
echo "# D5 — do the gates actually fire?"; echo
for s in scripts/catbl_gates.sh scripts/catbl_verifier_selftest.sh scripts/publication_ready.sh; do
  echo "## \`$s\`"
  if [ ! -x "$s" ]; then echo '```'; echo "ERROR: absent or not executable"; echo '```'; ST=ERROR; continue; fi
  b=$(basename "$s" .sh)
  timeout 25m "$s" > "$RAW/$b.out" 2>&1; rc=$?
  echo '```'; tail -25 "$RAW/$b.out"; echo "exit=$rc"; echo '```'
  [ $rc -eq 124 ] && { echo "**TIMEOUT**"; ST=FINDING; }
  echo
done
echo "## Negative fixtures — can the checks be observed to FAIL?"
echo
echo "A check never observed to fail is indistinguishable from one that cannot fail."
echo "Fixtures are built in /tmp only; the live tree is untouched."
echo
F=$(mktemp -d /tmp/d5fix.XXXXXX)
# N1: digest verification must catch a corrupted file
mkdir -p "$F/n1"
echo "a,b" > "$F/n1/arm_summary.csv"; echo "1,2" >> "$F/n1/arm_summary.csv"
h=$(sha256sum "$F/n1/arm_summary.csv" | cut -d' ' -f1)
printf 'rid\ninvocation\nsource deadbeef dirty 0 profile optimized\ndigest sha256\n%s  arm_summary.csv\n' "$h" > "$F/n1/run_id.txt"
res_clean=$(python3 - "$F/n1" <<'PY'
import hashlib,os,re,sys
d=sys.argv[1]; t=open(os.path.join(d,"run_id.txt")).read(); bad=0
for l in t.split("digest sha256",1)[1].strip().splitlines():
    m=re.match(r"([0-9a-f]{64})\s+(.+)$",l.strip())
    if m and hashlib.sha256(open(os.path.join(d,m.group(2)),'rb').read()).hexdigest()!=m.group(1): bad+=1
print("MISMATCH" if bad else "CLEAN")
PY
)
echo "3,4" >> "$F/n1/arm_summary.csv"   # corrupt it
res_dirty=$(python3 - "$F/n1" <<'PY'
import hashlib,os,re,sys
d=sys.argv[1]; t=open(os.path.join(d,"run_id.txt")).read(); bad=0
for l in t.split("digest sha256",1)[1].strip().splitlines():
    m=re.match(r"([0-9a-f]{64})\s+(.+)$",l.strip())
    if m and hashlib.sha256(open(os.path.join(d,m.group(2)),'rb').read()).hexdigest()!=m.group(1): bad+=1
print("MISMATCH" if bad else "CLEAN")
PY
)
echo '```'
echo "N1 digest verification"
echo "  untouched fixture : $res_clean   (expect CLEAN)"
echo "  corrupted fixture : $res_dirty   (expect MISMATCH)"
if [ "$res_clean" = "CLEAN" ] && [ "$res_dirty" = "MISMATCH" ]; then
  echo "  VERDICT: the digest check CAN fail -- it is a real check"
else
  echo "  VERDICT: **the digest check did NOT fail on a corrupted file**"
fi
echo '```'
[ "$res_dirty" != "MISMATCH" ] && ST=FINDING
# N2: degeneracy_check must flag a single-tier hybrid arm
mkdir -p "$F/n2"
cat > "$F/n2/arm_summary.csv" <<'CSV'
arm,seed,cycles_868,cycles_2p4,cycles_uwb,escalations,suspects,fixCycles,noFixCycles,cep50M
ca_tbl_full,1,1440,0,0,0,0,1438,2,63.0
CSV
n2=$(python3 scripts/diagnostics/degeneracy_check.py "$F/n2" 2>&1)
echo '```'
echo "N2 degeneracy_check on a synthetic single-tier ca_tbl_full:"
echo "$n2"
echo "$n2" | grep -q DEGENERATE && echo "  VERDICT: flags it -- real check" || echo "  VERDICT: **did not flag a 100%-one-tier hybrid arm**"
echo '```'
echo "$n2" | grep -q DEGENERATE || ST=FINDING
rm -rf "$F"
echo; echo "STATUS: $ST"
} > "$OUT" 2>&1
exit 0
