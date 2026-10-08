import sys; sys.path.insert(0,'.')
from harness import *
cfg,arm=sys.argv[1],sys.argv[2]
v={
 'replay EKF+IMU (P0=100m)': lambda c,i: run_ekf_imu(c,i),
 'replay EKF+IMU (P0=1m)': lambda c,i: run_ekf_imu(c,i,pos0=1.0),
 'EKF P0=1m + gates': lambda c,i: run_ekf_imu(c,i,pos0=1.0,gate=6.63,aux_gate=9.21),
 'aug P0=1m': lambda c,i: run_aug(c,i,posStd=1.0)[0],
 'aug P0=100m': lambda c,i: run_aug(c,i,posStd=100.0)[0],
 'aug P0=1m + gates': lambda c,i: run_aug(c,i,posStd=1.0,gate=6.63,aux_gate=9.21)[0],
 'aug P0=100m + gates': lambda c,i: run_aug(c,i,posStd=100.0,gate=6.63,aux_gate=9.21)[0],
}
evaluate(cfg,arm,v)
