import math, numpy as np
# --- verbatim from scratch/multimodal_generator.py ---
IMU_SIGMA_A=0.00294; IMU_GYRO_DPH=5.0; IMU_PEDESTRIAN=1.4
def imu_drift(t):
    t=np.maximum(t,0.0); acc=IMU_SIGMA_A*(t**1.5)/math.sqrt(3.0)
    gbr=IMU_GYRO_DPH*math.pi/180.0/3600.0; ct=IMU_PEDESTRIAN*t*np.sin(gbr*t)
    return np.sqrt(acc**2+ct**2)
def ekf_fuse(lora_err,s2_err,tsf):
    imu=np.maximum(imu_drift(tsf),0.5); rl=np.maximum(lora_err,1.0)
    ok2=~np.isnan(s2_err); rs2=np.where(ok2,np.abs(s2_err)+0.3,1e6)
    vi=imu**2; vl=rl**2; vs=rs2**2
    vli=(vi*vl)/(vi+vl+1e-9); v3=(vli*vs)/(vli+vs+1e-9)
    return np.sqrt(np.where(ok2,v3,vli))
nan=np.array([np.nan])
print("1) 'No-IMU' fallback in evaluate_3state.py: tsf=0, TDOA error given, Stage-2 failed")
for e in (5,50,134,254,1000,4999): print(f"   lora_err={e:5d} m -> reported fused error {ekf_fuse(np.array([float(e)]),nan,np.array([0.0]))[0]:.3f} m")
print("2) imu_drift(t) as implemented vs textbook terms (m)")
g=5.0*math.pi/180/3600
print("    t[s]  impl_total  acc(white,impl)  gyro(impl)  gyro_exact=v(1-cos)/b  | +1mg accel bias 0.5bt^2 | 50dph.. 1deg/s heading err")
for t in (5,15,30,60,120):
    tt=np.array([float(t)]); a=IMU_SIGMA_A*t**1.5/math.sqrt(3); c=1.4*t*math.sin(g*t)
    ex=1.4*(1-math.cos(g*t))/g
    bias=0.5*0.00981*t*t
    hd=1.4*t*math.radians(1.0)*t/2/t*t/t   # placeholder replaced below
    # heading error psi=0.5 deg after a 60s walk at 1.4 m/s => cross-track ~ v*t*psi/2
    print(f"   {t:5d}  {imu_drift(tt)[0]:8.2f}   {a:10.3f}      {c:8.4f}   {ex:8.4f}       | {bias:9.1f} m")
print("3) gyro term sign/periodicity: impl error at bt=pi (t = pi/g s):", 1.4*(math.pi/g)*math.sin(math.pi), "-> 'drift' returns to zero")
print("4) fused <= min(inputs) by construction? random trial")
rng=np.random.default_rng(1); l=rng.uniform(1,300,100000); t=rng.exponential(15,100000)
f=ekf_fuse(l,np.full(100000,np.nan),t); print("   fraction fused<=min(imu_floor,lora):",np.mean(f<=np.minimum(np.maximum(imu_drift(t),0.5),np.maximum(l,1))+1e-9))
print("   IMU can never worsen the output: fused/lora_err median =",np.median(f/np.maximum(l,1)))
