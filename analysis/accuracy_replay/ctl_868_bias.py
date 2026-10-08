import sys, copy; sys.path.insert(0,'.')
from harness import *
def is868(q):
    ax,ay,r,s,nl,src,fl=q
    return (s>=0.15*r and r>40) or fl==1
def tx(cyc,mode):
    out=[]
    for c in cyc:
        c2=dict(c); R=[]
        for q in c['R']:
            ax,ay,r,s,nl,src,fl=q
            if is868(q):
                if mode=='drop': continue
                if mode=='infl': s=np.sqrt(s*s+(0.15*r)**2)
                if mode=='debias' and r>1000: r=r/0.88
                if mode=='drop_if_precise':
                    pass
            R.append((ax,ay,r,s,nl,src,fl))
        if mode=='drop_if_precise':
            prec=[q for q in c['R'] if not is868(q)]
            R=[q for q in c['R'] if (not is868(q))] if len(prec)>=3 else list(c['R'])
        c2['R']=R; out.append(c2)
    return out
def mk(mode,P0=100.0):
    return lambda c,i: run_aug(tx(c,mode),i,posStd=P0,gate=6.63,aux_gate=9.21)[0]
v={'aug+gates (as is)':mk('none'),'drop 868 ranges':mk('drop'),'inflate 868 sigma (15% of range)':mk('infl'),'debias 868 long ranges /0.88':mk('debias'),'drop 868 if >=3 precise':mk('drop_if_precise')}
for cfg in ['ALLA','ALLB']: evaluate(cfg,'ca_tbl_full',v)
