#!/usr/bin/env python3
"""G101: Plan A, A2, the error-state filter (--imuErrorStates; CaTblEkf::EnableErrorStates).

Reads a directory written by scripts/ie_gate.sh. Every check runs and is reported; exit 1 if
any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  UNIT    the C++ filter (scratch/ca_tbl_ie_test) and the Python reference
          (make_ie_vectors.py, analytic Jacobian) differ by more than 1e-9 relative on any of
          x, y, psi, b, s, pxx, pxy, pyy after 200 cycles, or either did not run. The two
          agree to all 12 printed digits when written; 1e-9 leaves room for the last digit.
  RC      a simulator run that must succeed exited non-zero.
  PRESET  the preset sets the flag (since 2026-10-09): --imuErrorStates=true given explicitly
          is the preset, every output file byte-identical except run_id.txt and
          spec_profile.txt (which lists explicit settings).
  REACH   tag 0's trace and every subject's trace differ between the preset and the old
          filter (--imuErrorStates=false) in both arms: the error-state filter runs in every
          filter of every arm. A fire is an arm or a subject whose estimates did not move.
  REFUSE  --imuErrorStates without --imuPredict exits 1 with its FATAL line.
"""
import filecmp
import glob
import os
import sys

ARMS = ["ca_tbl_full", "static_2p4"]
KEYS = ["x", "y", "psi", "b", "s", "pxx", "pxy", "pyy"]


def read_vec(p):
    out = {}
    with open(p) as h:
        for line in h:
            k, v = line.split()
            out[k] = float(v)
    return out


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as h:
        return int(h.read().strip())


def main():
    if len(sys.argv) != 2:
        print("usage: ie_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    for n in ["ie_cpp", "ie_py", "preset", "explicit", "old", "r_noimu"]:
        if not os.path.isfile(os.path.join(d, n + ".rc")):
            print(f"  G101 MISSING input {n}")
            return 2
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    try:
        a, b = read_vec(os.path.join(d, "ie_cpp.txt")), read_vec(os.path.join(d, "ie_py.txt"))
        worst = max(abs(a[k] - b[k]) / max(abs(b[k]), 1e-300) for k in KEYS)
        ok = rc(d, "ie_cpp") == 0 and rc(d, "ie_py") == 0 and worst <= 1e-9
        why = f"worst relative difference {worst:.3e} over {KEYS}"
    except (KeyError, ValueError, OSError) as e:
        ok, why = False, f"unreadable vectors: {e}"
    check("UNIT", ok, why)

    bad = [n for n in ["preset", "explicit", "old"] if rc(d, n) != 0]
    check("RC", not bad, f"non-zero exit: {bad}")

    skip = ("run_id.txt", "spec_profile.txt")
    fb = sorted(os.listdir(os.path.join(d, "preset")))
    fo = sorted(os.listdir(os.path.join(d, "explicit")))
    diff = [f for f in fb if f not in skip and (f not in fo or not filecmp.cmp(
        os.path.join(d, "preset", f), os.path.join(d, "explicit", f), shallow=False))]
    diff += [f for f in fo if f not in fb]
    check("PRESET", fb and not diff, f"{len(fb)} files, differing: {diff[:5]}")

    same = []
    n = 0
    for arm in ARMS:
        for f in [f"trace_{arm}_seed1.csv"] + [os.path.basename(p) for p in sorted(
                glob.glob(os.path.join(d, "preset", f"subjtrace_{arm}_seed1_subj*.csv")))]:
            n += 1
            pb, po = os.path.join(d, "preset", f), os.path.join(d, "old", f)
            if not os.path.exists(po) or filecmp.cmp(pb, po, shallow=False):
                same.append(f)
    check("REACH", n >= 2 * 5 and not same, f"traces compared {n}, unchanged against the old filter: {same}")

    with open(os.path.join(d, "r_noimu.log")) as h:
        reason = "FATAL: --imuErrorStates needs --imuPredict=true" in h.read()
    check("REFUSE", rc(d, "r_noimu") == 1 and reason, f"exit {rc(d, 'r_noimu')} (need 1), reason present {reason}")

    nbad = 0
    for name, ok, why in res:
        print(f"  G101 {'ok  ' if ok else 'FAIL'} {name:8s} {why}")
        nbad += 0 if ok else 1
    print(f"  G101 {len(res) - nbad}/{len(res)} checks pass")
    return 1 if nbad else 0


if __name__ == "__main__":
    sys.exit(main())
