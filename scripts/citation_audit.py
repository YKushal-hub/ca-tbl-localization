#!/usr/bin/env python3
"""Every backtick-cited path in a tracked document must resolve in the repository.

Simulation.md -- one of the two named deliverables -- had zero commits for the whole
project because nothing ever asked whether the repository contained the things its
documents talk about. This asks, repeatably.

Exit 0 if every citation resolves, 1 otherwise. Used by G10.

Categories:
  TRACKED               in `git ls-files`
  TRACKED_ELSEWHERE     basename resolves to a tracked file at another path. Counts
                        as a pass but is reported separately: a citation that only
                        resolves by basename is fragile.
  ALLOWED               on the enumerated allowance list below
  UNTRACKED_BUT_EXISTS  on disk, untracked, not allowed  -> FAILS
  ABSENT                nowhere                          -> FAILS
"""
import glob, os, re, subprocess, sys

DOCS = ["Architecture.md", "Simulation.md", "src/catbl-propagation-study/DEFECTS.md",
        "CA-TBL_Technical_Report_SelfContained.md"] + sorted(glob.glob("report/*.md"))

# Enumerated, NOT a blanket exemption for anything untracked. Each entry states why.
ALLOWED = {
    "CA_TBL_HANDOFF_v4.md":        "must stay untracked by project rule; G8 asserts it",
    "SESSION_CHANGELOG.md":        "working log, must stay untracked; G8 asserts it",
    "CA_TBL_SIMULATION_REPORT.md": "earlier report, deliberately untracked; G8 asserts it",
    "subject_summary.csv":         "a per-run output the scenario writes under --measuredSubjects "
                                   "> 1; registrations cite it as a metric's source, not a repo path",
    "spec_profile.txt":            "a per-run output the scenario writes under --specProfile; cited "
                                   "as where a run records its settings, not a repo path",
    "gateways.csv":                "a file of the external Bonn dataset (github.com/mclab-hbrs/lora-bonn), "
                                   "not vendored; report/TIER1_VALIDATION.md records its sha256",
    "samples.csv":                 "a file of the external Bonn dataset (survey/samples.csv), not "
                                   "vendored; report/TIER1_VALIDATION.md records its sha256",
    "context.md":                  "S2's research file on branch work/2026-10-09-acc-reruns, cited by "
                                   "report/PREREG_ACC_A_FILTER.md as the source of the 3-54x measurement",
    "PREREG_ACC_B_HYBRID.md":      "on both unmerged Part B branches (work/2026-10-09-acc-part-b and "
                                   "work/2026-10-09-acc-reruns) with different content; named by "
                                   "report/S2C_CROSS_BRANCH_AUDIT.md as the collision the owner decides",
    "ACC_B_RESULTS.md":            "on both unmerged Part B branches, different content; named by "
                                   "report/S2C_CROSS_BRANCH_AUDIT.md as the collision the owner decides",
    "VERIFICATION_2026_10_09_LOCAL.md": "the local machine's verification on branch "
                                   "work/2026-10-09-local-verification (not merged); cited by "
                                   "report/S2C_CROSS_BRANCH_AUDIT.md as evidence for G63/G87/G89",
}
# Bare filenames that live inside excluded results/ directories. Legitimate because
# the directory is recorded in EXCLUDED.md; the file inside it is not separately tracked.
ALLOWED_IN_EXCLUDED = {"PREDICTION.md", "PREDICTION_P1_v2.md", "ENVELOPE_TABLE.txt"}

FILE_PAT = re.compile(r"`([A-Za-z0-9_./-]+\.(?:cc|h|py|sh|md|csv|json|txt|yaml|yml|pkl|npy))`")
DIR_PAT = re.compile(r"`(results/[A-Za-z0-9_./-]+)`")


def excluded_prefixes():
    if not os.path.exists("EXCLUDED.md"):
        return []
    out, inblk = [], False
    for line in open("EXCLUDED.md", errors="replace"):
        if line.startswith("```reviewed-exclusions"):
            inblk = True; continue
        if inblk and line.startswith("```"):
            break
        if inblk and line.strip():
            out.append(line.strip().rstrip("/"))
    return out


def main():
    tracked = set(t for t in subprocess.run(["git", "ls-files"], capture_output=True,
                                            text=True).stdout.split("\n") if t)
    basenames = {}
    for t in tracked:
        basenames.setdefault(os.path.basename(t), []).append(t)
    excl = excluded_prefixes()

    docs = [d for d in DOCS if os.path.exists(d)]
    # The docstring said "tracked document" while this globbed the filesystem. Two
    # report/*.md were untracked and were being read as if they were in the
    # repository -- the same class of error the audit exists to catch, in the audit.
    # Source documents that are not themselves tracked are reported, not silently used.
    untracked_docs = [d for d in docs if d not in tracked]
    cited, citedd = {}, {}
    for d in docs:
        txt = open(d, errors="replace").read()
        for m in FILE_PAT.finditer(txt):
            cited.setdefault(m.group(1), set()).add(d)
        for m in DIR_PAT.finditer(txt):
            citedd.setdefault(m.group(1).rstrip("/"), set()).add(d)

    cats = {}
    for p, src in sorted(cited.items()):
        b = os.path.basename(p)
        if p in tracked:
            k = "TRACKED"
        elif p in ALLOWED or b in ALLOWED:
            k = "ALLOWED"
        elif b in ALLOWED_IN_EXCLUDED or any(p.startswith(e) for e in excl):
            k = "ALLOWED"
        elif b in basenames:
            k = "TRACKED_ELSEWHERE"
        elif os.path.exists(p) or glob.glob("**/" + b, recursive=True):
            k = "UNTRACKED_BUT_EXISTS"
        else:
            k = "ABSENT"
        cats.setdefault(k, []).append((p, sorted(src)))

    print("citation audit over %d source documents (%d tracked, %d NOT), %d distinct cited paths"
          % (len(docs), len(docs) - len(untracked_docs), len(untracked_docs), len(cited)))
    for d in untracked_docs:
        print("  SOURCE DOC NOT TRACKED: `%s` -- its citations are audited, but the"
              " document itself is not in the repository" % d)
    for k in ("TRACKED", "TRACKED_ELSEWHERE", "ALLOWED", "UNTRACKED_BUT_EXISTS", "ABSENT"):
        print("  %-22s %d" % (k, len(cats.get(k, []))))
    if cats.get("TRACKED_ELSEWHERE"):
        print("  note: TRACKED_ELSEWHERE passes, but a citation that resolves only by")
        print("        basename is fragile -- it breaks if the file moves.")

    bad = cats.get("UNTRACKED_BUT_EXISTS", []) + cats.get("ABSENT", [])
    for p, src in bad:
        where = "on disk but untracked" if os.path.exists(p) else "ABSENT from disk"
        print("  UNRESOLVED: `%s` (%s) <- %s"
              % (p, where, ", ".join(os.path.basename(x) for x in src)))

    dbad = []
    for d in sorted(citedd):
        ist = any(t == d or t.startswith(d + "/") for t in tracked)
        isex = any(d.startswith(e) for e in excl)
        if not ist and not isex:
            dbad.append(d)
    print("  results dirs cited: %d, neither committed nor excluded: %d"
          % (len(citedd), len(dbad)))
    for d in dbad:
        print("  UNRESOLVED DIR: `%s`" % d)

    if untracked_docs:
        print("  (untracked source documents are reported for a decision, not failed on)")
    return 1 if (bad or dbad) else 0


if __name__ == "__main__":
    sys.exit(main())
