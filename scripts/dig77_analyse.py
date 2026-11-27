#!/usr/bin/env python3
"""DIG-77 (report/PREREG_DIG77.md): dig order ranked by the filter's covariance (sptf) vs the published DD 75 bound (spt), seeds 2201-2210.
usage: dig77_analyse.py <outdir>"""
import csv, glob, os, re, sys
import numpy as np
ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
sys.path.insert(0, os.path.join(ROOT, "scripts", "collapse"))
import score_dig as sd  # noqa: E402
CELLS, F, SEEDS = ("K5", "K8", "H5", "H8", "K2"), ("K5", "K8", "H5", "H8"), range(2201, 2211)
GRID = [(t, d, r) for t in (1, 2) for d in (0.5, 2.0, 8.0) for r in (1.0, 2.0)]


def coverage(d):
    c = []
    for f in glob.glob(os.path.join(d, "trace_ca_tbl_full_seed*.csv")) + glob.glob(os.path.join(d, "subjtrace_ca_tbl_full_seed*_subj*.csv")):
        a = np.array([[float(r["t"]), float(r["errM"]), float(r["traceSigma"])] for r in csv.DictReader(open(f))])
        h = a[a[:, 0] > a[:, 0].max() * 0.5]
        c.append(np.mean(h[:, 1] <= 1.73 * np.sqrt(np.maximum(h[:, 2], 0.0))))
    return float(np.mean(c)), len(c)


def main():
    out = sys.argv[1]; problems = []; A = {}; COV = {}; src = set()
    for c in CELLS:
        for s in SEEDS:
            d = os.path.join(out, c, f"seed{s}")
            if not os.path.exists(d + "/rc") or open(d + "/rc").read().strip() != "0":
                problems.append(f"{d}: rc"); continue
            m = re.search(r"^source (\S+) dirty (\d+)", open(d + "/run_id.txt").read(), re.M)
            if not m or m.group(2) != "0":
                problems.append(f"{d}: dirty")
            else:
                src.add(m.group(1))
            if "--sigmaShadow" in open(d + "/run_id.txt").read():
                problems.append(f"{d}: --sigmaShadow given explicitly (the preset must set it)")
            if not re.search(r"^DD75 sigmaShadow cycles=", open(d + "/run.log").read(), re.M):
                problems.append(f"{d}: no DD75 line (the preset did not turn DD 75 on)")
            W = sd.load(d, "ca_tbl_full", "END")
            if len(W) != 20 or any(w.get("sigmaf") is None for w in W.values()):
                problems.append(f"{d}: {len(W)} workers or a trace without pxx/pyy"); continue
            COV[(c, s)] = coverage(d)[0]
            start = max(w["t"] for w in W.values()) / 3600.0
            for k, (t, dg, r) in enumerate(GRID):
                P = sd.policies(W, dg, r, (0.0, 0.0))
                for pol in ("id", "spt", "sptf", "oracle"):
                    A[(c, s, k, pol)] = sd.schedule(P[pol], W, t, dg, r, start)[0]
    if problems:
        print("INCOMPLETE:", problems[:10]); return 1
    m = lambda c, k, p: float(np.mean([A[(c, s, k, p)] for s in SEEDS]))
    print(f"# DIG-77 (scripts/dig77_analyse.py)\n\nSources: {', '.join(sorted(src))}.\n")
    print("| cell | teams | t_dig h | r m | id | spt (DD 75 bound) | sptf (filter) | oracle | sptf - spt | sptf - id |")
    print("|---|---|---|---|---|---|---|---|---|---|")
    e1, e2 = [], []
    for c in CELLS:
        for k, (t, dg, r) in enumerate(GRID):
            i, sp, sf, o = m(c, k, "id"), m(c, k, "spt"), m(c, k, "sptf"), m(c, k, "oracle")
            e1.append(sf - sp); e2.append(sf - i)
            print(f"| {c} | {t} | {dg} | {r} | {i:.2f} | {sp:.2f} | {sf:.2f} | {o:.2f} | {sf - sp:+.2f} | {sf - i:+.2f} |")
    k0 = GRID.index((2, 2.0, 2.0))
    h5 = {c: m(c, k0, "sptf") - m(c, k0, "id") for c in CELLS}
    E1 = float(np.mean(e1)) >= 0.0 and min(e1) >= -0.10
    share = float(np.mean([v >= 0 for v in e2]))
    E2 = share >= 0.80 and all(v >= 0 for v in h5.values())
    cov = {c: float(np.mean([COV[(c, s)] for s in SEEDS])) for c in CELLS}
    E3 = all(cov[c] >= 0.85 for c in F)
    print(f"\nE1 sptf - spt: mean {np.mean(e1):+.3f}, worst {min(e1):+.2f}, best {max(e1):+.2f}")
    print(f"E2 sptf >= id in {sum(v >= 0 for v in e2)} of {len(e2)} ({share:.2f}); at teams 2, 2 h, 2 m: " + ", ".join(f"{c} {v:+.2f}" for c, v in h5.items()))
    print("E3 coverage of the published bound: " + ", ".join(f"{c} {v:.3f}" for c, v in cov.items()))
    print(f"\n**E1: {'HOLDS' if E1 else 'FAILS'}**  **E2: {'HOLDS' if E2 else 'FAILS'}**  **E3: {'HOLDS' if E3 else 'FAILS'}**\n\nProblems: 0")
    return 0


if __name__ == "__main__":
    sys.exit(main())
