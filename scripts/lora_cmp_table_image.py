#!/usr/bin/env python3
"""Render LORA-CMP-2's paired-difference table (results/lora_cmp2_2026-10-05/LORA_CMP2.md) as
lora_comparison_table.html in the current directory. To make the PNG:
  /opt/pw-browsers/chromium-1194/chrome-linux/chrome --headless=new --no-sandbox --disable-gpu \
    --hide-scrollbars --force-device-scale-factor=2 --window-size=1640,690 \
    --screenshot=$PWD/lora_comparison_table.png file://$PWD/lora_comparison_table.html
Numbers are parsed from the registered analysis's output, never retyped.
"""
import html
import os
import re
src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)), '..', 'results', 'lora_cmp2_2026-10-05', 'LORA_CMP2.md')).read()
secs = re.split(r'^## ', src, flags=re.M)[1:]
cfgs = ['5 km (A_main)', 'endgame (B_end)', 'N8_ring (C_n8)']
lab = {'5 km (A_main)': '5 km, 3 gateways', 'endgame (B_end)': 'Endgame, finders converged', 'N8_ring (C_n8)': '8 gateways'}
rows = [('LoRa 868 RSSI', 'LoRa 868 MHz RSSI'),
        ('LoRa 868 RSSI + gateway TDoA', 'LoRa 868 MHz RSSI + TDoA'),
        ('LoRa 2.4 GHz ToF', 'LoRa 2.4 GHz time-of-flight'),
        ('LoRa dual-band fusion, 868 RSSI+TDoA + 2.4 ToF', 'LoRa 868 + 2.4 GHz fused (TDoA)'),
        ('LoRa dual-band fusion, 868 RSSI + 2.4 ToF', 'LoRa 868 + 2.4 GHz fused (RSSI)'),
        ('Poucet-style LoRa+UWB fusion', 'Poucet (LoRa + UWB, all fused)')]
num = r'(-?[\d.]+) \[(-?[\d.]+), (-?[\d.]+)\]'
delta, base = {}, {}
for s in secs:
    title = s.splitlines()[0].strip()
    if title not in cfgs: continue
    for ln in s.splitlines():
        c = [x.strip() for x in ln.strip('|').split('|')]
        if len(c) == 5 and re.match(num, c[1]):
            delta[(title, c[0])] = [tuple(map(float, re.match(num, x).groups())) for x in c[1:4]]
        if len(c) == 9 and c[0] == 'CA-TBL':
            av = float(c[3].split()[0]); cep = float(c[4].split('/')[1]); j = float(c[7])
            base[title] = (av, cep, j)
# better-for-comparator direction: availability up, CEP down, energy down
def cell(t, kind):
    if t is None: return '<td class="na">—</td>'
    m, lo, hi = t
    sig = lo > 0 or hi < 0
    better = (m > 0) if kind == 0 else (m < 0)
    cls = 'ns' if not sig else ('cb' if better else 'tb')
    p = 3 if kind == 0 else 1
    return f'<td class="{cls}">{m:+.{p}f}<span class="ci">[{lo:.{p}f}, {hi:.{p}f}]</span></td>'
h = ['<table><thead><tr><th rowspan="2" class="m">Method<br><span class="sub">(difference from CA-TBL)</span></th>']
h += [f'<th colspan="3" class="grp">{lab[c]}</th>' for c in cfgs] + ['</tr><tr>']
for c in cfgs:
    h += ['<th>Δ availability</th><th>Δ CEP95 tag 0 (m)</th><th>Δ subject energy (J)</th>']
h += ['</tr></thead><tbody>']
h += ['<tr class="base"><td class="m">CA-TBL <span class="sub">(absolute values)</span></td>']
for c in cfgs:
    av, cep, j = base[c]
    h += [f'<td>{av:.3f}</td><td>{cep:.1f}</td><td>{j:.2f}</td>']
h += ['</tr>']
for key, name in rows:
    h += [f'<tr><td class="m">{html.escape(name)}</td>']
    for c in cfgs:
        d = delta.get((c, key))
        h += [cell(d[0] if d else None, 0), cell(d[1] if d else None, 1), cell(d[2] if d else None, 2)]
    h += ['</tr>']
h += ['</tbody></table>']
page = f'''<!doctype html><html><head><meta charset="utf-8"><style>
:root{{--bg:#ffffff;--fg:#1d2433;--muted:#6b7280;--line:#e3e6ec;--head:#f4f6f9;--cb:#dbeafe;--cbfg:#1e3a8a;--tb:#fde7c7;--tbfg:#7a3e00}}
body{{margin:0;background:var(--bg);color:var(--fg);font:14px/1.35 "DejaVu Sans",Arial,sans-serif}}
.wrap{{padding:28px 32px 24px;display:inline-block}}
h1{{font-size:20px;margin:0 0 4px}} .lead{{color:var(--muted);margin:0 0 16px;font-size:13px}}
table{{border-collapse:collapse}} th,td{{border:1px solid var(--line);padding:7px 10px;text-align:center;white-space:nowrap}}
thead th{{background:var(--head);font-size:12.5px;font-weight:600}} th.grp{{font-size:13.5px}}
td.m,th.m{{text-align:left;font-weight:600}} .sub{{font-weight:400;color:var(--muted);font-size:11.5px}}
.ci{{display:block;font-size:10.5px;opacity:.75}} tr.base td{{background:#f9fafb;font-weight:600}}
td.cb{{background:var(--cb);color:var(--cbfg);font-weight:600}} td.tb{{background:var(--tb);color:var(--tbfg);font-weight:600}}
td.ns{{color:var(--muted)}} td.na{{color:var(--muted)}}
.legend{{margin-top:14px;font-size:12px;color:var(--muted);display:flex;gap:18px;flex-wrap:wrap;max-width:1180px}}
.sw{{display:inline-block;width:12px;height:12px;border-radius:2px;vertical-align:-1px;margin-right:6px}}
</style></head><body><div class="wrap">
<h1>CA-TBL vs LoRa-based localisation, on the CA-TBL simulator</h1>
<p class="lead">20 seeds per cell, 1 800 s, spec preset. Each comparator minus CA-TBL, paired by seed: mean with 95 % seed-bootstrap interval. Availability = share of cycles with a fix (mean over 5 subjects); CEP95 = tag 0's 95th-percentile error.</p>
{"".join(h)}
<div class="legend"><span><span class="sw" style="background:var(--cb)"></span>comparator significantly better than CA-TBL</span>
<span><span class="sw" style="background:var(--tb)"></span>CA-TBL significantly better</span>
<span>grey: interval includes 0 (no detectable difference)</span><span>— : not run in that configuration</span>
<span>Sources: results/lora_cmp2_2026-10-05 (LORA-CMP-2), lora_cmp_2026-10-05, camp_final_2026-10-04</span></div>
</div></body></html>'''
open('lora_comparison_table.html', 'w').write(page)
print(len(delta), base)
