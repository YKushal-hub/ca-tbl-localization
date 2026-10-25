#!/usr/bin/env python3
"""Check that every relative markdown link in the named files points at a tracked file or directory.

usage: check_doc_links.py FILE.md [FILE.md ...]    (run from the repository root; exits 1 if any link is dead)
A link is checked only if it is relative: URLs, mailto: and in-page anchors are skipped. A path#anchor is checked as the path.
"""
import os
import re
import subprocess
import sys

LINK = re.compile(r"\]\(([^)\s]+)\)")


def main(files):
    tracked = set(subprocess.run(["git", "ls-files"], capture_output=True, text=True, check=True).stdout.split("\n"))
    dirs = {os.path.dirname(t) for t in tracked}
    for t in list(dirs):
        while t:
            t = os.path.dirname(t)
            dirs.add(t)
    bad = 0
    for f in files:
        base = os.path.dirname(f)
        for lineno, line in enumerate(open(f, encoding="utf-8", errors="replace"), 1):
            for m in LINK.finditer(line):
                target = m.group(1).split("#")[0]
                if not target or re.match(r"^[a-z]+:", target):
                    continue
                path = os.path.normpath(os.path.join(base, target))
                if path not in tracked and path not in dirs:
                    print("DEAD  %s:%d  %s" % (f, lineno, m.group(1)))
                    bad += 1
    print("%d dead link(s) in %d file(s)" % (bad, len(files)))
    return 1 if bad else 0


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(2)
    sys.exit(main(sys.argv[1:]))
