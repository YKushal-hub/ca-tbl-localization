#!/usr/bin/env python3
"""Assemble SUMMARY.md from whatever sections exist. Missing sections are stated."""
import os, re, sys, datetime
D = sys.argv[1]
MODS = ["D0","D1","D2","D3","D4","D5","D6","D7","D8"]
TITLE = {"D0":"preflight","D1":"arm liveness matrix","D2":"flag-confound audit",
         "D3":"degeneracy pre-screen","D4":"provenance & completeness",
         "D5":"gates & negative fixtures","D6":"document reconciliation",
         "D7":"clean integrity test","D8":"build hygiene"}
status = {}
for line in open(os.path.join(D,"STATUS.txt")) if os.path.exists(os.path.join(D,"STATUS.txt")) else []:
    p = line.split()
    if len(p) >= 2 and p[0] in MODS: status[p[0]] = (p[1], " ".join(p[2:]))

def sect(m, start, end=None):
    f = os.path.join(D, m + ".md")
    if not os.path.exists(f): return None
    t = open(f, errors="replace").read()
    i = t.find(start)
    if i < 0: return None
    j = t.find(end, i) if end else -1
    return t[i:j if j > 0 else len(t)]

print("# Overnight diagnostic — SUMMARY\n")
print("Generated %s\n" % datetime.datetime.now().strftime("%Y-%m-%d %H:%M"))
print("Nothing in the tree was changed. No commits, no source edits, no `results/` deletions,")
print("no ledger updates. Findings are recorded for Kushal to act on, not acted on.\n")

print("## 1. Modules\n")
print("| module | what it does | outcome | notes |")
print("|---|---|---|---|")
for m in MODS:
    s, n = status.get(m, ("NOT RUN", ""))
    print("| **%s** | %s | **%s** | %s |" % (m, TITLE[m], s, n))

print("\n## 2. Degenerate / inert / broken cells (D1)\n")
b = sect("D1", "## Flagged cells", "## Full matrix")
print(b.split("\n",1)[1].strip() if b else "_D1 section absent — module did not complete._")

print("\n## 3. Confounded campaigns (D2)\n")
b = sect("D2", "## Explicit scoping")
print(b.split("\n",1)[1].strip() if b else "_D2 section absent._")

print("\n## 4. Provenance and digest failures (D4)\n")
b = sect("D4", "## Digest verification", "## Killed mid-run")
print(b.split("\n",1)[1].strip() if b else "_D4 section absent._")
b = sect("D4", "## Killed mid-run", "## Orphaned")
if b: print("\n**Killed mid-run:**\n" + b.split("\n",1)[1].strip()[:1200])

print("\n## 5. Document numbers that could not be reproduced (D6)\n")
b = sect("D6", "| claim |", "## Claims not")
print(b.strip() if b else "_D6 section absent._")

print("\n## 6. D7 — clean integrity test\n")
f = os.path.join(D, "D7.md")
if os.path.exists(f):
    t = open(f, errors="replace").read()
    if "SKIPPED" in t:
        print(t.split("\n",2)[2].strip()[:1500])
    else:
        b = sect("D7", "## Registered prediction")
        print(b.strip()[:1500] if b else t[:1500])
else:
    print("_D7 did not run._")

print("\n## 7. Tree state\n")
for lbl, p in (("start","/tmp/tree_start.txt"), ("end", os.path.join(D,"tree_end.txt"))):
    if os.path.exists(p):
        c = open(p, errors="replace").read().strip()
        print("- **%s**: %s" % (lbl, "clean (no tracked modifications)" if not c else "```\n%s\n```" % c))
    else:
        print("- **%s**: not recorded" % lbl)
s = open("/tmp/tree_start.txt", errors="replace").read().strip() if os.path.exists("/tmp/tree_start.txt") else None
e = open(os.path.join(D,"tree_end.txt"), errors="replace").read().strip() if os.path.exists(os.path.join(D,"tree_end.txt")) else None
print("\n**MATCH**" if s == e else "\n**MISMATCH — the tree changed during the batch. Investigate before trusting anything below.**")

print("\n## 8. What was NOT done, and why\n")
print("- **No fresh-clone build (D8).** ~40 min of budget; D5 exercises the built binary instead. "
      "The clone test therefore remains unverified against the post-D1 engine.")
print("- **`baseline_v7` was not generated.** It is a close-out item, not a diagnostic, and §12 "
      "forbids changing anything.")
print("- **No document or ledger updates.** W1, W4, Claim 3's new entry, Claim 7's review and "
      "`Architecture.md:71` (r80 909.5 -> 1149.5) all remain outstanding.")
print("- **Claim 9 was not moved.** The B2 `no_integrity` observation is confounded; D7 is the "
      "test that would settle it, and its outcome is in §6.")
print("- **D1 covers 6 configurations x 6 arms at 1 seed.** It detects degeneracy, not effect "
      "size. A cell reading `ok` here is not evidence of a result.")
print("- **D3 screens existing directories only.** It cannot flag a configuration never run.")
print("- **D5's negative fixtures cover the digest check and the degeneracy screen.** The gate "
      "suite's individual assertions were run but not each given a failing fixture.")
print("- **No campaign was re-run to replace a confounded one.** D2 lists what is affected; "
      "regenerating is a decision for Kushal.")
