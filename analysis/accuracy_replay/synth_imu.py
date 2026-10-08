import sys, glob, os; sys.path.insert(0,'.')
import numpy as np, pandas as pd
from replay import *

class DR:
    def __init__(self, seed, bscale=1.0, sscale=1.0, nscale=1.0):
        rng=np.random.default_rng(seed)
        self.sb=8.816*np.pi/180/3600*bscale; self.N=1.6682e-4*nscale; self.ss=0.0266*sscale; self.sw=0.05
        self.rng=rng; self.bias=self.sb*rng.normal(); self.scale=self.ss*rng.normal()
        self.psi=0.0; self.t=0.0; self.dx=self.dy=0.0; self.sig=np.zeros(3)
    def reset(self): self.dx=self.dy=0.0; self.sig=np.zeros(3)
    def step(self,dx,dy,dt):
        self.t+=dt
        self.psi+=self.bias*dt+self.N*np.sqrt(dt)*self.rng.normal()
        c,s=np.cos(self.psi),np.sin(self.psi)
        ux=(1+self.scale)*(c*dx-s*dy); uy=(1+self.scale)*(s*dx+c*dy)
        st=np.hypot(dx,dy)
        if not st>0:
            w=self.sw**2; return ux,uy,(w,0.0,w)
        self.dx+=dx; self.dy+=dy
        prev=self.sig.copy()
        vp=self.sb**2*self.t**2+self.N**2*self.t; ss=self.ss**2
        self.sig=np.array([vp*self.dy**2+ss*self.dx**2, -vp*self.dx*self.dy+ss*self.dx*self.dy, vp*self.dx**2+ss*self.dy**2])
        q=self.sig-prev
        a,b,c2=q; tr=a+c2; disc=np.sqrt(max(0,(a-c2)**2/4+b*b)); l1=tr/2+disc; l2=tr/2-disc
        if l2<0:
            if l1<=0: q=np.zeros(3)
            else:
                vx,vy=(l1-c2,b) if abs(b)>1e-300 else ((1.0,0.0) if a>=c2 else (0.0,1.0))
                n=np.hypot(vx,vy); vx/=n; vy/=n; q=np.array([l1*vx*vx,l1*vx*vy,l1*vy*vy])
        return ux,uy,tuple(q)

def run_synth(cyc, imuTrue, kind, seed, bscale=1.0, sscale=1.0, gates=False, P0=1.0):
    dr=DR(seed*7919+13,bscale,sscale)
    c0=cyc[0]
    if kind=='aug':
        e=AugEkf(c0['tx'],c0['ty'],posStd=P0,sig_b=dr.sb,sig_s=dr.ss,arw=dr.N,sig_w=dr.sw)
    else:
        e=EkfImu(c0['tx'],c0['ty'],posStd=P0)
    est=[]
    g=6.63 if gates else None; ag=9.21 if gates else None
    for k,c in enumerate(cyc):
        r=imuTrue.iloc[k]
        ux,uy,q=dr.step(r.dxTrue,r.dyTrue,c['dt'])
        if kind=='aug': e.predict(c['dt'],ux,uy)
        else: e.predict_ctrl(c['dt'],ux,uy,*q)
        if len(c['R'])>=3:
            e.update_ranges(c['R'],3.0,g)
            if c['aux'] is not None:
                a=c['aux']; e.fuse_pos((a[0],a[1]),a[2],a[3],a[4],ag)
            dr.reset()
        z=e.z if kind=='aug' else e.x
        est.append((z[0],z[1],0.0))
    return np.array(est)

def drift_only(cyc, imuTrue, seed, bscale, sscale):
    dr=DR(seed*7919+13,bscale,sscale); x=cyc[0]['tx']; y=cyc[0]['ty']; out=[]
    for k,c in enumerate(cyc):
        r=imuTrue.iloc[k]; ux,uy,q=dr.step(r.dxTrue,r.dyTrue,c['dt']); x+=ux; y+=uy; out.append((x,y,0))
    return np.array(out)

if __name__=='__main__':
    cfg,arm=sys.argv[1],sys.argv[2]
    cells=sorted([d for d in glob.glob(f'runs/{cfg}/s*')],key=lambda p:int(p.rsplit('/s',1)[1]))
    for bs,ss in [(1,1),(10,1),(30,1),(10,2)]:
        res={k:[] for k in ['DR only','EKF','EKF+gates','aug','aug+gates']}
        for d in cells:
            s=int(d.rsplit('/s',1)[1])
            cyc=parse(f'{d}/ranges_{arm}_seed{s}.csv'); imu=pd.read_csv(f'{d}/imu_{arm}_seed{s}.csv')
            res['DR only'].append(err(cyc,drift_only(cyc,imu,s,bs,ss)))
            res['EKF'].append(err(cyc,run_synth(cyc,imu,'ekf',s,bs,ss)))
            res['EKF+gates'].append(err(cyc,run_synth(cyc,imu,'ekf',s,bs,ss,gates=True)))
            res['aug'].append(err(cyc,run_synth(cyc,imu,'aug',s,bs,ss)))
            res['aug+gates'].append(err(cyc,run_synth(cyc,imu,'aug',s,bs,ss,gates=True)))
        print(f'--- {cfg} {arm}  gyro-bias x{bs}  scale-sigma x{ss}')
        for k,v in res.items():
            a=np.concatenate(v); print(f'  {k:10s} pooled CEP50={np.median(a):8.1f} CEP95={np.quantile(a,.95):9.1f} mean={a.mean():8.1f}  per-seed-mean-CEP95={np.mean([np.quantile(x,.95) for x in v]):8.1f}')
