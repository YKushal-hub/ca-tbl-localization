import sys; sys.path.insert(0,'.')
from score import *
rng=np.random.default_rng(3)
def pb(d):
    d=np.asarray(d); m=d[rng.integers(0,len(d),(4000,len(d)))].mean(1); return d.mean(),np.quantile(m,.025),np.quantile(m,.975)
def get(cell,arm):
    r,_=collect(cell,arm); return r.set_index('seed')
def verdict(m,lo,hi,better_low):
    if lo>0 or hi<0:
        good=(m<0) if better_low else (m>0)
        return 'WIN' if good else 'LOSS'
    return 'tie'
for cfg in ['A','B']:
    ca=get('Q10'+cfg,'ca_tbl_full')
    comps={'fixed_threshold':get('Q10'+cfg,'fixed_threshold'),'static_2p4':get('Q10'+cfg,'static_2p4'),'poucet_fusion':get('Q10C'+cfg,'poucet_fusion'),'lora_fusion':get('Q10C'+cfg,'lora_fusion')}
    print('=====',cfg,' ca_tbl_full(Q10) minus comparator(Q10), paired over seeds; 95% bootstrap')
    for nm,c in comps.items():
        idx=ca.index.intersection(c.index)
        out=[]
        for col,lab,low in [('fix','fixAvail',False),('w25','within25(5 nodes)',False),('t0w25','tag0 within25',False),('c50','CEP50 m',True),('c95','CEP95 m',True),('J','J/node',True)]:
            d=ca.loc[idx,col]-c.loc[idx,col]; m,lo,hi=pb(d)
            out.append(f'{lab}: {m:+.3f} [{lo:+.3f},{hi:+.3f}] {verdict(m,lo,hi,low)}')
        print(f'  vs {nm:16s}',' | '.join(out))
