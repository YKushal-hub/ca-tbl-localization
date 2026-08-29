#!/usr/bin/env python3
"""D4 - provenance and completeness over all of results/. Filesystem only."""
import glob, hashlib, os, re, sys

print("# D4 — provenance and completeness\n")
dirs = sorted(set(os.path.dirname(p) for p in glob.glob("results/**/run_id.txt", recursive=True)))
print("%d directories carry a run_id.txt.\n" % len(dirs))

no_digest, mismatch, killed, dirty, orphan = [], [], [], [], []
checked = 0
for d in dirs:
    rid = os.path.join(d, "run_id.txt")
    try:
        txt = open(rid).read()
    except IOError:
        continue
    lines = txt.splitlines()
    src = next((l for l in lines if l.startswith("source ")), "")
    m = re.match(r"source (\S+) dirty (\d+)", src)
    if m and m.group(2) != "0":
        dirty.append((d, m.group(1), m.group(2)))
    if "digest sha256" not in txt:
        no_digest.append(d)
    else:
        for line in txt.split("digest sha256", 1)[1].strip().splitlines():
            mm = re.match(r"([0-9a-f]{64})\s+(.+)$", line.strip())
            if not mm:
                continue
            fp = os.path.join(d, mm.group(2))
            checked += 1
            if not os.path.exists(fp):
                mismatch.append((d, mm.group(2), "MISSING"))
            else:
                try:
                    h = hashlib.sha256(open(fp, "rb").read()).hexdigest()
                except IOError:
                    mismatch.append((d, mm.group(2), "UNREADABLE")); continue
                if h != mm.group(1):
                    mismatch.append((d, mm.group(2), "HASH MISMATCH"))
    tr = len(glob.glob(os.path.join(d, "trace_*.csv")))
    asum = os.path.join(d, "arm_summary.csv")
    has_sum = os.path.exists(asum) and os.path.getsize(asum) > 0
    if tr > 0 and not has_sum:
        killed.append((d, tr))

# orphan: directory with traces but no run_id at all
alltrace = set(os.path.dirname(p) for p in glob.glob("results/**/trace_*.csv", recursive=True))
orphan = sorted(alltrace - set(dirs))

print("## Digest verification\n")
print("- entries checked against files on disk: **%d**" % checked)
print("- directories with NO digest block: **%d**" % len(no_digest))
print("- digest failures (missing/unreadable/hash mismatch): **%d**\n" % len(mismatch))
for x in mismatch[:25]:
    print("  - `%s` — `%s` — %s" % x)
if len(mismatch) > 25:
    print("  - ... and %d more" % (len(mismatch) - 25))

print("\n## Killed mid-run (traces present, arm_summary absent)\n")
if killed:
    for d, t in killed[:30]:
        print("- `%s` (%d traces, no arm_summary)" % (d, t))
    if len(killed) > 30:
        print("- ... and %d more" % (len(killed) - 30))
else:
    print("none")

print("\n## Orphaned trace directories (no run_id.txt)\n")
if orphan:
    for d in orphan[:30]:
        print("- `%s`" % d)
    if len(orphan) > 30:
        print("- ... and %d more" % (len(orphan) - 30))
else:
    print("none")

print("\n## Dirty-tree provenance\n")
print("%d directories record `dirty != 0`.\n" % len(dirty))
# which of those are cited by a document?
cited = set()
for doc in ("CA-TBL_Technical_Report_SelfContained.md", "Simulation.md", "Architecture.md",
            "report/REPORT_DATA_PACK.md"):
    if not os.path.exists(doc):
        continue
    for m in re.finditer(r"results/[A-Za-z0-9_./-]+", open(doc, errors="replace").read()):
        cited.add(m.group(0).rstrip("./"))
cd = [x for x in dirty if any(x[0].startswith(c) for c in cited)]
print("Of those, **%d are cited by a document** — these are the unciteable ones:\n" % len(cd))
for d, s, n in cd[:30]:
    print("- `%s` — source `%s`, dirty %s" % (d, s, n))
if len(cd) > 30:
    print("- ... and %d more" % (len(cd) - 30))

st = "FINDING" if (mismatch or killed or orphan or cd or no_digest) else "OK"
print("\nSTATUS: %s" % st)
