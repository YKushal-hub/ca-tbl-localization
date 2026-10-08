import sys; sys.path.insert(0,'.')
from harness import *
cfg,arm=sys.argv[1],sys.argv[2]
v={'replay EKF P0=100 + aux gate (≈ ALL)':lambda c,i: run_ekf_imu(c,i,aux_gate=9.21),
   'EKF P0=1 + aux gate':lambda c,i: run_ekf_imu(c,i,pos0=1.0,aux_gate=9.21),
   'EKF P0=1 Qu x10 + aux gate':lambda c,i: run_ekf_imu(c,i,pos0=1.0,qscale=10,aux_gate=9.21),
   'EKF P0=1 Qu x10 + gates':lambda c,i: run_ekf_imu(c,i,pos0=1.0,qscale=10,gate=6.63,aux_gate=9.21),
   'aug + gates':lambda c,i: run_aug(c,i,posStd=1.0,gate=6.63,aux_gate=9.21)[0]}
evaluate(cfg,arm,v)
