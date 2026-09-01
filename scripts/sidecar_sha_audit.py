#!/usr/bin/env python3
"""G13 — every sidecar's recorded source SHA resolves, in history or through SHA_MAP.md.

A run_id.txt records `source <sha>`. That SHA is the only link between a results
directory and the code that produced it. If it resolves nowhere, the link is broken —
by a history rewrite, a partial clone, or a sidecar written by a build that was never
committed.

The 2026-09-01 attribution rewrite moved 30 commits, and 1283 sidecars cite six SHAs
that no longer exist in history. They resolve through SHA_MAP.md instead. This gate
asserts that every sidecar SHA lands in one of those two places, so a third route --
a SHA that exists in neither -- is caught rather than discovered.

Exit 0 if every SHA resolves, 1 otherwise.
"""
import glob, os, re, subprocess, sys


def history_shas():
    out = subprocess.run(["git", "log", "--all", "--format=%H"],
                         capture_output=True, text=True).stdout.split()
    return {s[:7] for s in out} | set(out)


def map_shas(path="SHA_MAP.md"):
    """Old SHAs recorded in the map. Both columns count as resolvable."""
    if not os.path.exists(path):
        return set(), False
    s = open(path, errors="replace").read()
    found = set(m.group(1) for m in re.finditer(r"`([0-9a-f]{7,40})`", s))
    return {x[:7] for x in found} | found, True


def main():
    hist = history_shas()
    mapped, have_map = map_shas()
    if not have_map:
        print("  SHA_MAP.md ABSENT -- rewritten SHAs have no resolution path")
    counts = {"history": 0, "sha_map": 0, "UNRESOLVED": 0}
    unresolved = {}
    for f in sorted(glob.glob("results/**/run_id.txt", recursive=True)):
        try:
            t = open(f, errors="replace").read()
        except IOError:
            continue
        m = re.search(r"^source (\S+)", t, re.M)
        if not m:
            continue
        sha = m.group(1)
        if sha in hist or sha[:7] in hist:
            counts["history"] += 1
        elif sha in mapped or sha[:7] in mapped:
            counts["sha_map"] += 1
        else:
            counts["UNRESOLVED"] += 1
            unresolved.setdefault(sha, []).append(f)
    tot = sum(counts.values())
    print("  sidecars examined: %d" % tot)
    print("    resolve in history      : %d" % counts["history"])
    print("    resolve via SHA_MAP.md  : %d" % counts["sha_map"])
    print("    UNRESOLVED anywhere     : %d" % counts["UNRESOLVED"])
    for sha, files in list(unresolved.items())[:10]:
        print("      %s  cited by %d sidecar(s), e.g. %s" % (sha, len(files), files[0]))
    if counts["sha_map"]:
        print("  note: SHAs resolving via the map are NOT a defect -- they record what the")
        print("        run recorded, and sidecars are evidence and are never edited.")
    return 1 if counts["UNRESOLVED"] else 0


if __name__ == "__main__":
    sys.exit(main())
