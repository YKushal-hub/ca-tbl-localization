#!/usr/bin/env python3
"""G191: Phase 0 (DD 43, scripts/collapse/phase0.py) and the digging order (plan B6, DD 41 / 51, scripts/collapse/score_dig.py).

Phase 0
  P-ROWS   phase0.py's defaults equal their source rows: coremark_nrf 212 (NRF-CM), i_cpu_a 3.3 mA (NRF-ICPU0), v 3.0 V (NRF-ICPU0)
  P-DD43   with DD 43's measured inputs the script prints DD 43's numbers: ca_tbl_full 1077 us, CoreMark 25134, 89.4 J/h ->
           127.7 ms (DD 43 rounds it to 128), 5.1 % of the cycle, 1.82 J/h, 2.0 %, k_dp <= 3.9; static_868 1019 us, 28.2 J/h -> k_dp <= 1.6
  P-RUN    (with --sim) --phase0Timing prints one PHASE0 line and leaves every CSV byte-identical to the same run without it
Digging order
  D-SURV   survival() equals DEBRUY-T2 (333/379 day 0, 20/57 day 1, 3/35 days 2-3, 0 from day 4), read from SOURCES_COLLAPSE.md
  D-TIME   dig_time: t_dig within the radius, t_dig (1 + (e^2 - r^2) / r^2) outside it (e = 2r -> 4 t_dig)
  D-LEAK   the system policies (id, nearest, confident, spt) give the same order when the truth is moved; the oracle does not
  D-RD     range_or_dig: constant sigmas start at the first epoch; steeply falling sigmas wait; never before every worker has
           a window of history
Fixtures (--fixtures): a mutated copy of each script must fail the check it breaks (a policy reading the truth, a survival
step changed, a default constant changed, the dig-time formula changed); the clean copies pass.
usage: phase0_dig_gate.py [--fixtures] [--sim BIN]
"""
import importlib.util, io, math, os, re, subprocess, sys, tempfile, contextlib, filecmp, glob

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
P0 = os.path.join(ROOT, "scripts", "collapse", "phase0.py")
DIG = os.path.join(ROOT, "scripts", "collapse", "score_dig.py")
SRC = os.path.join(ROOT, "docs", "SOURCES_COLLAPSE.md")


def load(path, name, patch=None):
    s = open(path).read()
    if patch:
        a, b = patch
        assert a in s, f"fixture patch target missing in {path}: {a!r}"
        s = s.replace(a, b, 1)
    spec = importlib.util.spec_from_loader(name, loader=None)
    m = importlib.util.module_from_spec(spec)
    exec(compile(s, path, "exec"), m.__dict__)
    return m, s


def row(rid):
    for l in open(SRC):
        if l.startswith(f"| {rid} |"):
            return l
    return ""


def phase0_out(mod, args):
    old = sys.argv
    sys.argv = ["phase0.py"] + args
    buf = io.StringIO()
    try:
        with contextlib.redirect_stdout(buf):
            mod.main()
    finally:
        sys.argv = old
    return buf.getvalue()


def checks(p0, p0src, dig):
    out = []
    # P-ROWS
    d = dict(re.findall(r'add_argument\("--(\w+)", type=float, default=([0-9.e-]+)\)', p0src))
    cm = re.search(r"CoreMark (\d+)", row("NRF-CM"))
    ic = re.search(r"\| NRF-ICPU0 \| ([0-9.]+) mA", row("NRF-ICPU0"))
    ok = (cm and ic and float(d.get("coremark_nrf", "nan")) == float(cm.group(1))
          and abs(float(d.get("i_cpu_a", "nan")) - float(ic.group(1)) * 1e-3) < 1e-12 and float(d.get("v", "nan")) == 3.0)
    out.append(("P-ROWS", ok, f"defaults {d.get('coremark_nrf')} CoreMark, {d.get('i_cpu_a')} A, {d.get('v')} V; rows "
                f"{cm.group(1) if cm else None} CoreMark, {ic.group(1) if ic else None} mA"))
    # P-DD43
    a = phase0_out(p0, ["--step_us_host", "1077", "--coremark_host", "25134", "--radio_j_per_h", "89.4"])
    b = phase0_out(p0, ["--step_us_host", "1019", "--coremark_host", "25134", "--radio_j_per_h", "28.2"])
    want_a = ["127.7 ms", "= 5.1% of", "1.82 J/h", "= 2.0% of", "k_dp <= 3.9"]
    ok = all(w in a for w in want_a) and "k_dp <= 1.6" in b
    out.append(("P-DD43", ok, "DD 43's numbers reproduced" if ok else f"got: {a.strip()} | {b.strip()}"))
    # D-SURV
    t2 = row("DEBRUY-T2")
    fr = [tuple(map(int, x)) for x in re.findall(r"\((\d+)/(\d+)\)", t2)]
    want = [n / m for n, m in fr[:3]] + [0.0]
    got = [dig.survival(h) for h in (0.0, 23.9, 24.0, 47.9, 48.0, 95.9, 96.0, 500.0)]
    ok = len(fr) >= 4 and fr[3][0] == 0 and got == [want[0], want[0], want[1], want[1], want[2], want[2], 0.0, 0.0]
    out.append(("D-SURV", ok, f"row fractions {fr}; survival at 0/23.9/24/47.9/48/95.9/96/500 h {[round(g, 4) for g in got]}"))
    # D-TIME
    ok = (dig.dig_time(0.0, 2.0, 2.0) == 2.0 and dig.dig_time(2.0, 2.0, 2.0) == 2.0 and abs(dig.dig_time(4.0, 2.0, 2.0) - 8.0) < 1e-12
          and abs(dig.dig_time(3.0, 1.0, 1.0) - 9.0) < 1e-12)
    out.append(("D-TIME", ok, f"e=0,2,4 at r=2 t=2: {dig.dig_time(0, 2, 2)}, {dig.dig_time(2, 2, 2)}, {dig.dig_time(4, 2, 2)}"))
    # D-LEAK
    W = {k: dict(est=(10.0 * k, 5.0 - k), true=(10.0 * k + (k % 3), 5.0 - k), sigma=0.5 + 0.37 * ((7 * k) % 5), t=1800.0)
         for k in range(1, 9)}
    W2 = {k: dict(w, true=(w["true"][0] + 40.0 * ((k * 5) % 7), w["true"][1] - 9.0 * k)) for k, w in W.items()}
    p1, p2 = dig.policies(W, 2.0, 2.0, (0.0, 0.0)), dig.policies(W2, 2.0, 2.0, (0.0, 0.0))
    same = [n for n in ("id", "nearest", "confident", "spt") if p1[n] == p2[n]]
    ok = len(same) == 4 and p1["oracle"] != p2["oracle"]
    out.append(("D-LEAK", ok, f"system policies unchanged by the truth {same} (need 4); oracle changed {p1['oracle'] != p2['oracle']}"))
    # D-RD
    ts = [60.0 * i for i in range(0, 31)]
    const = {k: [(t, (0, 0), (0, 0), 1.0) for t in ts] for k in (1, 2, 3)}
    fall = {k: [(t, (0, 0), (0, 0), 50.0 * math.exp(-t / 900.0) + 0.5) for t in ts] for k in (1, 2, 3)}
    rc, rf = dig.range_or_dig(const, 2, 2.0, 2.0), dig.range_or_dig(fall, 2, 2.0, 2.0)
    ok = rc == 300.0 and rf > 300.0 and min(rc, rf) >= 300.0
    out.append(("D-RD", ok, f"constant sigmas start at {rc:.0f} s (need 300), falling sigmas at {rf:.0f} s (need > 300)"))
    return out


def sim_check(binary):
    with tempfile.TemporaryDirectory() as t:
        base = ["--specProfile=true", "--duration=300", "--seed=7", "--arms=ca_tbl_full"]
        lib = os.path.join(os.path.dirname(binary), "lib")
        lib = lib if os.path.isdir(lib) else os.path.join(os.path.dirname(os.path.dirname(binary)), "lib")
        env = dict(os.environ, LD_LIBRARY_PATH=lib)
        r = {}
        for tag, extra in (("off", []), ("on", ["--phase0Timing=true"])):
            o = os.path.join(t, tag)
            os.makedirs(o)
            p = subprocess.run([binary] + base + extra + [f"--outdir={o}"], capture_output=True, text=True, env=env)
            r[tag] = (p.returncode, p.stdout, o)
        fs = sorted(os.path.basename(f) for f in glob.glob(os.path.join(r["off"][2], "*.csv")))
        diff = [f for f in fs if not filecmp.cmp(os.path.join(r["off"][2], f), os.path.join(r["on"][2], f), shallow=False)]
        n0 = r["off"][1].count("PHASE0 "); n1 = r["on"][1].count("PHASE0 ")
        ok = r["off"][0] == 0 and r["on"][0] == 0 and fs and not diff and n0 == 0 and n1 == 1
        return ("P-RUN", ok, f"exit {r['off'][0]}/{r['on'][0]}; {len(fs)} CSVs, differing {len(diff)}; PHASE0 lines off {n0} on {n1} (need 0, 1)")


def report(res):
    bad = 0
    for name, ok, msg in res:
        print(f"  G191 {'ok  ' if ok else 'FAIL'} {name:7} {msg}")
        bad += not ok
    return bad


def main():
    argv = sys.argv[1:]
    p0, p0src = load(P0, "phase0")
    dig, _ = load(DIG, "score_dig")
    res = checks(p0, p0src, dig)
    if "--sim" in argv:
        res.append(sim_check(argv[argv.index("--sim") + 1]))
    bad = report(res)
    if "--fixtures" in argv:
        fx = [
            ("policy reads truth", "D-LEAK", None, ('"confident": sorted(W, key=lambda k: W[k]["sigma"])',
                                                    '"confident": sorted(W, key=lambda k: math.dist(W[k]["est"], W[k]["true"]))')),
            ("survival step changed", "D-SURV", None, ("(2.0, 20 / 57)", "(2.0, 20 / 50)")),
            ("dig-time formula changed", "D-TIME", None, ("(err * err - r * r) / (r * r)", "(err - r) / r")),
            ("CoreMark default changed", "P-ROWS", ('"--coremark_nrf", type=float, default=212.0', '"--coremark_nrf", type=float, default=250.0'), None),
            ("current default changed", "P-DD43", ('default=3.3e-3', 'default=3.0e-3'), None),
        ]
        print("  G191 fixtures")
        for name, target, pp, dp in fx:
            p0m, p0s = load(P0, "phase0_fx", pp)
            dm, _ = load(DIG, "dig_fx", dp)
            r = {n: ok for n, ok, _ in checks(p0m, p0s, dm)}
            good = r[target] is False
            print(f"    {'OK  ' if good else 'BAD '} {name}: {target} {'fails' if good else 'PASSED (should fail)'}")
            bad += not good
    print(f"G191 {'PASSED' if not bad else 'FAILED'}")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
