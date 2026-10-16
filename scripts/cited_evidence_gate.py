#!/usr/bin/env python3
"""G92 -- every results/ path a tracked document cites is evidence the repository holds.

WHY THIS EXISTS. G10 asks whether backtick-cited paths resolve, but its population is
five named documents plus report/*.md. The positive-results document and every
experiments/*_RESULT.md sat outside it, and five directories those documents cite as the
source of headline results -- joint_criterion, siting_repair, claim2_rebase, claim4_src,
p1_condfloor -- were never committed on any ref and were deleted with the untracked
results/ tree. A check whose population excludes the failure cannot see it; this one is
built around the population, stated first.

POPULATION. Every TRACKED .md file at the repository root, under docs/ (not
docs/archive/, which is retained for findings and not current), under report/, and
anywhere under src/catbl-propagation-study/. A citation is `results/<top>` where the
character before `results/` is not part of a path -- so src/.../results/corpus.json is not
read as results/corpus.json. Untracked documents are outside it, deliberately: this gates
what the repository asserts. That an untracked deliverable exists is G9b's business.

RULE, per cited <top>:
  directory  PASS if at least one tracked run_id.txt exists under results/<top>/
  file       PASS if results/<top> is tracked
  pattern    (<top> followed by * { or <) PASS if any tracked top-level entry with that
             prefix passes
The propagation study writes `results/...` relative to src/catbl-propagation-study/, so a
citation that fails at the repository root is retried there before it is classified.
Otherwise the citation is classified:
  RECORDED_LOST       listed in RESULTS_NONREGENERABLE.tsv. Known and documented; reported,
                      does not fail. The document citing it should say so.
  UNTRACKED_ON_DISK   present in the working tree, not tracked            -> FAIL
  ABSENT              nowhere, and not recorded as lost                    -> FAIL
  ALLOWED             in EXCLUDED.md ```reviewed-cited-evidence``` with a reason

Usage: cited_evidence_gate.py [--docs F ...] [--root DIR]
  --docs replaces the population (used by the fixture); --root runs against another tree.
Exit 0 if nothing fails, 1 if something does, 2 if the gate could not run.
"""

import os
import re
import subprocess
import sys

CITE = re.compile(r"(?<![A-Za-z0-9_./-])results/([A-Za-z0-9_.-]+)([*{<]?)")
FILE_EXT = (".csv", ".txt", ".json", ".md", ".tsv", ".log", ".yaml", ".yml")


def git_ls(root):
    r = subprocess.run(["git", "-C", root, "ls-files"], capture_output=True, text=True)
    if r.returncode != 0:
        return None
    return [t for t in r.stdout.split("\n") if t]


def population(tracked):
    out = []
    for t in tracked:
        if not t.endswith(".md"):
            continue
        if "/" not in t:
            out.append(t)
        elif t.startswith("docs/") and not t.startswith("docs/archive/"):
            out.append(t)
        elif t.startswith("report/") or t.startswith("src/catbl-propagation-study/"):
            out.append(t)
    return sorted(out)


def allow_list(root):
    p = os.path.join(root, "EXCLUDED.md")
    out = {}
    if not os.path.exists(p):
        return out
    inblk = False
    for line in open(p, errors="replace"):
        if line.startswith("```reviewed-cited-evidence"):
            inblk = True
            continue
        if inblk and line.startswith("```"):
            break
        if inblk and line.strip():
            name, _, why = line.strip().partition(" ")
            out[name] = why.strip()
    return out


def lost_tops(root):
    p = os.path.join(root, "RESULTS_NONREGENERABLE.tsv")
    tops = set()
    if os.path.exists(p):
        for line in open(p, errors="replace"):
            if line.startswith("results/"):
                tops.add(line.split("\t")[0].split("/")[1])
    return tops


def main(argv):
    root = "."
    docs = None
    i = 1
    while i < len(argv):
        if argv[i] == "--root":
            root = argv[i + 1]
            i += 2
        elif argv[i] == "--docs":
            docs = []
            i += 1
            while i < len(argv) and not argv[i].startswith("--"):
                docs.append(argv[i])
                i += 1
        else:
            print("unknown argument %s" % argv[i])
            return 2
    tracked = git_ls(root)
    if tracked is None:
        print("ERROR: %s is not a git tree" % root)
        return 2
    if docs is None:
        docs = population(tracked)
    tset = set(tracked)
    sidecar_tops = {t.split("/")[1] for t in tracked
                    if t.startswith("results/") and t.endswith("/run_id.txt")}
    all_tops = {t.split("/")[1] for t in tracked if t.startswith("results/")}
    STUDY = "src/catbl-propagation-study/results/"
    study_tracked = {t[len(STUDY):].split("/")[0] for t in tracked if t.startswith(STUDY)}
    allowed = allow_list(root)
    lost = lost_tops(root)

    cites = {}
    for d in docs:
        path = d if os.path.isabs(d) else os.path.join(root, d)
        if not os.path.exists(path):
            print("ERROR: document %s does not exist" % d)
            return 2
        for m in CITE.finditer(open(path, errors="replace").read()):
            top, tail = m.group(1).rstrip("."), m.group(2)
            if not top:
                continue
            cites.setdefault((top, bool(tail)), set()).add(d)

    print("G92 population: %d tracked documents, %d distinct results/ citations" % (
        len(docs), len(cites)))
    fails, recorded, allowed_n, ok = [], [], 0, 0
    for (top, is_pattern), ds in sorted(cites.items()):
        if is_pattern:
            good = any(t.startswith(top) and (t in sidecar_tops or "results/" + t in tset)
                       for t in all_tops)
        elif top.endswith(FILE_EXT):
            good = ("results/" + top) in tset
        else:
            good = top in sidecar_tops
        if not good and not is_pattern and top in study_tracked:
            good = True  # resolves under the propagation study's own results/
        if good:
            ok += 1
            continue
        if top in allowed:
            allowed_n += 1
            continue
        if top in lost:
            recorded.append((top, ds))
            continue
        state = "UNTRACKED_ON_DISK" if os.path.exists(os.path.join(root, "results", top)) else "ABSENT"
        fails.append((state, top, ds))

    print("  resolve with a tracked sidecar: %d" % ok)
    print("  allow-listed with a reason:     %d" % allowed_n)
    print("  RECORDED_LOST (in RESULTS_NONREGENERABLE.tsv, reported, not failing): %d" % len(recorded))
    for top, ds in recorded:
        print("    results/%-28s cited by %s" % (top, ", ".join(sorted(ds)[:3]) + (" ..." if len(ds) > 3 else "")))
    for state, top, ds in fails:
        print("  %-17s results/%-28s cited by %s" % (
            state, top, ", ".join(sorted(ds)[:3]) + (" ..." if len(ds) > 3 else "")))
    if fails:
        print("G92 FAILED: %d cited results path(s) the repository does not hold" % len(fails))
        print("  remedy: regenerate to the cited path and track its summaries and sidecar; or")
        print("  record it in RESULTS_NONREGENERABLE.tsv; or allow-list it with a reason")
        return 1
    print("G92 PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
