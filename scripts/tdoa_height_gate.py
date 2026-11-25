#!/usr/bin/env python3
"""G189: arrival stamps and the 2-D TDoA solve (DD 62, DD 63). usage: tdoa_height_gate.py <run set dir>.
RUNS exit 0; OFFSAME the explicit defaults change no output; WORLD the delay changes a crew trace; LINES each switch announced only
when on; ROWS 19 crew traces of 720 rows; OFFSET near-noiseless stamps,
devices with at least 100 accepted time-difference fixes in both runs: the median length of the mean fix error is above 0.3 m without the
correction and below half of that with it (with real stamp noise the paired offsets do not separate: 0.34 to 0.29 m); EFFECT second half of the run: the median over crew devices of the median error is lower. Coverage is printed, not judged: on this seed it
is 0.021 lower with the correction (0.778 to 0.757), and one seed does not settle it (COLLAPSE-10 does)."""
import csv
import glob
import os
import statistics as st
import sys


def crew(d):
    out, rows = [], []
    for f in sorted(glob.glob(os.path.join(d, "subjtrace_ca_tbl_full_*"))):
        r = [(float(x["t"]), float(x["errM"]), float(x["traceSigma"])) for x in csv.DictReader(open(f))]
        rows.append(len(r))
        h = [x for x in r if x[0] > 900.0]
        if h:
            out.append((st.median(x[1] for x in h), sum(x[1] <= 1.73 * max(x[2], 0.0) ** 0.5 for x in h) / len(h)))
    return out, rows


def offsets(d):
    out = {}
    for f in glob.glob(os.path.join(d, "subjtdoa_ca_tbl_full_*")):
        v = [(float(r["tdoaX"]) - float(r["trueX"]), float(r["tdoaY"]) - float(r["trueY"])) for r in csv.DictReader(open(f)) if r["verdict"] == "OK"]
        if len(v) >= 100:
            out[os.path.basename(f)] = (st.mean(x[0] for x in v) ** 2 + st.mean(x[1] for x in v) ** 2) ** 0.5
    return out


def main():
    d = sys.argv[1]
    bad = 0

    def rep(ok, name, msg):
        nonlocal bad
        print(f"  G189 {'ok  ' if ok else 'FAIL'} {name} {msg}")
        bad += not ok

    def text(n):
        p = os.path.join(d, n)
        return open(p, errors="replace").read() if os.path.exists(p) else ""

    rc = {n: text(n + ".rc").strip() or "none" for n in ("off", "false", "rd", "both", "n0", "n1")}
    rep(all(v == "0" for v in rc.values()), "RUNS", f"exit codes {rc}")
    x, y = os.path.join(d, "off"), os.path.join(d, "false")
    fa = sorted(q for q in os.listdir(x) if q != "run_id.txt") if os.path.isdir(x) else []
    fb = sorted(q for q in os.listdir(y) if q != "run_id.txt") if os.path.isdir(y) else []
    diff = [q for q in fa if q in fb and open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
    rep(bool(fa) and fa == fb and not diff, "OFFSAME", f"{len(fa)} files, differing {diff[:3]}")
    names = sorted(os.path.basename(f) for f in glob.glob(os.path.join(x, "subjtrace_*")))
    ch = [n for n in names if text("off/" + n) != text("rd/" + n)]
    rep(bool(ch), "WORLD", f"crew traces the delay changes: {len(ch)} of {len(names)}")
    on = {n: ("DD63 tdoaRubbleDelay" in text(n + ".log"), "DD62 tdoaHeightCorrect" in text(n + ".log")) for n in ("off", "rd", "both")}
    rep(on == {"off": (False, False), "rd": (True, False), "both": (True, True)}, "LINES", str(on))
    a, ra = crew(os.path.join(d, "rd"))
    b, rb = crew(os.path.join(d, "both"))
    rep(len(ra) == 19 and ra == rb and all(k == 720 for k in ra), "ROWS", f"{len(ra)} and {len(rb)} traces, rows {sorted(set(ra + rb))} (need 19 x 720)")
    p, q = offsets(os.path.join(d, "n0")), offsets(os.path.join(d, "n1"))
    k = sorted(set(p) & set(q))
    if len(k) >= 5 and a and b:
        u, v = st.median(p[z] for z in k), st.median(q[z] for z in k)
        rep(u > 0.3 and v < 0.5 * u, "OFFSET", f"{len(k)} devices, median constant offset of the fix {u:.2f} -> {v:.2f} m, near-noiseless stamps (need above 0.3 before, below 0.5x after)")
        e = [st.median(x[0] for x in z) for z in (a, b)]
        c = [st.mean(x[1] for x in z) for z in (a, b)]
        rep(e[1] < e[0], "EFFECT", f"crew median error {e[0]:.2f} -> {e[1]:.2f} m (coverage, reported: {c[0]:.3f} -> {c[1]:.3f})")
    else:
        rep(False, "OFFSET", f"{len(k)} devices with 100 fixes in both runs (need 5)")
        rep(False, "EFFECT", "no crew traces")
    print(f"  G189: {bad} failed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
