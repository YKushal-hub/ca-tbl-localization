# makes a memory-budgeted copy of a frozen runner: same cells, flags, seeds, order; only the start condition differs
import sys
n=sys.argv[1]; s=open(f'/home/nethi/catbl_multi/scripts/realistic{n}_run.sh').read()
def rep(a,b):
    global s
    assert s.count(a)==1,(a,s.count(a)); s=s.replace(a,b)
rep('cd "$(dirname "$0")/.."','cd /home/nethi/catbl_multi')
rep('    while [ "$(jobs -rp | wc -l)" -ge "$J" ]; do sleep 1; done\n','    pgrep -f -- "--outdir=$d\\$" >/dev/null && return  # already running (survived a restart)\n    while ! /home/nethi/catbl_work/campaigns/admit.sh "$d"; do sleep 2; done\n')
if n=='1':
    i=s.index('for s in $(seq 501 520); do'); j=s.index('\nwait\n',i); body=s[i:j]
    assert body.count('for site in ROAD1 ROAD3 CLUS3 RING12; do')==1
    a=body.replace('for site in ROAD1 ROAD3 CLUS3 RING12; do','for site in ROAD1 ROAD3 CLUS3; do')
    k=body.index('    # The field-measured'); b=body[:k].replace('for site in ROAD1 ROAD3 CLUS3 RING12; do','for site in RING12; do')+'done\n'
    s=s[:i]+a+'\n'+b.rstrip('\n')+s[j:]   # RING12 (reference only) last
open(f'/home/nethi/catbl_work/campaigns/realistic{n}_budget.sh','w').write(s)
