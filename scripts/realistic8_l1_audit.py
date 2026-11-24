import glob,os,re,filecmp,collections
A='/home/nethi/catbl_work/campaigns/r8_audit';B='/home/nethi/catbl_work/campaigns/realistic8'
n=diff=0;bad=[];rows=collections.defaultdict(list)
for d in sorted(glob.glob(A+'/[EQ]_*/s*')):
    r=os.path.relpath(d,A)
    if open(d+'/rc').read().strip()!='0': bad.append(r);continue
    n+=1
    for f in glob.glob(d+'/*trace_ca_tbl_full*.csv')+glob.glob(d+'/nodes_ca_tbl_full*.csv'):
        g=os.path.join(B,r,os.path.basename(f))
        if not os.path.exists(g) or not filecmp.cmp(f,g,False): diff+=1;bad.append(os.path.relpath(f,A))
    for m,p,c,a in re.findall(r'DD56 device \d+ ca_tbl_full: .* s: ([0-9.e+-]+), trailing-hour peak ([0-9.e+-]+), .*carried ([0-9.e+-]+) s, audited peak ([0-9.e+-]+)',open(d+'/run.log',errors='replace').read()):
        dur=28800 if r[0]=='Q' else 7200
        rows[r.split('/')[0]].append((float(m)+float(c)/dur,float(a),float(c)))
print('cells',n,'differing files',diff,bad[:5])
for k,v in rows.items(): print(k,len(v),'mean+carry max %.5f  audited peak max %.5f  >2.5%%: %d  carried max %.3f s'%(max(x[0] for x in v),max(x[1] for x in v),sum(max(x[:2])>.025 for x in v),max(x[2] for x in v)))
