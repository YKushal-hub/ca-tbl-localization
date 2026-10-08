import glob, os, numpy as np, pandas as pd
from sklearn.ensemble import HistGradientBoostingRegressor
from sklearn.model_selection import GroupKFold
from scipy.stats import spearmanr
feats=['traceSigma','gdop','nAnchors','nNlos','mahalanobis','meanSigmaM','meanSigmaRawM','sigmaGeoM','nbr868','nbr2p4','nbrUwb','nRangesEkf','floorRanged','pcs','usedFloor','tierNum','age','prevTrace']
def load(cfg,arm='ca_tbl_full'):
    out=[]
    for d in sorted(glob.glob(f'runs/{cfg}/s*')):
        s=int(d.rsplit('/s',1)[1])
        t=pd.read_csv(f'{d}/trace_{arm}_seed{s}.csv'); t['seed']=s
        t['tierNum']=t.fixTier.map({'868MHz':0,'2.4GHz':1,'UWB':2}).fillna(-1)
        age=[];a=99
        for h in t.hadFix.values:
            a=0 if h==1 else a+1; age.append(a)
        t['age']=age
        t['prevTrace']=t.traceSigma.shift(1).fillna(t.traceSigma)
        out.append(t)
    return pd.concat(out,ignore_index=True)
for cfg in ['A','B']:
    t=load(cfg)
    f=t[(t.hadFix==1)].copy()
    y=np.log1p(f.errM.values)
    X=f[feats].values
    g=f.seed.values
    pred=np.zeros(len(f))
    for tr,te in GroupKFold(5).split(X,y,g):
        m=HistGradientBoostingRegressor(max_iter=200,learning_rate=0.05,max_depth=4,random_state=0).fit(X[tr],y[tr])
        pred[te]=m.predict(X[te])
    r=lambda a: spearmanr(a,f.errM.values)[0]
    print(f'== {cfg} ca_tbl_full fix cycles n={len(f)} seeds={f.seed.nunique()}')
    print('  rho(pred_GBM, err)   = %.3f'%r(pred))
    print('  rho(traceSigma, err) = %.3f'%r(f.traceSigma.values))
    print('  rho(-pcs, err)       = %.3f'%r(-f.pcs.values))
    print('  rho(meanSigmaM, err) = %.3f'%r(f.meanSigmaM.values))
    print('  rho(mahalanobis, err)= %.3f'%r(f.mahalanobis.values))
    # per-seed paired: rho_gbm - rho_trace
    d=[]
    for s in np.unique(g):
        m=g==s
        if m.sum()>30: d.append(spearmanr(pred[m],f.errM.values[m])[0]-spearmanr(f.traceSigma.values[m],f.errM.values[m])[0])
    d=np.array(d); rng=np.random.default_rng(1); bs=d[rng.integers(0,len(d),(4000,len(d)))].mean(1)
    print('  per-seed rho(GBM)-rho(trace): mean %.3f [%.3f, %.3f], GBM better on %d/%d seeds'%(d.mean(),np.quantile(bs,.025),np.quantile(bs,.975),(d>0).sum(),len(d)))
    # top-decile error detection
    thr=np.quantile(f.errM,.9); from sklearn.metrics import roc_auc_score
    print('  AUC for err>p90: GBM %.3f  trace %.3f  mahal %.3f'%(roc_auc_score(f.errM>thr,pred),roc_auc_score(f.errM>thr,f.traceSigma),roc_auc_score(f.errM>thr,f.mahalanobis)))
