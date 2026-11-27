#!/usr/bin/env python3
"""G187: --ekf5WindowThetaWalk (DD 59). usage: theta_walk_gate.py <run set dir>. Exit 0 only if every check passes.
RUNS four runs exit 0; FATAL refused without the smoother; OFFSAME the explicit default changes no output; LINES announced only
when on; ROWS every crew trace has the full shift; COVER crew coverage (errM <= 1.73 sqrt(traceSigma)) with the switch at least
0.85 and at least 0.10 above the run without it; ACC the share within 25 m not more than 0.02 lower."""
import csv
import glob
import os
import sys


def crew(d):
    cov = a25 = n = 0
    files = sorted(glob.glob(os.path.join(d, "subjtrace_ca_tbl_full_*")))
    rows = []
    for f in files:
        k = 0
        for r in csv.DictReader(open(f)):
            e = float(r["errM"])
            cov += e <= 1.73 * max(float(r["traceSigma"]), 0.0) ** 0.5
            a25 += e <= 25.0
            k += 1
        rows.append(k)
        n += k
    return (cov / n, a25 / n, rows) if n else (None, None, rows)


def main():
    d = sys.argv[1]
    bad = 0

    def rep(ok, name, msg):
        nonlocal bad
        print(f"  G187 {'ok  ' if ok else 'FAIL'} {name} {msg}")
        bad += not ok

    rc = {n: (open(os.path.join(d, n + ".rc")).read().strip() if os.path.exists(os.path.join(d, n + ".rc")) else "none")
          for n in ("off", "false", "base", "walk", "r_cv")}
    rep(all(rc[n] == "0" for n in ("off", "false", "base", "walk")), "RUNS", f"exit codes {rc}")
    rep(rc["r_cv"] == "1", "FATAL", f"without the smoother exit {rc['r_cv']} (need 1)")
    x, y = os.path.join(d, "off"), os.path.join(d, "false")
    fa = sorted(q for q in os.listdir(x) if q != "run_id.txt") if os.path.isdir(x) else []
    fb = sorted(q for q in os.listdir(y) if q != "run_id.txt") if os.path.isdir(y) else []
    diff = [q for q in fa if q in fb and open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
    rep(bool(fa) and fa == fb and not diff, "OFFSAME", f"{len(fa)} files, differing {diff[:3]}")
    has = {n: "DD59 ekf5WindowThetaWalk" in (open(os.path.join(d, n + ".log"), errors="replace").read()
                                             if os.path.exists(os.path.join(d, n + ".log")) else "") for n in ("off", "base", "walk")}
    rep(has == {"off": False, "base": False, "walk": True}, "LINES", str(has))
    cb, ab, rb = crew(os.path.join(d, "base"))
    cw, aw, rw = crew(os.path.join(d, "walk"))
    rep(len(rb) == 4 and rb == rw and all(k == 11520 for k in rb), "ROWS", f"base {rb}, walk {rw} (need 4 x 11520)")
    if cb is None or cw is None:
        rep(False, "COVER", "no crew traces")
        rep(False, "ACC", "no crew traces")
    else:
        rep(cw >= 0.85 and cw >= cb + 0.10, "COVER", f"coverage {cb:.3f} -> {cw:.3f}")
        rep(aw >= ab - 0.02, "ACC", f"within 25 m {ab:.3f} -> {aw:.3f}")
    print(f"  G187: {bad} failed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
