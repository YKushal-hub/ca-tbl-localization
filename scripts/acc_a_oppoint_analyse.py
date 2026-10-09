#!/usr/bin/env python3
"""ACC-A-OPPOINT analysis (report/PREREG_ACC_A_OPPOINT.md): fixed_threshold at --fixedVarThreshold T
against ACC-A's ca_tbl_full on the same binary, paired over seeds.

usage: acc_a_oppoint_analyse.py <outdir of acc_a_oppoint_run.sh> <outdir of acc_a_run.sh>

Writes <outdir>/OPPOINT.md and <outdir>/oppoint_table.csv, prints the same, and exits 1 if any
registered abort condition (PREREG sect 5) holds.
"""
import csv
import filecmp
import glob
import math
import os
import sys

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
import acc_a_analyse as aa  # noqa: E402
import camp_spec_analyse as cs  # noqa: E402

SOURCE = "source 681e90c8 dirty 0"
NRUNS = 100
B_T = (8, 12, 25, 100)
KEYS = (("av", "availability", False), ("J", "J per node", True), ("c50", "tag 0 CEP50", True),
        ("c95", "tag 0 CEP95", True), ("w25", "within 25 m", False), ("esc", "escalations", True),
        ("cuwb", "UWB cycles", True), ("uwbfix", "UWB-solved fixes", False))


def cell_name(cfg, t):
    return f"C_B_T{t}" if t == 100 else f"O_{cfg}_T{t}"


def rule_share(d, t):
    """Share of fixed_threshold's escalation decisions on a decision row with trace > t."""
    s = int(d.rsplit("s", 1)[1])
    rows = aa.rows(os.path.join(d, f"trace_fixed_threshold_seed{s}.csv")) or []
    esc = [r for r in rows if r["escalated"] == "1"]
    rule = [r for r in esc if r["hadFix"] == "1" and int(r["nAnchors"]) >= 3
            and r["activeTier"] in ("868MHz", "2.4GHz") and float(r["traceSigma"]) > t]
    return len(rule), len(esc)


def arm_lines(path, arm):
    with open(path) as fh:
        return [ln for ln in fh.read().splitlines() if f",{arm}," in f",{ln},"]


def control(out, acc, problems):
    same = 0
    for s in range(1, 21):
        a, b = os.path.join(out, "C_B_T100", f"s{s}"), os.path.join(acc, "M_B_k10", f"s{s}")
        files = sorted(os.path.basename(p) for p in glob.glob(os.path.join(a, f"*_fixed_threshold_seed{s}*")))
        ok = bool(files) and all(os.path.exists(os.path.join(b, f)) and
                                 filecmp.cmp(os.path.join(a, f), os.path.join(b, f), shallow=False)
                                 for f in files)
        for summ in ("arm_summary.csv", "subject_summary.csv"):
            ok = ok and arm_lines(os.path.join(a, summ), "fixed_threshold") == \
                arm_lines(os.path.join(b, summ), "fixed_threshold") != []
        same += ok
    if same != 20:
        problems.append(f"R1 leak: control byte-identical on {same} of 20 seeds")
    return same


def main():
    if len(sys.argv) != 3:
        print(__doc__)
        return 2
    out, acc = sys.argv[1], sys.argv[2]
    problems = []
    rcs = glob.glob(os.path.join(out, "*", "s*", "rc"))
    if len(rcs) != NRUNS:
        problems.append(f"{len(rcs)} runs with an rc, not {NRUNS}")
    for p in rcs:
        d = os.path.dirname(p)
        if not cs.ok(d):
            problems.append(f"non-zero exit: {d}")
        try:
            if SOURCE not in open(os.path.join(d, "run_id.txt")).read():
                problems.append(f"run_id not '{SOURCE}': {d}")
        except OSError:
            problems.append(f"no run_id.txt: {d}")

    ref = {cfg: aa.collect(acc, f"M_{cfg}_k10", "ca_tbl_full") for cfg in ("A", "B")}
    fx = {("B", t): aa.collect(out, cell_name("B", t), "fixed_threshold") for t in B_T}
    fx[("A", 12)] = aa.collect(out, "O_A_T12", "fixed_threshold")
    for (cfg, t), v in list(fx.items()) + [((c, "ca_tbl_full"), r) for c, r in ref.items()]:
        if len(v) < 20:
            problems.append(f"{cfg} {t}: {len(v)} seeds, not 20")
    same = control(out, acc, problems)

    md = ["# ACC-A-OPPOINT", "",
          f"fixed_threshold@T − ca_tbl_full (ACC-A M_k10), paired seeds 1-20, mean [95 % bootstrap]. "
          f"Control byte-identical on {same} of 20 seeds.", ""]
    table = []
    diffs = {}
    for cfg, t in [("B", t) for t in B_T] + [("A", 12)]:
        md += [f"## {cfg}, T = {t}", "", "| metric | fixed_threshold | ca_tbl_full | difference |", "|---|---|---|---|"]
        for k, label, _ in KEYS:
            iv = aa.paired(fx[(cfg, t)], ref[cfg], k)
            diffs[(cfg, t, k)] = iv
            mf = sum(x[k] for x in fx[(cfg, t)].values()) / max(len(fx[(cfg, t)]), 1)
            mr = sum(x[k] for x in ref[cfg].values()) / max(len(ref[cfg]), 1)
            md.append(f"| {label} | {mf:.3f} | {mr:.3f} | {cs.fmt(*iv)} |")
            table.append(dict(config=cfg, T=t, metric=k, fixed=mf, ca_tbl_full=mr,
                              diff=iv[0], lo=iv[1], hi=iv[2]))
        rs = [rule_share(d, t) for d in cs.seeds(out, cell_name(cfg, t)) if cs.ok(d)]
        n_rule, n_esc = sum(a for a, _ in rs), sum(b for _, b in rs)
        md += ["", f"Escalation decisions by fixed_threshold's own rule (trace > {t}): "
               f"{n_rule} of {n_esc}" + (f" ({100.0 * n_rule / n_esc:.1f} %)" if n_esc else ""), ""]

    def inc0(cfg, t, k):
        iv = diffs.get((cfg, t, k))
        return iv is not None and aa.inc0(iv)

    def mean(cfg, t, k):
        v = fx[(cfg, t)]
        return sum(x[k] for x in v.values()) / len(v) if v else float("nan")

    mono = all(mean("B", a, k) >= mean("B", b, k) for k in ("av", "J") for a, b in zip(B_T, B_T[1:]))
    verdicts = [
        ("R1", "control byte-identical, 20 of 20", same == 20),
        ("R2", "B T12 availability contains 0", inc0("B", 12, "av")),
        ("R3", "B T12 J contains 0", inc0("B", 12, "J")),
        ("R4", "B T12 CEP50 and CEP95 contain 0", inc0("B", 12, "c50") and inc0("B", 12, "c95")),
        ("R5", "B availability and J non-increasing in T", mono),
        ("R6", "B some T in {8,12,25} matches availability and J",
         any(inc0("B", t, "av") and inc0("B", t, "J") for t in (8, 12, 25))),
        ("R7", "A T12 availability, CEP50, J contain 0",
         inc0("A", 12, "av") and inc0("A", 12, "c50") and inc0("A", 12, "J")),
        ("R8", "B T12 UWB cycles contain 0", inc0("B", 12, "cuwb")),
    ]
    md += ["## Registered predictions", "", "| # | prediction | verdict |", "|---|---|---|"]
    md += [f"| {n} | {txt} | {'held' if v else 'failed'} |" for n, txt, v in verdicts]
    md += ["", f"Problems: {len(problems)}"] + [f"- {p}" for p in problems]
    text = "\n".join(md) + "\n"
    print(text)
    open(os.path.join(out, "OPPOINT.md"), "w").write(text)
    with open(os.path.join(out, "oppoint_table.csv"), "w", newline="") as fh:
        w = csv.DictWriter(fh, fieldnames=list(table[0]))
        w.writeheader()
        w.writerows(table)
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main())
