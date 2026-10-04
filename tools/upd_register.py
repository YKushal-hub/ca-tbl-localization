#!/usr/bin/env python3
"""Update one row of docs/SPEC_CONFORMANCE.md and re-parse the count table.
usage: upd_register.py ROW STATUS EVIDENCE PLAN "MOVE-NOTE (e.g. 'row 35 with MC-1')"
"""
import re, sys
import os
P = os.path.join(os.path.dirname(os.path.dirname(os.path.abspath(__file__))), "docs", "SPEC_CONFORMANCE.md")
row, status, evidence, plan, note = sys.argv[1:6]
s = open(P).read()
lines = s.split("\n")
hit = 0
old_status = None
for i, l in enumerate(lines):
    m = re.match(r"^\| (\d+) \| ([^|]+) \| ([^|]+) \| (.*) \| ([^|]+) \|$", l)
    if m and m.group(1) == row:
        old_status = m.group(3).strip()
        lines[i] = f"| {row} | {m.group(2).strip()} | {status} | {evidence} | {plan} |"
        hit += 1
assert hit == 1, hit
s = "\n".join(lines)
cnt = {}
for l in s.split("\n"):
    m = re.match(r"^\| \d+ \| [^|]+ \| ([^|]+) \|", l)
    if m:
        st = m.group(1).strip()
        key = "DIVERGENT" if st.startswith("DIVERGENT") else st
        cnt[key] = cnt.get(key, 0) + 1
kept = sum(1 for l in s.split("\n") if re.match(r"^\| \d+ \| [^|]+ \| DIVERGENT \(kept\)", l))
for k in ("CONFORMANT", "OPT-IN", "PARTIAL", "ABSENT"):
    s = re.sub(rf"\| {k} \| \d+ \|", f"| {k} | {cnt.get(k, 0)} |", s)
s = re.sub(r"\| DIVERGENT \| \d+ \(", f"| DIVERGENT | {cnt.get('DIVERGENT', 0)} (", s)
if old_status != status and note:
    s = re.sub(r"(They have moved since the first parsed count \(16/10/14/15/2\), )([^.]*)\.",
               lambda m: m.group(1) + m.group(2) + f"; {note} ({old_status} → {status})" + ".", s, count=1)
open(P, "w").write(s)
print("counts", cnt, "kept", kept, "row", row, old_status, "->", status)
