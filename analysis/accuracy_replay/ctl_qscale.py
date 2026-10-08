import sys; sys.path.insert(0,'.')
from harness import *
cfg,arm=sys.argv[1],sys.argv[2]
v={}
for q in [1,3,10,30,100]:
    v[f'EKF Qu x{q}']=(lambda q: lambda c,i: run_ekf_imu(c,i,pos0=1.0,qscale=q))(q)
    v[f'EKF Qu x{q} +gates']=(lambda q: lambda c,i: run_ekf_imu(c,i,pos0=1.0,qscale=q,gate=6.63,aux_gate=9.21))(q)
v['aug']=lambda c,i: run_aug(c,i,posStd=1.0)[0]
v['aug+gates']=lambda c,i: run_aug(c,i,posStd=1.0,gate=6.63,aux_gate=9.21)[0]
evaluate(cfg,arm,v)
