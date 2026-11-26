#!/usr/bin/env python3
"""REALISTIC-12 (report/PREREG_REALISTIC12.md): quiet shift, particle count 2000 (N2000) and 20000 (N20000), seeds 1901-1910.
Crew = the five workers' devices (trace + subjtrace). Per run: share of crew cycles within 25 m (A25) and coverage (errM <= 1.73 sqrt(traceSigma)).
Paired 95 % bootstrap over seeds (camp_spec_analyse.ci).
  S1  RAND3: ours A25, N20000 - N2000, lower end > 0
  S2  every site, N20000: ours mean coverage >= 0.85
  S3  every site: ours A25, N20000 - N2000, lower end >= -0.02
usage: realistic12_analyse.py <outdir>"""
import csv, glob, os, re, sys
import numpy as np
sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402

SITES, SEEDS, NS = ("RAND3", "CLUS3", "ROAD3"), range(1901, 1911), (2000, 20000)


def crew(d, arm):
    e = []
    for f in glob.glob(os.path.join(d, f"trace_{arm}_seed*.csv")) + glob.glob(os.path.join(d, f"subjtrace_{arm}_seed*_subj*.csv")):
        e += [(float(r["errM"]), float(r["traceSigma"])) for r in csv.DictReader(open(f))]
    a = np.array(e)
    return (float(np.mean(a[:, 0] <= 25.0)), float(np.mean(a[:, 0] <= 1.73 * np.sqrt(np.maximum(a[:, 1], 1e-9))))) if len(a) else None


def main():
    out = sys.argv[1]
    problems, X, src = [], {}, set()
    for st in SITES:
        for n in NS:
            for s in SEEDS:
                d = os.path.join(out, f"Q_{st}_N{n}", f"s{s}")
                if not os.path.exists(d + "/rc") or open(d + "/rc").read().strip() != "0":
                    problems.append(f"{d}: missing or rc not 0"); continue
                m = re.search(r"^source (\S+) dirty (\d+)", open(d + "/run_id.txt").read(), re.M)
                if not m or m.group(2) != "0":
                    problems.append(f"{d}: not dirty 0")
                else:
                    src.add(m.group(1))
                if (f"--acquireParticleCount={n}" in open(d + "/run_id.txt").read()) is False:
                    problems.append(f"{d}: command line lacks --acquireParticleCount={n}")
                if ("DD70 acquireParticleCount" in open(d + "/run.log", errors="replace").read()) != (n != 2000):
                    problems.append(f"{d}: DD70 line does not match {n}")
                for arm in ("ca_tbl_full", "static_2p4"):
                    v = crew(d, arm)
                    if v is None:
                        problems.append(f"{d}: no crew traces for {arm}")
                    else:
                        X[(st, n, arm, s)] = v
    if len(src) > 1:
        problems.append(f"more than one source {sorted(src)}")
    md = ["# REALISTIC-12 (scripts/realistic12_analyse.py)", "", f"Sources: {', '.join(sorted(src))}.", "",
          "| site | ours A25 N2000 | N20000 | N20000 - N2000 | ours coverage N2000 | N20000 | 2.4 GHz only A25 N2000 | N20000 |", "|---|---|---|---|---|---|---|---|"]
    s1 = False; s2 = s3 = True
    for st in SITES:
        k = [s for s in SEEDS if all((st, n, a, s) in X for n in NS for a in ("ca_tbl_full", "static_2p4"))]
        if not k:
            md.append(f"| {st} | no data |"); s2 = s3 = False; continue
        m = lambda n, a, i: float(np.mean([X[(st, n, a, s)][i] for s in k]))
        d = cs.ci([X[(st, 20000, "ca_tbl_full", s)][0] - X[(st, 2000, "ca_tbl_full", s)][0] for s in k])
        if st == "RAND3":
            s1 = d[1] > 0.0
        s2 = s2 and m(20000, "ca_tbl_full", 1) >= 0.85
        s3 = s3 and d[1] >= -0.02
        md.append(f"| {st} | {m(2000,'ca_tbl_full',0):.3f} | {m(20000,'ca_tbl_full',0):.3f} | {cs.fmt(*d)} | {m(2000,'ca_tbl_full',1):.3f} | {m(20000,'ca_tbl_full',1):.3f} | {m(2000,'static_2p4',0):.3f} | {m(20000,'static_2p4',0):.3f} |")
    md += ["", f"**S1: {'HOLDS' if s1 else 'FAILS'}**  **S2: {'HOLDS' if s2 else 'FAILS'}**  **S3: {'HOLDS' if s3 else 'FAILS'}**", "", f"Problems: {len(problems)}"] + [f"- {p}" for p in problems[:40]]
    open(os.path.join(out, "REALISTIC12.md"), "w").write("\n".join(md) + "\n")
    print("\n".join(md))


if __name__ == "__main__":
    main()
