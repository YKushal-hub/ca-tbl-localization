"""Precise (reported sigma < 40 m) ranges per cycle for tag 0, by anchor class.
usage: python3 precise_ranges.py <cell> <arm> [<cell> <arm> ...]   (reads runs/<cell>/s*/ranges_<arm>_seed*.csv)
Anchor class from the id: >= 501 finder, >= 101 gateway, else peer (a relayed field node).
The sigma < 40 m test is a proxy for 2.4 GHz and UWB ranges; a finder range with an old, uncertain
position can exceed it."""
import sys, glob
import numpy as np, pandas as pd

def rows(cell, arm):
    out = []
    for d in sorted(glob.glob(f'runs/{cell}/s*')):
        s = int(d.rsplit('/s', 1)[1])
        cyc = -1
        for line in open(f'{d}/ranges_{arm}_seed{s}.csv'):
            p = line.rstrip('\n').split(',')
            if p[0] == 'T':
                cyc += 1
            elif p[0] == 'R':
                out.append((s, cyc, float(p[5]), int(float(p[9]))))
    return pd.DataFrame(out, columns=['seed', 'cyc', 'sigma', 'aid'])

args = sys.argv[1:]
for cell, arm in zip(args[0::2], args[1::2]):
    r = rows(cell, arm)
    n = r.seed.nunique() * 720
    r['cls'] = np.where(r.aid >= 501, 'finder', np.where(r.aid >= 101, 'gateway', 'peer'))
    p = r[r.sigma < 40]
    g = p.groupby('cls').size() / n
    per = p.groupby(['seed', 'cyc']).size().reindex(pd.MultiIndex.from_product([r.seed.unique(), range(720)]), fill_value=0)
    print(f'{cell:10s}{arm:16s} precise/cycle {len(p)/n:.2f}  gateway {g.get("gateway",0):.2f}  finder {g.get("finder",0):.2f}  peer {g.get("peer",0):.2f}  share(>=3) {(per>=3).mean():.3f}')
