#!/usr/bin/env python3
"""G158 (T41, TECH_DEBT 5.3): the current documents do not name flags that do not exist, and RULES.md's preset list
matches the code.

FLAGS   every --flag named in RULES.md, docs/DESIGN_DECISIONS.md and docs/TECH_DEBT_STATUS.md is registered by a
        cmd.AddValue in some scratch/*.cc program, or is a named tool option (TOOL_OPTIONS).
PRESET  every --flag in RULES.md's "The preset" list is set by kSpecProfile[] in scratch/ca_tbl_causal_sim.cc.
Stale prose about numbers cannot be checked this way; stale flag names and a drifted preset list can, and both have
happened (TECH_DEBT 5.3, M.11).
usage: doc_flags_gate.py [--docs F ...]   (--docs replaces the documents; the fixture uses it)
"""
import re, sys, glob
TOOL_OPTIONS = {"analyse", "build", "enable", "strict", "root", "docs", "resume", "require"}
argv = sys.argv[1:]
docs = argv[argv.index("--docs") + 1:] if "--docs" in argv else ["RULES.md", "docs/DESIGN_DECISIONS.md", "docs/TECH_DEBT_STATUS.md"]
flags = set()
for f in glob.glob("scratch/*.cc"):
    flags |= set(re.findall(r'cmd\.AddValue\(\s*"([A-Za-z0-9_]+)"', open(f, errors="replace").read()))
sim = open("scratch/ca_tbl_causal_sim.cc", errors="replace").read()
spec = set(re.findall(r'^\s*\{"([A-Za-z0-9_]+)",\s*"[^"]*",', sim[sim.index("kSpecProfile[]"):], re.M))
bad = 0
for doc in docs:
    t = open(doc, errors="replace").read()
    used = set(re.findall(r'(?<![A-Za-z0-9-])--([a-z][A-Za-z0-9]+)(?==|\b)', t))
    unknown = sorted(u for u in used if u not in flags and u not in TOOL_OPTIONS)
    print(f"  G158 {'ok  ' if not unknown else 'FAIL'} FLAGS  {doc}: {len(used)} named, unknown {unknown}")
    bad |= bool(unknown)
    if doc.endswith("RULES.md") and "**The preset" in t:
        blk = t[t.index("**The preset"):]
        blk = blk[:blk.index("\n\n", blk.index("- DD"))] if "- DD" in blk else blk[:800]
        pre = set(re.findall(r'--([a-z][A-Za-z0-9]+)', blk))
        missing = sorted(p for p in pre if p not in spec)
        print(f"  G158 {'ok  ' if not missing else 'FAIL'} PRESET RULES.md lists {len(pre)} preset flags; not set by kSpecProfile: {missing}")
        bad |= bool(missing)
sys.exit(1 if bad else 0)
