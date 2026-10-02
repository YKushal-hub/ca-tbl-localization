#!/usr/bin/env python3
"""G38: gateway placement shapes (docs/SPEC_CONFORMANCE_PLAN.md MS-2, --gwLayout).

--gwLayout=perimeter|cluster|star|lshape|file replaces the gateways BuildDeployment placed,
in every tier list, keeping ids 101+; legacy (default) leaves them. Each shape prints one MS-2
line with the camp and every coordinate. GEO-01 compares these shapes, so a wrong coordinate
here is a wrong study there.

Reads a directory written by scripts/gw_layout_gate.sh (boxHalf 5000, 5 gateways). Every check
runs and is reported; exit 1 if any failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL    --gwLayout=legacy IS the default: every file byte-identical, no MS-2 line.
  SHAPES  every shape's coordinates, RECOMPUTED here from the documented construction
          (frac 0.9, cluster radius 300 m, 3 arms at 1500 m, 120 degree spread), equal the
          MS-2 line's to 1e-4 m (it prints 9 significant digits), with ids 101.. and every
          point inside the box; the camp is the base camp when one is given ((3750, 3750)
          at --baseCampFrac=0.75) and (-4500, 0) otherwise.
  BUILT   the deployment the run actually used (deployment.json) has the line's gateways,
          by id, in all three tier lists (topology 0: one tri-band node each), to the 6
          significant digits the file is written with.
  FILE    the file layout is the CSV's x,y rows in order, comments and the header skipped,
          clamped to the box (7000 -> 5000).
  R       an unknown shape, a shape without --nGateways, a missing file and --topology=1
          are refused (exit 1).
"""
import json
import math
import os
import re
import sys

BOX, N, FRAC, CR, ARMS, SP = 5000.0, 5, 0.9, 300.0, 3, 1500.0
LINE = re.compile(r"MS-2 layout (\w+): (\d+) gateways, camp \(([0-9.eE+-]+), ([0-9.eE+-]+)\):((?: \d+ [0-9.eE+-]+ [0-9.eE+-]+)+)")


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def parse(d, n):
    m = LINE.search(log(d, n))
    if not m:
        raise ValueError(f"{n}: no MS-2 line")
    f = m.group(5).split()
    pts = [(int(f[i]), float(f[i + 1]), float(f[i + 2])) for i in range(0, len(f), 3)]
    return m.group(1), (float(m.group(3)), float(m.group(4))), pts


def clamp(v):
    return max(-BOX, min(BOX, v))


def expect(shape, camp, n=N, arms=ARMS, sp=SP):
    h = FRAC * BOX
    out = []
    if shape == "perimeter":
        L = 8 * h
        for k in range(n):
            s = L * k / n
            if s < h:
                x, y = -h, -s
            elif s < 3 * h:
                x, y = -h + (s - h), -h
            elif s < 5 * h:
                x, y = h, -h + (s - 3 * h)
            elif s < 7 * h:
                x, y = h - (s - 5 * h), h
            else:
                x, y = -h, h - (s - 7 * h)
            out.append((clamp(x), clamp(y)))
    elif shape == "cluster":
        for k in range(n):
            th = 2 * math.pi * k / n
            r = 0.0 if n == 1 else CR
            out.append((clamp(camp[0] + r * math.cos(th)), clamp(camp[1] + r * math.sin(th))))
    elif shape == "star":
        centre = 0.0 if camp == (0.0, 0.0) else math.atan2(-camp[1], -camp[0])
        spread = math.radians(120.0)
        for k in range(n):
            arm, step = k % arms, 1 + k // arms
            th = centre if arms == 1 else centre - spread / 2 + spread * arm / (arms - 1)
            out.append((clamp(camp[0] + sp * step * math.cos(th)), clamp(camp[1] + sp * step * math.sin(th))))
    elif shape == "lshape":
        steps = max(1, n // 2)
        dd = 2 * h / steps
        for k in range(n):
            x, y = -h, -h
            if k > 0:
                step = 1 + (k - 1) // 2
                if (k - 1) % 2 == 0:
                    y += dd * step
                else:
                    x += dd * step
            out.append((clamp(x), clamp(y)))
    return out


CASES = {
    "perimeter": ("perimeter", (-4500.0, 0.0), {}),
    "cluster": ("cluster", (-4500.0, 0.0), {}),
    "star": ("star", (-4500.0, 0.0), {}),
    "lshape": ("lshape", (-4500.0, 0.0), {}),
    "cluster_bc": ("cluster", (3750.0, 3750.0), {}),
    "star_bc": ("star", (3750.0, 3750.0), {"arms": 2, "sp": 1200.0}),
}


def main():
    if len(sys.argv) != 2:
        print("usage: gw_layout_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def c_runs():
        bad = [n for n in ["def", "legacy", "file"] + list(CASES) if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fl = sorted(f for f in os.listdir(os.path.join(d, "legacy")) if f != "run_id.txt")
        same = fd == fl and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "legacy", f), "rb").read() for f in fd)
        lines = log(d, "def").count("MS-2") + log(d, "legacy").count("MS-2")
        check("NULL", same and fd and lines == 0,
              f"--gwLayout=legacy vs default: {len(fd)} files, {'identical' if same else 'DIFFER'}; "
              f"MS-2 lines {lines} (need 0)")
    safe("NULL", c_null)

    def c_shapes():
        bad = []
        for n, (shape, camp, kw) in CASES.items():
            got_shape, got_camp, pts = parse(d, n)
            want = expect(shape, camp, **kw)
            if got_shape != shape or abs(got_camp[0] - camp[0]) > 1e-6 or abs(got_camp[1] - camp[1]) > 1e-6:
                bad.append((n, "shape/camp", got_shape, got_camp))
            if [p[0] for p in pts] != list(range(101, 101 + len(want))):
                bad.append((n, "ids", [p[0] for p in pts]))
            for (i, x, y), (wx, wy) in zip(pts, want):
                if abs(x - wx) > 1e-4 or abs(y - wy) > 1e-4 or abs(x) > BOX or abs(y) > BOX:
                    bad.append((n, i, x, y, wx, wy))
            if len(pts) != len(want):
                bad.append((n, "count", len(pts), len(want)))
        check("SHAPES", not bad,
              f"{len(CASES)} shape runs recomputed; wrong: {len(bad)} (need 0)"
              f"{': ' + str(bad[:3]) if bad else ''}")
    safe("SHAPES", c_shapes)

    def c_built():
        bad = []
        for n in list(CASES) + ["file"]:
            _, _, pts = parse(d, n)
            dj = json.load(open(os.path.join(d, n, "deployment.json")))
            for t in ("0", "1", "2"):
                got = [(a["id"], a["x"], a["y"]) for a in dj["anchors"][t]]
                if len(got) != len(pts) or any(
                        gi != pi or abs(gx - px) > 1e-5 * max(1.0, abs(px)) + 0.01 or
                        abs(gy - py) > 1e-5 * max(1.0, abs(py)) + 0.01
                        for (gi, gx, gy), (pi, px, py) in zip(got, pts)):
                    bad.append((n, t))
        check("BUILT", not bad,
              f"deployment.json tier lists vs the MS-2 line, {len(CASES) + 1} runs x 3 tiers: "
              f"{len(bad)} differ (need 0){': ' + str(bad[:3]) if bad else ''}")
    safe("BUILT", c_built)

    def c_file():
        shape, _, pts = parse(d, "file")
        want = [(-4000.5, -1000.0), (0.0, 4999.0), (3000.0, -2500.0), (5000.0, 0.0)]
        ok = shape == "file" and len(pts) == len(want) and all(
            abs(x - wx) < 1e-6 and abs(y - wy) < 1e-6 for (_, x, y), (wx, wy) in zip(pts, want))
        check("FILE", ok, f"file layout {[(x, y) for _, x, y in pts]} (need {want})")
    safe("FILE", c_file)

    def c_ref():
        for n, text in (("r_bad", "unknown --gwLayout 'hexagon'"), ("r_non", "needs --nGateways > 0"),
                        ("r_file", "cannot open --gwLayoutFile"), ("r_topo", "--gwLayout needs --topology=0")):
            got, has = rc(d, n), text in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G38 {tag:7s} {name:6s} {why}")
    print(f"  G38: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
