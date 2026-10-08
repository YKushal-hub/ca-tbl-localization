import glob, os, sys, numpy as np, pandas as pd
rng=np.random.default_rng(7)
def cellseeds(cell): return sorted([d for d in glob.glob(f'runs/{cell}/s*') if os.path.exists(d+'/rc')], key=lambda p:int(p.rsplit('/s',1)[1]))
def collect(cell, arm):
    rows=[]; E=[]
    for d in cellseeds(cell):
        s=int(d.rsplit('/s',1)[1])
        try:
            ss=pd.read_csv(f'{d}/subject_summary.csv'); ss=ss[ss.arm==arm]
            t0=pd.read_csv(f'{d}/trace_{arm}_seed{s}.csv')
        except Exception: continue
        if len(ss)==0: continue
        fix=(ss.fixCycles/ss.steps).mean(); J=ss.totalJ.mean()
        w=[(t0.errM<=25).mean()]
        for k in range(2,6):
            f=f'{d}/subjtrace_{arm}_seed{s}_subj{k}.csv'
            if os.path.exists(f): w.append((pd.read_csv(f).errM<=25).mean())
        rows.append(dict(seed=s,fix=fix,J=J,w25=np.mean(w),t0w25=w[0],t0fix=(t0.hadFix==1).mean(),c50=np.median(t0.errM),c95=np.quantile(t0.errM,.95)))
        E.append(t0.errM.values)
    return pd.DataFrame(rows), (np.concatenate(E) if E else np.array([]))
def line(name,cell,arm):
    r,E=collect(cell,arm)
    if len(r)==0: return None
    return dict(name=name,n=len(r),fix=r.fix.mean(),w25=r.w25.mean(),t0w25=r.t0w25.mean(),c50=np.median(E),c95=np.quantile(E,.95),J=r.J.mean()), r
if __name__=='__main__':
    for cfg,cmp in [('A','CMPA'),('B','CMPB')]:
        print('=====',cfg)
        items=[('ca_tbl_full (preset)',cfg,'ca_tbl_full'),('ca_tbl_full + ALL (N26)','ALL'+cfg,'ca_tbl_full'),('fixed_threshold',cfg,'fixed_threshold'),
               ('static_868',cfg,'static_868'),('static_2p4',cfg,'static_2p4'),('poucet_fusion',cmp,'poucet_fusion'),('lora_fusion',cmp,'lora_fusion')]
        print(f'{"arm":26s} n  fixAvail  within25(5 nodes)  tag0 within25  tag0 CEP50  CEP95   J/node')
        for nm,c,a in items:
            x=line(nm,c,a)
            if x is None: print(nm,'missing'); continue
            d=x[0]; print(f'{d["name"]:26s}{d["n"]:3d}  {d["fix"]:.3f}     {d["w25"]:.3f}            {d["t0w25"]:.3f}         {d["c50"]:7.1f} {d["c95"]:8.1f} {d["J"]:8.2f}')
