import sys, glob, os; sys.path.insert(0,'.')
import numpy as np, pandas as pd
from replay import *
from sklearn.ensemble import HistGradientBoostingClassifier
from sklearn.model_selection import GroupKFold
from sklearn.metrics import roc_auc_score
def nis_stream(cyc, imu, pos0=100.0):
    e=EkfImu(cyc[0]['tx'],cyc[0]['ty'],posStd=pos0); out={}
    for k,c in enumerate(cyc):
        r=imu.iloc[k]; e.predict_ctrl(c['dt'],r.ux,r.uy,r.quxx,r.quxy,r.quyy)
        if len(c['R'])>=3:
            e.update_ranges(c['R'],3.0)
        if c['aux'] is not None and len(c['R'])>=3:
            a=c['aux']; S=np.array([[e.P[0,0]+a[2],e.P[0,1]+a[3]],[e.P[1,0]+a[3],e.P[1,1]+a[4]]]); v=np.array([a[0],a[1]])-e.x[:2]
            try: out[round(c['t'],2)]=float(v@np.linalg.inv(S)@v)
            except Exception: out[round(c['t'],2)]=np.nan
            e.fuse_pos((a[0],a[1]),a[2],a[3],a[4])
    return out
rows=[]
for cfg in ['A','B']:
    for d in sorted(glob.glob(f'runs/{cfg}/s*')):
        s=int(d.rsplit('/s',1)[1])
        for arm in ['ca_tbl_full','static_868','fixed_threshold']:
            cyc=parse(f'{d}/ranges_{arm}_seed{s}.csv'); imu=pd.read_csv(f'{d}/imu_{arm}_seed{s}.csv')
            ns=nis_stream(cyc,imu)
            t=pd.read_csv(f'{d}/tdoa_{arm}_seed{s}.csv'); t=t[t.verdict=='OK'].copy()
            t['nisPrior']=[ns.get(round(x,2),np.nan) for x in t.t]
            t['cfg']=cfg; t['seed']=s; t['arm']=arm; rows.append(t)
T=pd.concat(rows,ignore_index=True)
T['sig']=np.sqrt((T.rxx+T.ryy)/2); T['z']=T.tdoaErrM/T.sig
T['trace']=T.rxx+T.ryy
T['tieGap']=(T.tieOther-T.tieChosen).where(T.tieChosen>=0,-1)
T['rootsGt1']=(T.nRoots>1).astype(int)
feats=['nStamps','nRoots','tieChosen','tieOther','tieGap','iterations','rxx','rxy','ryy','trace','chi2','refAnchor','nisPrior','cfgB']
T['cfgB']=(T.cfg=='B').astype(int)
T['log_nis']=np.log1p(T.nisPrior)
for cfg in ['A','B']:
    S=T[T.cfg==cfg].dropna(subset=['nisPrior'])
    for name,y in [('err>100 m',(S.tdoaErrM>100).values),('err>3 sigma',(S.z>3).values),('err>5x claimed trace^.5 or >300m',((S.tdoaErrM>300)).values)]:
        if y.sum()<10: print(cfg,name,'too few positives',y.sum()); continue
        X=S[[f for f in feats if f!='cfgB']].fillna(-1).values; g=S.seed.values
        pred=np.zeros(len(S))
        for tr,te in GroupKFold(5).split(X,y,g):
            m=HistGradientBoostingClassifier(max_iter=150,learning_rate=0.05,max_depth=3,random_state=0).fit(X[tr],y[tr]); pred[te]=m.predict_proba(X[te])[:,1]
        print(f'{cfg} n={len(S)} pos={int(y.sum())} target={name}: AUC GBM {roc_auc_score(y,pred):.3f} | NIS-vs-prior {roc_auc_score(y,S.nisPrior.values):.3f} | claimed sigma {roc_auc_score(y,S.sig.values):.3f} | tieGap {roc_auc_score(y,-S.tieGap.values):.3f}')
T.to_pickle('tdoa_all.pkl')
