#!/usr/bin/env python3
"""Housekeeping for ORACLE-ESC on a small disk (not part of the registered pipeline).
For each finished cell (rc 0) not yet handled: verify every file against the simulator's own
digest in run_id.txt; on a mismatch remove rc (the runner's resume re-runs it); else delete the
files oracle_esc_analyse.py never reads. The four preset-check cells are kept whole.
usage: prune_cells.py <outdir> <logfile> [--loop]"""
import glob, hashlib, os, re, sys, time
out, logf = sys.argv[1], sys.argv[2]
FULL = {"P_A/s321", "P_B/s321", "A_K_T12/s321", "B_K_T12/s321"}
KEEP = re.compile(r"^(trace_fixed_threshold_seed\d+\.csv|subjtrace_fixed_threshold_seed\d+_subj\d+\.csv|"
                  r"arm_summary\.csv|subject_summary\.csv|run_id\.txt|run\.log|rc|spec_profile\.txt)$")
done = set(l.split()[0] for l in open(logf)) if os.path.exists(logf) else set()
def sha(p):
    h = hashlib.sha256()
    with open(p, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()
def one(d):
    rel = os.path.relpath(d, out)
    txt = open(os.path.join(d, "run_id.txt")).read().splitlines()
    i = txt.index("digest sha256")
    dig = dict((l.split("  ", 1)[1], l.split("  ", 1)[0]) for l in txt[i + 1:] if "  " in l)
    bad = [n for n, h in dig.items() if not os.path.exists(os.path.join(d, n)) or sha(os.path.join(d, n)) != h]
    if not dig or bad:
        os.remove(os.path.join(d, "rc"))
        return f"{rel} BAD {len(bad)}/{len(dig)} -> rc removed"
    gone = 0
    if rel not in FULL:
        for n in os.listdir(d):
            if not KEEP.match(n):
                os.remove(os.path.join(d, n)); gone += 1
    return f"{rel} ok {len(dig)} verified, {gone} deleted"
while True:
    for d in sorted(glob.glob(os.path.join(out, "*", "s*"))):
        rel = os.path.relpath(d, out)
        rcp = os.path.join(d, "rc")
        if rel in done or not os.path.exists(rcp) or open(rcp).read().strip() != "0":
            continue
        try:
            msg = one(d)
        except (OSError, ValueError) as e:
            msg = f"{rel} ERROR {e}"
        if " ok " in msg:
            done.add(rel)
        with open(logf, "a") as f:
            f.write(msg + "\n")
    if "--loop" not in sys.argv:
        break
    if re.search(r"done\s*$", open(os.path.join(out, "RUN.md")).read()) and \
       os.path.exists(os.path.join(out, "RESUMED")):
        pass
    time.sleep(20)
