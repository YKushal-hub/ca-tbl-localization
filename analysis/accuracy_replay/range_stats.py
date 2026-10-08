import glob, os, numpy as np, pandas as pd
rows=[]
for cfg in ['A','B']:
    for d in sorted(glob.glob(f'runs/{cfg}/s*')):
        s=int(d.rsplit('/s',1)[1])
        for arm in ['ca_tbl_full']:
            cur=None
            for line in open(f'{d}/ranges_{arm}_seed{s}.csv'):
                p=line.rstrip('\n').split(',')
                if p[0]=='T': cur=(float(p[1]),float(p[2]),float(p[3]),int(float(p[7])))
                elif p[0]=='R':
                    ax,ay,r,sg=float(p[2]),float(p[3]),float(p[4]),float(p[5])
                    fl=int(p[8]); aid=int(float(p[9]))
                    tr=np.hypot(cur[1]-ax,cur[2]-ay)
                    tier=0 if fl else cur[3]
                    rows.append((cfg,s,cur[0],tier,aid,ax,ay,r,sg,int(p[6]),tr))
df=pd.DataFrame(rows,columns=['cfg','seed','t','tier','aid','ax','ay','r','sigma','nlos','trueR'])
df['err']=df.r-df.trueR; df['z']=df.err/df.sigma
df['cls']=np.where(df.aid>=501,'finder',np.where(df.aid>=101,'gateway','peer'))
df.to_pickle('ranges_all.pkl')
g=df.groupby(['cfg','tier','cls']).agg(n=('z','size'),zmean=('z','mean'),zstd=('z','std'),mederr=('err',lambda x:np.median(np.abs(x))),sigmed=('sigma','median'),gt3=('z',lambda x:(np.abs(x)>3).mean()))
print(g.round(3))
def ac1(g):
    z=g.sort_values('t').z.values
    return (np.corrcoef(z[:-1],z[1:])[0,1],len(z)) if len(z)>30 else (np.nan,0)
for (cfg,tier,cls),g in df[df.cls=='gateway'].groupby(['cfg','tier','cls']):
    a=[ac1(h) for _,h in g.groupby(['seed','aid'])]
    a=[x for x in a if not np.isnan(x[0])]
    w=np.array([x[1] for x in a]); v=np.array([x[0] for x in a])
    print(cfg,'tier',tier,cls,'lag-1 autocorr (len-weighted) %.3f over %d series'%(np.sum(v*w)/np.sum(w),len(a)))
# censoring bias vs true range for tier 0
t0=df[(df.tier==0)&(df.cfg=='A')].copy()
t0['bin']=pd.cut(t0.trueR,[0,500,1000,1500,2000,3000,4000,5000,8000])
print(t0.groupby('bin',observed=True).agg(n=('z','size'),zmean=('z','mean'),zstd=('z','std'),relerr=('err',lambda x:x.mean())).round(3))
t0['rel']=t0.err/t0.trueR
print(t0.groupby('bin',observed=True).rel.agg(['mean','std']).round(3))
