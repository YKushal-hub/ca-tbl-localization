#!/usr/bin/env python3
"""Regenerate docs/SCENARIO_FLAGS.md sect 3 from --PrintHelp, and (optionally) add a
header bullet and a sect-1 row for a new flag.

usage: regen_flags.py [--bullet FLAG "TEXT with {row}"] [--row "| ... |"] [--date YYYY-MM-DD]
"""
import re, subprocess, sys
import os
R = os.path.dirname(os.path.dirname(os.path.abspath(__file__))) + "/"
DOC = R + "docs/SCENARIO_FLAGS.md"
B = R + "build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
ONES = "zero one two three four five six seven eight nine ten eleven twelve thirteen fourteen fifteen sixteen seventeen eighteen nineteen".split()
TENS = {20: "twenty", 30: "thirty", 40: "forty", 50: "fifty", 60: "sixty", 70: "seventy", 80: "eighty", 90: "ninety"}
WORDS = ONES + [TENS[(n // 10) * 10] + ("" if n % 10 == 0 else "-" + ONES[n % 10]) for n in range(20, 100)]

help_ = subprocess.run([B, "--PrintHelp"], capture_output=True, text=True).stdout
flags = []
inprog = False
for line in help_.split("\n"):
    if line.startswith("Program Options:"):
        inprog = True
        continue
    if inprog and line.startswith("General Arguments"):
        break
    m = re.match(r"^    --(\S+):\s+(.*)$", line)
    if inprog and m:
        d = re.search(r"\[([^\[\]]*)\]\s*$", m.group(2))
        flags.append((m.group(1), d.group(1) if d else None))
rows = {f: i + 1 for i, (f, _) in enumerate(flags)}

s = open(DOC).read()
args = sys.argv[1:]
bullet = row = None
while args:
    a = args.pop(0)
    if a == "--bullet":
        bullet = (args.pop(0), args.pop(0))
    elif a == "--row":
        row = args.pop(0)

# sect 3 table
i = s.index("## 3. Every flag, with its default")
j = s.index("|---|---|---|\n", i) + len("|---|---|---|\n")
k = s.find("\n\n", j)
k = len(s) if k < 0 else k + 1
body = "".join(f"| {n} | `--{f}` | {'`' + d + '`' if d is not None else '(empty)'} |\n"
               for n, (f, d) in enumerate(flags, 1))
s = s[:j] + body + s[k:]
s = re.sub(r"It lists \d+ flags\.", f"It lists {len(flags)} flags.", s)

if bullet:
    f, text = bullet
    if f"- `--{f}`" not in s:
        anchor = "\n\nA fresh `--PrintHelp` differs from the `ff4f8bb` one in those rows only, and adds the\n"
        assert s.count(anchor) == 1
        s = s.replace(anchor, "\n" + text.format(row=rows[f]).rstrip("\n") + anchor, 1)
        m = re.search(r"and adds the\n(\S+) names\.", s)
        cur = m.group(1)
        n = (WORDS.index(cur) if cur in WORDS else int(cur)) + 1
        s = s.replace(f"and adds the\n{cur} names.", f"and adds the\n{WORDS[n] if n < len(WORDS) else n} names.")
# recompute every "(row N of sect 3" in the header bullets
def fix(m):
    names = re.findall(r"`--([A-Za-z0-9]+)`", m.group(0))
    return m.group(0)
hdr_end = s.index("A fresh `--PrintHelp`")
hdr = s[:hdr_end]
def rowfix(m):
    f = m.group(1)
    return m.group(0) if f not in rows else re.sub(r"\(row \d+ of", f"(row {rows[f]} of", m.group(0))
hdr = re.sub(r"- `--([A-Za-z0-9]+)` \(row \d+ of", rowfix, hdr)
s = hdr + s[hdr_end:]
four = [rows.get(f) for f in ("gwArcDeg", "gwArcCentreDeg", "trailDeviateSide", "searchLaunchAtS")]
s = re.sub(r"Those four are rows \d+, \d+, \d+ and \d+ of §3\.",
           f"Those four are rows {four[0]}, {four[1]}, {four[2]} and {four[3]} of §3.", s)
if row:
    marker = "\n## 2. Help text corrected, default unchanged"
    if ("| " + row.split("|")[1].strip() + " |") not in s[:s.index(marker)]:
        sect = s[:s.index(marker)].rstrip("\n")
        s = sect + "\n" + row + "\n" + s[s.index(marker):]
open(DOC, "w").write(s)
print(f"{len(flags)} flags; rows:", {f: rows[f] for f in rows if bullet and f == bullet[0]})
