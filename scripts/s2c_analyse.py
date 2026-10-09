#!/usr/bin/env python3
"""S2-C analysis (report/PREREG_S2C.md): unknown start, and the subjects' measurement weighting.

usage: s2c_analyse.py --select <outdir>                        print s* (the registered rule)
       s2c_analyse.py <outdir> <ACC-A outdir> <ACC-A-OPPOINT outdir>

Writes <outdir>/S2C.md and <outdir>/s2c_table.csv, prints the same, and exits 1 if any registered
abort condition (PREREG sect 5) holds.
"""
import csv
import filecmp
import glob
import math
import os
import re
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import camp_spec_analyse as cs  # noqa: E402
import numpy as np  # noqa: E402

DUR = 1800.0
WARM = 120.0
NIS99 = 9.21
EXTRA = 5
ARMS3 = ("ca_tbl_full", "fixed_threshold", "static_2p4")
SCALES = (1, 3, 10)


def rows(p):
    try:
        with open(p) as fh:
            return list(csv.DictReader(fh))
    except OSError:
        return None


def nodes(d, arm):
    s = int(d.rsplit("s", 1)[1])
    out = [rows(os.path.join(d, f"trace_{arm}_seed{s}.csv"))]
    out += [rows(os.path.join(d, f"subjtrace_{arm}_seed{s}_subj{k}.csv")) for k in range(2, 6)]
    return out if all(out) else None


def subj_consistency(d, arm):
    """(NIS > 9.21 count, NIS rows, NEES list) over subjects' fix rows, t >= WARM."""
    ns = nis = 0
    nees = []
    s = int(d.rsplit("s", 1)[1])
    for k in range(2, 6):
        for r in rows(os.path.join(d, f"subjtrace_{arm}_seed{s}_subj{k}.csv")) or []:
            if r["hadFix"] != "1" or float(r["t"]) < WARM or "pxx" not in r:
                continue
            m = float(r["mahalanobis"])
            if m > 0.0:
                ns += 1
                nis += m > NIS99
            pxx, pxy, pyy = float(r["pxx"]), float(r["pxy"]), float(r["pyy"])
            det = pxx * pyy - pxy * pxy
            if det > 0:
                ex = float(r["estX"]) - float(r["trueX"])
                ey = float(r["estY"]) - float(r["trueY"])
                nees.append((pyy * ex * ex - 2 * pxy * ex * ey + pxx * ey * ey) / det)
    return nis, ns, nees


def select(out):
    share = {}
    for k in SCALES:
        a = b = 0
        for d in cs.seeds(out, f"T_A_s{k}"):
            if cs.ok(d):
                x, n, _ = subj_consistency(d, "ca_tbl_full")
                a, b = a + x, b + n
        share[k] = a / b if b else float("nan")
    ok = [k for k in SCALES if share[k] <= 0.05]
    return (min(ok) if ok else min(SCALES, key=lambda k: share[k])), share


def seed_metrics(d, arm):
    ns = nodes(d, arm)
    if ns is None:
        return None
    first, never, w25, w25acq, med, rec, trunc = [], 0, [], [], [], [], 0
    for tr in ns:
        t = np.array([float(r["t"]) for r in tr])
        e = np.array([float(r["errM"]) for r in tr])
        fx = np.array([r["hadFix"] == "1" for r in tr])
        i0 = int(np.argmax(fx)) if fx.any() else None
        first.append(t[i0] if i0 is not None else DUR)
        never += i0 is None
        w25.append(float(np.mean(e <= 25.0)))
        if i0 is not None:
            w25acq.append(float(np.mean(e[i0:] <= 25.0)))
        med.append(float(np.median(e)))
        inside = e <= 25.0
        i = 1
        while i < len(e):
            if inside[i - 1] and not inside[i]:
                j = i + 1
                while j < len(e) and not inside[j]:
                    j += 1
                if j < len(e):
                    rec.append(t[j] - t[i])
                else:
                    trunc += 1
                i = j
            i += 1
    ss = cs.subj_rows(d, arm)
    e0 = np.array([float(r["errM"]) for r in ns[0]])
    nis, nn, nees = subj_consistency(d, arm)
    return dict(
        first=float(np.mean(first)), never=never / len(ns), w25=float(np.mean(w25)),
        w25acq=float(np.mean(w25acq)) if w25acq else float("nan"), med=float(np.mean(med)),
        c95=float(np.quantile(e0, 0.95)), rec=float(np.median(rec)) if rec else float("nan"),
        trunc=trunc, av=float(np.mean([float(r["fixCycles"]) / float(r["steps"]) for r in ss])),
        J=float(np.mean([float(r["totalJ"]) for r in ss])),
        nis=(nis / nn) if nn else float("nan"), anees=float(np.mean(nees)) if nees else float("nan"))


def collect(out, cell, arm):
    res = {}
    for d in cs.seeds(out, cell):
        if cs.ok(d):
            m = seed_metrics(d, arm)
            if m is not None:
                res[int(d.rsplit("s", 1)[1])] = m
    return res


def paired(a, b, k):
    s = sorted(set(a) & set(b))
    return cs.ci([a[i][k] - b[i][k] for i in s]), len(s)


def inc0(iv):
    return not math.isnan(iv[1]) and iv[1] <= 0.0 <= iv[2]


def arm_lines(path, arm):
    with open(path) as fh:
        return [ln for ln in fh.read().splitlines() if f",{arm}," in f",{ln},"]


def strip_extra(path):
    with open(path) as fh:
        return [",".join(ln.split(",")[:-EXTRA]) for ln in fh.read().splitlines()]


def reuse_check(out, acc, opp, problems):
    good = total = 0
    src = {"ca_tbl_full": (acc, "M_{}_k10"), "static_2p4": (acc, "M_{}_k10"),
           "fixed_threshold": (opp, "O_{}_T12")}
    for cfg in ("A", "B"):
        for s in (1, 2, 3):
            c = os.path.join(out, f"C_{cfg}", f"s{s}")
            for arm, (root, pat) in src.items():
                total += 1
                r = os.path.join(root, pat.format(cfg), f"s{s}")
                files = sorted(os.path.basename(p) for p in glob.glob(os.path.join(c, f"*_{arm}_seed{s}*")))
                ok = bool(files)
                for f in files:
                    a, b = os.path.join(c, f), os.path.join(r, f)
                    if not os.path.exists(b):
                        ok = False
                    elif f.startswith("subjtrace_"):
                        ok = ok and strip_extra(a) == open(b).read().splitlines()
                    else:
                        ok = ok and filecmp.cmp(a, b, shallow=False)
                for summ in ("arm_summary.csv", "subject_summary.csv"):
                    ok = ok and arm_lines(os.path.join(c, summ), arm) == arm_lines(os.path.join(r, summ), arm) != []
                good += ok
    if good != total:
        problems.append(f"P6 leak: stage C byte-identical on {good} of {total} seed-arm pairs")
    return good, total


def main():
    if len(sys.argv) == 3 and sys.argv[1] == "--select":
        k, share = select(sys.argv[2])
        sys.stderr.write("NIS>9.21 share by s: " + ", ".join(f"{a}: {b:.4f}" for a, b in share.items()) + "\n")
        print(k)
        return 0
    if len(sys.argv) != 4:
        print(__doc__)
        return 2
    out, acc, opp = sys.argv[1:]
    problems = []
    rcs = glob.glob(os.path.join(out, "*", "s*", "rc"))
    stamps = set()
    for p in rcs:
        d = os.path.dirname(p)
        if not cs.ok(d):
            problems.append(f"non-zero exit: {d}")
        rid = open(os.path.join(d, "run_id.txt")).read() if os.path.exists(os.path.join(d, "run_id.txt")) else ""
        if " dirty 0 " not in rid:
            problems.append(f"run_id not dirty 0: {d}")
        m = re.search(r"^build ([^|]+)\|", rid, re.M)
        stamps.add(m.group(1).strip() if m else None)
    if len(stamps) != 1 or None in stamps:
        problems.append(f"build stamps: {sorted(map(str, stamps))}")
    sstar = int(open(os.path.join(out, "SSTAR")).read().strip())
    expect = {f"T_A_s{k}": 10 for k in SCALES}
    expect.update({"C_A": 3, "C_B": 3, "U_A": 20, "U_B": 20})
    if sstar != 1:
        expect.update({"H_A": 20, "H_B": 20})
    for cell, n in expect.items():
        got = len([d for d in cs.seeds(out, cell) if os.path.exists(os.path.join(d, "rc"))])
        if got != n:
            problems.append(f"{cell}: {got} runs, not {n}")
    good, total = reuse_check(out, acc, opp, problems)
    reuse = good == total
    _, share = select(out)

    md = ["# S2-C", "", f"s* = {sstar}. Training NIS > 9.21 share (subjects, ca_tbl_full, seeds 21-30): "
          + ", ".join(f"s = {k}: {v:.4f}" for k, v in share.items()) + ".",
          f"Stage C reuse check: {good} of {total} seed-arm pairs byte-identical.", ""]
    table = []
    keys = ("first", "never", "av", "w25", "w25acq", "med", "c95", "rec", "J", "nis", "anees")
    U = {(cfg, arm): collect(out, f"U_{cfg}", arm) for cfg in ("A", "B") for arm in ARMS3}
    Ctl = {}
    for cfg in ("A", "B"):
        Ctl[(cfg, "ca_tbl_full")] = collect(acc, f"M_{cfg}_k10", "ca_tbl_full")
        Ctl[(cfg, "static_2p4")] = collect(acc, f"M_{cfg}_k10", "static_2p4")
        Ctl[(cfg, "fixed_threshold")] = collect(opp, f"O_{cfg}_T12", "fixed_threshold")
    for cfg in ("A", "B"):
        md += [f"## U_{cfg}: unknown start, means per arm (seeds 1-20)", "",
               "| metric | " + " | ".join(ARMS3) + " | known start (control) " + " / ".join(ARMS3) + " |",
               "|---|---|---|---|---|"]
        for k in keys:
            vals = [np.nanmean([x[k] for x in U[(cfg, a)].values()]) if U[(cfg, a)] else float("nan") for a in ARMS3]
            cv = [np.nanmean([x[k] for x in Ctl[(cfg, a)].values()]) if Ctl[(cfg, a)] else float("nan") for a in ARMS3]
            md.append(f"| {k} | " + " | ".join(f"{v:.3f}" for v in vals) + " | " + " / ".join(f"{v:.3f}" for v in cv) + " |")
        md += ["", "| paired difference | value |", "|---|---|"]
        for (lab, a, b) in (("ca_tbl_full − static_2p4 (U)", U[(cfg, "ca_tbl_full")], U[(cfg, "static_2p4")]),
                            ("ca_tbl_full − fixed_threshold@12 (U)", U[(cfg, "ca_tbl_full")], U[(cfg, "fixed_threshold")]),
                            ("ca_tbl_full U − known start", U[(cfg, "ca_tbl_full")], Ctl[(cfg, "ca_tbl_full")]),
                            ("static_2p4 U − known start", U[(cfg, "static_2p4")], Ctl[(cfg, "static_2p4")])):
            for k in ("first", "never", "av", "w25", "w25acq", "med", "c95", "rec", "J"):
                iv, n = paired(a, b, k)
                md.append(f"| {lab}: {k} (n {n}) | {cs.fmt(*iv)} |")
                table.append(dict(stage=f"U_{cfg}", contrast=lab, metric=k, n=n, diff=iv[0], lo=iv[1], hi=iv[2]))
        md.append("")
    H = {(cfg, arm): collect(out, f"H_{cfg}", arm) for cfg in ("A", "B") for arm in ("ca_tbl_full", "static_2p4")}
    if sstar != 1:
        for cfg in ("A", "B"):
            md += [f"## H_{cfg}: held-out, s* = {sstar} − s = 1 (control: ACC-A M_{cfg}_k10)", "",
                   "| contrast | value |", "|---|---|"]
            for arm in ("ca_tbl_full", "static_2p4"):
                for k in ("av", "w25", "w25acq", "med", "c95", "rec", "J", "first"):
                    iv, n = paired(H[(cfg, arm)], Ctl[(cfg, arm)], k)
                    md.append(f"| {arm}: {k} (n {n}) | {cs.fmt(*iv)} |")
                    table.append(dict(stage=f"H_{cfg}", contrast=f"{arm} s*-s1", metric=k, n=n, diff=iv[0], lo=iv[1], hi=iv[2]))
            hv = [np.nanmean([x[k] for x in H[(cfg, 'ca_tbl_full')].values()]) for k in ("nis", "anees")]
            md += ["", f"ca_tbl_full subjects at s*: NIS > 9.21 share {hv[0]:.4f}, ANEES {hv[1]:.2f}", ""]
    Tm = {k: collect(out, f"T_A_s{k}", "ca_tbl_full") for k in SCALES}
    md += ["## T: training (seeds 21-30, A, ca_tbl_full)", "", "| s | NIS > 9.21 share | ANEES | w25 | med | J |", "|---|---|---|---|---|---|"]
    for k in SCALES:
        v = Tm[k]
        md.append(f"| {k} | {share[k]:.4f} | {np.nanmean([x['anees'] for x in v.values()]):.2f} | "
                  f"{np.nanmean([x['w25'] for x in v.values()]):.3f} | {np.nanmean([x['med'] for x in v.values()]):.1f} | "
                  f"{np.nanmean([x['J'] for x in v.values()]):.2f} |")

    def iv(a, b, k):
        return paired(a, b, k)[0]

    ca, st, fx = (lambda c: U[(c, "ca_tbl_full")]), (lambda c: U[(c, "static_2p4")]), (lambda c: U[(c, "fixed_threshold")])
    never = {a: np.mean([x["never"] for x in U[("A", a)].values()]) for a in ARMS3}
    p4 = all(abs(iv(ca(c), Ctl[(c, "ca_tbl_full")], "w25acq")[1]) <= 0.05 and abs(iv(ca(c), Ctl[(c, "ca_tbl_full")], "w25acq")[2]) <= 0.05 for c in ("A", "B")) if reuse else None
    ann = {k: abs(np.nanmean([x["anees"] for x in Tm[k].values()]) - 2.0) for k in SCALES}
    p9 = (iv(H[("A", "ca_tbl_full")], Ctl[("A", "ca_tbl_full")], "w25")[1] > 0) if (sstar != 1 and reuse) else None
    if sstar != 1 and reuse:
        base_c95 = np.mean([x["c95"] for x in Ctl[("B", "ca_tbl_full")].values()])
        p10 = iv(H[("B", "ca_tbl_full")], Ctl[("B", "ca_tbl_full")], "c95")[0] <= 0.10 * base_c95
    else:
        p10 = None
    verdicts = [
        ("P1", "U_A w25 ca − static above 0", iv(ca("A"), st("A"), "w25")[1] > 0),
        ("P2", "U_A never-fix: static ≥ 0.40 and ca ≤ 0.15", never["static_2p4"] >= 0.40 and never["ca_tbl_full"] <= 0.15),
        ("P3", "U_A and U_B first fix ca − static below 0", all(iv(ca(c), st(c), "first")[2] < 0 for c in ("A", "B"))),
        ("P4", "U − C w25acq within ±0.05 (ca_tbl_full)", p4),
        ("P5", "U w25 and av, ca − fixed@12 contain 0", all(inc0(iv(ca(c), fx(c), k)) for c in ("A", "B") for k in ("w25", "av"))),
        ("P6", "stage C byte-identity 18/18", reuse),
        ("P7", "T s=1 NIS > 9.21 share > 0.05", share[1] > 0.05),
        ("P8", "T share strictly decreasing in s", share[1] > share[3] > share[10]),
        ("P9", "H_A ca w25 s* − s1 above 0", p9),
        ("P10", "H_B ca CEP95 s* − s1 ≤ +10 %", p10),
        ("P11", "T |ANEES − 2| smaller at s* than s = 1", (ann[sstar] < ann[1]) if sstar != 1 else None),
    ]
    md += ["", "## Registered predictions", "", "| # | prediction | verdict |", "|---|---|---|"]
    for n, txt, v in verdicts:
        md.append(f"| {n} | {txt} | {'held' if v is True else 'failed' if v is False else 'not evaluable'} |")
        if v is None and not (n in ("P9", "P10", "P11") and sstar == 1):
            problems.append(f"{n} not evaluable")
    md += ["", f"Problems: {len(problems)}"] + [f"- {p}" for p in problems]
    text = "\n".join(md) + "\n"
    print(text)
    open(os.path.join(out, "S2C.md"), "w").write(text)
    with open(os.path.join(out, "s2c_table.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(table[0]) if table else ["stage"])
        w.writeheader()
        w.writerows(table)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
