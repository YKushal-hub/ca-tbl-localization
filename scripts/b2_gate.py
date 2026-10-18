#!/usr/bin/env python3
"""G136: B2 --dualBaseOnDemand (master plan v2).
  RUNS     a run's rc is not 0, or a directory is missing.
  DEFAULT  false given explicitly changes an output (run_id.txt excepted).
  FIRES    the flag-on run prints no 'B2 dualBaseOnDemand' line, decides no cycle or sleeps on none; or the control prints one.
  ENERGY   the flag-on run's ca_tbl_full totalJ is not below the control's (sleeping the radio must save energy).
"""
import csv, filecmp, os, re, sys


def main():
    d = sys.argv[1]; res = []
    bad = [r for r in ("base", "off", "on") if not os.path.isdir(os.path.join(d, r)) or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"bad: {bad}" if bad else "3 runs exit 0"))
    if bad:
        return report(res)
    fa = sorted(os.listdir(os.path.join(d, "base")))
    diff = [f for f in fa if f != "run_id.txt" and os.path.isfile(os.path.join(d, "base", f))
            and not filecmp.cmp(os.path.join(d, "base", f), os.path.join(d, "off", f), shallow=False)]
    res.append(("DEFAULT", not diff and fa == sorted(os.listdir(os.path.join(d, "off"))), f"{len(diff)} differ {diff[:2]}"))
    pat = r"B2 dualBaseOnDemand: tag-0 cycles decided (\d+), 2.4 GHz radio asleep (\d+)"
    m_on = re.findall(pat, open(os.path.join(d, "on.log"), errors="replace").read())
    m_b = re.findall(pat, open(os.path.join(d, "base.log"), errors="replace").read())
    ok = bool(m_on) and int(m_on[-1][0]) > 0 and int(m_on[-1][1]) > 0 and not m_b
    res.append(("FIRES", ok, f"flag on {m_on[-1] if m_on else None}, control {len(m_b)} lines"))
    def J(r):
        for row in csv.DictReader(open(os.path.join(d, r, "arm_summary.csv"))):
            if row["arm"] == "ca_tbl_full":
                return float(row["totalJ"])
    jo, jb = J("on"), J("base")
    res.append(("ENERGY", jo is not None and jb is not None and jo < jb, f"totalJ on {jo}, control {jb}"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G136 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
