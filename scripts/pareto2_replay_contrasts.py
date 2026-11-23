import sys,csv,glob;sys.path.insert(0,'/home/nethi/catbl_multi/scripts')
import pareto2_analyse as p,camp_spec_analyse as cs,inspect
P='/home/nethi/catbl_work/campaigns/replay41/pareto2_dd41'
D={c:[p.row(f'{P}/A_{c}/s{s}','fixed_threshold') for s in range(421,441)] for c in('BEN1000','P1BEN1000','BASE')}
import numpy as np
for a,b in(('BEN1000','P1BEN1000'),('BEN1000','BASE'),('P1BEN1000','BASE')):
  for k,n in((0,'lost A25'),(1,'five A25'),(2,'CEP95'),(3,'J')):
    d=np.array([x[k]-y[k] for x,y in zip(D[a],D[b])]);print(a,b,n,round(d.mean(),4),cs.ci(d))
  i=1;d=sorted((x[i]-y[i],s) for x,y,s in zip(D[a],D[b],range(421,441)));print(d[:3],d[-2:])
