import sys; sys.path.insert(0,'.')
from harness import *
cfg,arm=sys.argv[1],sys.argv[2]
v={
 'replay EKF+IMU': lambda c,i: run_ekf_imu(c,i),
 'DR only (truth start)': lambda c,i: run_ekf_imu(c,i,dr_only=True),
 'aux gate 9.21': lambda c,i: run_ekf_imu(c,i,aux_gate=9.21),
 'range gate 6.63': lambda c,i: run_ekf_imu(c,i,gate=6.63),
 'range 6.63 + aux 9.21': lambda c,i: run_ekf_imu(c,i,gate=6.63,aux_gate=9.21),
 'range 3.84 + aux 5.99': lambda c,i: run_ekf_imu(c,i,gate=3.84,aux_gate=5.99),
 'aug (b,s) EKF': lambda c,i: run_aug(c,i)[0],
 'aug + gates': lambda c,i: run_aug(c,i,gate=6.63,aux_gate=9.21)[0],
}
evaluate(cfg,arm,v)
