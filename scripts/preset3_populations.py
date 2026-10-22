#!/usr/bin/env python3
"""Extract the contention and rare-mechanism populations of a registered preset campaign's cells (PRESET-3, PRESET-5) into
a tracked CSV, one row per cell and mechanism, with the sha256 of the run.log it was read from (raw_manifest.sha256
names the same file). G24 reads it (option B of the owner's G24 decision, 2026-10-21): a mechanism its own three short
seeds do not exercise is shown to have a population in a campaign registered before it ran, under the adopted preset.

usage: preset3_populations.py <campaign outdir with A_NEW/ and B_NEW/> <output csv>
"""
import csv
import glob
import hashlib
import os
import re
import sys

PATS = {
    "arbitration pre-emptions": (r"NW-5 arbitration[^\n]*pre-emptions (\d+)", (1,)),
    "grant tie-break verdicts": (r"tie-break WIN (\d+) DEFER (\d+)", (1, 2)),
    "queue gossip items": (r"queue items sent (\d+)", (1,)),
    "finder requests granted": (r"TA-1b finder requester escalation \(\S+\): [^\n]*granted (\d+)", (1,)),
    "Case A fixes": (r"A\(2 gateways\) (\d+)", (1,)),
    "Case B fixes": (r"B\(anchor\+peer\) (\d+)", (1,)),
}


def main():
    src, outp = sys.argv[1], sys.argv[2]
    rows = []
    for d in sorted(glob.glob(os.path.join(src, "[AB]_NEW", "s*"))):
        log = open(os.path.join(d, "run.log"), errors="replace").read()
        sha = hashlib.sha256(open(os.path.join(d, "run.log"), "rb").read()).hexdigest()
        # only the ca_tbl_full arm's block: from its first labelled line to the next arm's
        arms = [(m.start(), m.group(1)) for m in re.finditer(r"TA-1 base-station session \((\w+)\)", log)]
        blocks, prev = {}, 0
        for k, (pos, arm) in enumerate(arms):
            end = arms[k + 1][0] if k + 1 < len(arms) else len(log)
            # the CASES line precedes the TA-1 line of its own arm: start the block at the previous arm's end
            blocks[arm] = log[prev:end]
            prev = end
        b = blocks.get("ca_tbl_full", "")
        cell = os.path.relpath(d, src)
        for mech, (pat, groups) in PATS.items():
            n = sum(int(m.group(g)) for m in re.finditer(pat, b) for g in groups)
            rows.append((cell, mech, n, sha))
    with open(outp, "w") as f:
        w = csv.writer(f)
        w.writerow(["cell", "mechanism", "count", "run_log_sha256"])
        w.writerows(rows)
    tot = {}
    for _, m, n, _ in rows:
        tot[m] = tot.get(m, 0) + n
    print(tot)
    return 0


if __name__ == "__main__":
    sys.exit(main())
