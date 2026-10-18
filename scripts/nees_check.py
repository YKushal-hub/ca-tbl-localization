"""B1 / flaw S3: is the filter covariance consistent with the error? (MASTER_PLAN_V2 §2b S3)

Pass band (registered in the master plan before any read): mean NEES over rows within [1.6, 2.5]
(2-D), and empirical R95 coverage 0.95 +/- 0.03, where R95 = sqrt(5.991 * lambda_max(P)).
Reads trace_<arm>_seed<s>.csv (tag 0) and subjtrace_<arm>_seed<s>_subj<k>.csv (subjects 2-5) with
pxx,pxy,pyy. Rows: after the node's first fix, t >= 120 s. Reported for all such rows and for fix rows.
usage: nees_check.py <campaign outdir> <cell> [<cell> ...]
"""
import csv, glob, math, os, sys
import numpy as np

ARMS = ("static_868", "static_2p4", "lora_fusion", "ca_tbl_full", "fixed_threshold")


def node_files(d, arm):
    s = int(d.rsplit("s", 1)[1])
    fs = [os.path.join(d, f"trace_{arm}_seed{s}.csv")]
    fs += sorted(glob.glob(os.path.join(d, f"subjtrace_{arm}_seed{s}_subj*.csv")))
    return [f for f in fs if os.path.exists(f)]


def rows_stats(path):
    nees, cov, fixmask = [], [], []
    with open(path) as fh:
        rd = csv.DictReader(fh)
        if "pxx" not in rd.fieldnames:
            return None
        seen_fix = False
        for r in rd:
            if r.get("hadFix") == "1":
                seen_fix = True
            if not seen_fix or float(r["t"]) < 120.0:
                continue
            try:
                pxx, pxy, pyy = float(r["pxx"]), float(r["pxy"]), float(r["pyy"])
                ex, ey = float(r["estX"]) - float(r["trueX"]), float(r["estY"]) - float(r["trueY"])
            except (KeyError, ValueError):
                continue
            det = pxx * pyy - pxy * pxy
            if not (det > 0 and math.isfinite(det)):
                continue
            n = (pyy * ex * ex - 2 * pxy * ex * ey + pxx * ey * ey) / det
            lmax = 0.5 * (pxx + pyy) + math.sqrt(0.25 * (pxx - pyy) ** 2 + pxy * pxy)
            nees.append(n)
            cov.append(math.hypot(ex, ey) <= math.sqrt(5.991 * lmax))
            fixmask.append(r.get("hadFix") == "1")
    return np.array(nees), np.array(cov), np.array(fixmask)


def main():
    out = sys.argv[1]
    lines = ["| cell | arm | rows | mean NEES | median NEES | R95 coverage | fix rows: mean NEES | fix rows: coverage | verdict |",
             "|---|---|---|---|---|---|---|---|---|"]
    for cell in sys.argv[2:]:
        for arm in ARMS:
            N, C, F = [], [], []
            for d in sorted(glob.glob(os.path.join(out, cell, "s*"))):
                for f in node_files(d, arm):
                    st = rows_stats(f)
                    if st is None:
                        continue
                    N.append(st[0]); C.append(st[1]); F.append(st[2])
            if not N:
                continue
            N, C, F = np.concatenate(N), np.concatenate(C).astype(bool), np.concatenate(F).astype(bool)
            ok = 1.6 <= N.mean() <= 2.5 and abs(C.mean() - 0.95) <= 0.03
            lines.append(f"| {cell} | {arm} | {len(N)} | {N.mean():.2f} | {np.median(N):.2f} | {C.mean():.3f} | "
                         f"{N[F].mean() if F.any() else float('nan'):.2f} | {C[F].mean() if F.any() else float('nan'):.3f} | "
                         f"{'PASS' if ok else 'FAIL'} |")
    print("\n".join(lines))


if __name__ == "__main__":
    main()
