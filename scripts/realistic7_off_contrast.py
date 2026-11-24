import sys,math,numpy as np;sys.path.insert(0,'/home/nethi/catbl_calib/scripts')
import realistic_analyse as ra,camp_spec_analyse as cs
R='/home/nethi/catbl_work/campaigns/realistic7/'
c=lambda z:4200 if math.isnan(z) else z
for st in('ROAD1','ROAD3','CLUS3','RAND3','ROAD3F4'):
  X={}
  for p in('OFF','E'):
    for a in('ca_tbl_full','static_2p4','fixed_threshold'):
      X[p,a]=[(lambda e:(c(e['treach_s']),c(e['t25_s'])))(ra.emergency(f'{R}{p}_{st}_ARMS/s{s}',a)) for s in range(621,641)]
  f=lambda A,B,i:cs.fmt(*cs.ci([x[i]-y[i] for x,y in zip(A,B)]),p=0)
  print(st,'| ours-2.4 reach OFF',f(X['OFF','ca_tbl_full'],X['OFF','static_2p4'],0),'ON',f(X['E','ca_tbl_full'],X['E','static_2p4'],0),'| t25 OFF',f(X['OFF','ca_tbl_full'],X['OFF','static_2p4'],1),'ON',f(X['E','ca_tbl_full'],X['E','static_2p4'],1),'\n    reach means off/on ours %d/%d 2.4 %d/%d'%tuple(np.mean([x[0] for x in X[k]]) for k in(('OFF','ca_tbl_full'),('E','ca_tbl_full'),('OFF','static_2p4'),('E','static_2p4'))),'| 2.4 on-off reach',f(X['E','static_2p4'],X['OFF','static_2p4'],0),'| ours on-off',f(X['E','ca_tbl_full'],X['OFF','ca_tbl_full'],0),flush=True)
