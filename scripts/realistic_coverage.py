# crew-pooled (5 subjects) 95% region coverage, errM <= 1.73*sqrt(traceSigma), and tier share
import csv,glob,os,sys
from collections import Counter
for camp in sys.argv[1:]:
    print('##',camp)
    for cell in sorted(os.listdir(camp)):
        if not cell.endswith('_ARMS'): continue
        n=k=0;T=Counter()
        for f in glob.glob(f'{camp}/{cell}/s*/trace_ca_tbl_full_seed*.csv')+glob.glob(f'{camp}/{cell}/s*/subjtrace_ca_tbl_full_seed*.csv'):
            for r in csv.DictReader(open(f)):
                n+=1;T[r['activeTier']]+=1
                k+=float(r['errM'])<=1.73*max(float(r['traceSigma']),1e-9)**.5
        print(cell,n,'cover',round(k/n,3),{a:round(b/n,3) for a,b in T.items()},flush=True)
