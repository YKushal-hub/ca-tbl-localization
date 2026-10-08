import sys, glob, os, time
sys.path.insert(0,'.')
import numpy as np, pandas as pd
from replay import *

def cells(cfg):
    return sorted([d for d in glob.glob(f'runs/{cfg}/s*') if os.path.exists(d+'/rc')], key=lambda p:int(p.rsplit('/s',1)[1]))

def evaluate(cfg, arm, variants, maxseeds=99):
    rows={k:[] for k in ['sim']+list(variants)}
    per={k:[] for k in rows}
    for d in cells(cfg)[:maxseeds]:
        s=int(d.rsplit('/s',1)[1])
        try:
            cyc=parse(f'{d}/ranges_{arm}_seed{s}.csv')
            imu=pd.read_csv(f'{d}/imu_{arm}_seed{s}.csv')
            tr=pd.read_csv(f'{d}/trace_{arm}_seed{s}.csv')
        except Exception as ex:
            print('skip',d,ex); continue
        rows['sim'].append(tr.errM.values); per['sim'].append(np.median(tr.errM.values))
        for k,fn in variants.items():
            est=fn(cyc,imu); e=err(cyc,est)
            rows[k].append(e); per[k].append(np.median(e))
    out=[]
    for k,v in rows.items():
        if not v: continue
        a=np.concatenate(v)
        out.append((k,len(v),np.median(a),np.quantile(a,.95),np.mean(a),np.mean(per[k])))
    print(f'--- {cfg} {arm}')
    for k,n,m,p,mu,pm in out:
        print(f'{k:22s} seeds={n:2d} pooled CEP50={m:8.1f} CEP95={p:9.1f} mean={mu:9.1f}')

if __name__=='__main__':
    cfg,arm=sys.argv[1],sys.argv[2]
    variants={
      'replay EKF+IMU': lambda c,i: run_ekf_imu(c,i),
      'DR only (from truth)': lambda c,i: run_ekf_imu(c,i,dr_only=True),
      'EKF+IMU, ranges x3 R': lambda c,i: run_ekf_imu(c,i,infl=3.0*3),
    }
    evaluate(cfg,arm,variants)
