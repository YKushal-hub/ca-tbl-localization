import sys, glob, os, json
sys.path.insert(0,'.')
import numpy as np, pandas as pd
from replay import *
rng=np.random.default_rng(12345)
def boot(d, n=4000):
    d=np.asarray(d); m=d[rng.integers(0,len(d),(n,len(d)))].mean(1)
    return d.mean(), np.quantile(m,.025), np.quantile(m,.975), int((d<0).sum()), int((d>0).sum())
def cellsof(cfg): return sorted([d for d in glob.glob(f'runs/{cfg}/s*') if os.path.exists(d+'/rc')], key=lambda p:int(p.rsplit('/s',1)[1]))
V={
 'base': lambda c,i: run_ekf_imu(c,i),
 'base_P1': lambda c,i: run_ekf_imu(c,i,pos0=1.0),
 'auxgate': lambda c,i: run_ekf_imu(c,i,aux_gate=9.21),
 'gates_P1': lambda c,i: run_ekf_imu(c,i,pos0=1.0,gate=6.63,aux_gate=9.21),
 'aug_P1': lambda c,i: run_aug(c,i,posStd=1.0)[0],
 'aug_gates_P1': lambda c,i: run_aug(c,i,posStd=1.0,gate=6.63,aux_gate=9.21)[0],
 'dr': lambda c,i: run_ekf_imu(c,i,dr_only=True),
}
out={}
for cfg in ['A','B']:
    for arm in ['ca_tbl_full','static_868','static_2p4']:
        per={k:[] for k in V}; sim=[]; nees={k:[] for k in ['base','aug_P1']}
        for d in cellsof(cfg):
            s=int(d.rsplit('/s',1)[1])
            cyc=parse(f'{d}/ranges_{arm}_seed{s}.csv'); imu=pd.read_csv(f'{d}/imu_{arm}_seed{s}.csv'); tr=pd.read_csv(f'{d}/trace_{arm}_seed{s}.csv')
            sim.append((np.median(tr.errM),np.quantile(tr.errM,.95)))
            for k,fn in V.items():
                est=fn(cyc,imu); e=err(cyc,est)
                per[k].append((np.median(e),np.quantile(e,.95),e.mean()))
                if k in nees:
                    r=e**2/np.maximum(est[:,2],1e-9)
                    nees[k].append((np.median(r), np.mean(e**2<=5.991*est[:,2]/2)))
        out[(cfg,arm)]=dict(per={k:np.array(v) for k,v in per.items()},sim=np.array(sim),nees={k:np.array(v) for k,v in nees.items()})
import pickle; pickle.dump(out,open('paired.pkl','wb'))
def row(cfg,arm,a,b,col,lab):
    P=out[(cfg,arm)]['per']; d=P[b][:,col]-P[a][:,col]
    m,lo,hi,neg,pos=boot(d)
    print(f'{cfg} {arm:12s} {lab:34s} mean per-seed diff {m:8.2f} [{lo:8.2f},{hi:8.2f}]  better/worse seeds {neg}/{pos}')
for cfg in ['A','B']:
    for arm in ['ca_tbl_full','static_868','static_2p4']:
        print('---',cfg,arm)
        for a,b,lab in [('base','auxgate','aux gate vs base'),('base','base_P1','P0=1m vs base'),('base','aug_gates_P1','aug+gates(P0=1) vs base'),('base_P1','aug_gates_P1','aug+gates vs EKF (both P0=1)'),('base_P1','gates_P1','gates vs EKF (both P0=1)'),('dr','aug_gates_P1','aug+gates vs DR-only')]:
            for col,cn in [(0,'CEP50'),(1,'CEP95')]:
                row(cfg,arm,a,b,col,lab+' '+cn)
        n=out[(cfg,arm)]['nees']
        print('  NEES median err^2/trace (calibrated=0.69) & 95%-coverage: base',n['base'].mean(0).round(2),' aug',n['aug_P1'].mean(0).round(2))
