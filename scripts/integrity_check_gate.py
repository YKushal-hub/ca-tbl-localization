#!/usr/bin/env python3
"""G163: --integrityCheck=false makes ca_tbl_full run exactly as ca_tbl_no_integrity, and true is the check running.

Reads the run set scripts/integrity_check_gate.sh writes. What each failure looks like on disk:
  RUNS    off/, on/, def/, tru/ exit 0                                        (a crash under the flag)
  EQUIV   in off/, every ca_tbl_no_integrity output has a ca_tbl_full twin   (the flag removing more, or less, than
          byte-identical except the arm-name column; the two arm_summary      the check: anything else that differs)
          rows identical but for the arm name
  EFFECT  on/'s ca_tbl_full flags suspects (> 0); off/'s flags none            (the check never running, or still
                                                                              running when switched off)
  OFF     def/ and tru/ byte-identical, run_id.txt and spec_profile.txt's     (true not being the check as it was;
          integrityCheck line aside                                           the preset's setting excused)
"""
import csv
import os
import sys

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G163 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def rows(p):
    return list(csv.DictReader(open(p))) if os.path.exists(p) else []


def main():
    d = sys.argv[1]
    rcs = {n: rc(d, n) for n in ("off", "on", "def", "tru")}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    o = os.path.join(d, "off")
    bad, n = [], 0
    if os.path.isdir(o):
        for f in sorted(os.listdir(o)):
            if "ca_tbl_no_integrity" not in f:
                continue
            g = f.replace("ca_tbl_no_integrity", "ca_tbl_full")
            n += 1
            if not os.path.exists(os.path.join(o, g)):
                bad.append("missing " + g)
                continue
            a, b = open(os.path.join(o, f), "rb").read(), open(os.path.join(o, g), "rb").read()
            if a != b and a.replace(b"ca_tbl_no_integrity", b"ca_tbl_full") != b:
                bad.append(g)
        summ = rows(os.path.join(o, "arm_summary.csv"))
        vals = {r["arm"]: {k: v for k, v in r.items() if k != "arm"} for r in summ}
        same = vals.get("ca_tbl_full") is not None and vals.get("ca_tbl_full") == vals.get("ca_tbl_no_integrity")
        report(n > 0 and not bad and same, "EQUIV", f"{n} file pairs, differing {bad[:3]}; arm_summary rows equal "
               f"but for the arm name {same}")
    else:
        report(False, "EQUIV", "off/ absent")
    s_on = [r for r in rows(os.path.join(d, "on", "arm_summary.csv")) if r["arm"] == "ca_tbl_full"]
    s_off = [r for r in rows(os.path.join(o, "arm_summary.csv")) if r["arm"] == "ca_tbl_full"]
    son = float(s_on[0]["suspects"]) if s_on else -1
    soff = float(s_off[0]["suspects"]) if s_off else -1
    report(son > 0 and soff == 0, "EFFECT", f"suspects with the check {son:.0f} (need > 0), without {soff:.0f} (need 0)")
    x, y = os.path.join(d, "def"), os.path.join(d, "tru")
    if os.path.isdir(x) and os.path.isdir(y):
        def body(p):
            t = open(p, "rb").read()
            if os.path.basename(p) == "spec_profile.txt":
                t = b"\n".join(ln for ln in t.split(b"\n") if not ln.startswith(b"integrityCheck"))
            return t
        fa = sorted(q for q in os.listdir(x) if q != "run_id.txt")
        fb = sorted(q for q in os.listdir(y) if q != "run_id.txt")
        diff = [q for q in fa if q not in fb or body(os.path.join(x, q)) != body(os.path.join(y, q))]
        diff += [q for q in fb if q not in fa]
        report(not diff and bool(fa), "OFF", f"def/ against tru/: {len(fa)} files, differing {diff[:3]} (need none)")
    else:
        report(False, "OFF", "def/ or tru/ absent")
    print(f"  G163: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
