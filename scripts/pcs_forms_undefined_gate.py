#!/usr/bin/env python3
"""G72: analyse_pcs_forms.py reads an undefined statistic as undefined (TECH_DEBT M.8).

usage: pcs_forms_undefined_gate.py [<analysis script>]   (default: the repo's)

The campaign's traces are not tracked, so the gate builds a synthetic ROOT in a temp dir:
four forms x 20 seeds x one arm, 60 fix cycles a seed. In legacy, v2 and weighted the score
falls with the error (a defined rho); in gated it is constant on every fix cycle, which is
the case M.8 describes (gated PCS is constant on 99.98 % of fix cycles), so its Spearman rho
is undefined on every seed.

Checks (each prints "G72 ok|FAIL <LABEL> ..."):
  DEFAULT  with no option the output is byte-identical to the script as both registrations
           applied it (d82b865), so their verdicts reproduce; gated reads
           "indistinguishable" there (the defect, kept).
  STRICT   with --strict-undefined, gated reads "undefined (rho undefined on 20 of 20
           seeds)", the other three forms keep their default verdicts, and every cell
           prints its fix-cycle count (1 200 over 20 seeds).
"""
import csv
import os
import random
import re
import subprocess
import sys
import tempfile

HERE = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(HERE)
SCRIPT = sys.argv[1] if len(sys.argv) > 1 else os.path.join(REPO, "scratch/python_handoff/analyse_pcs_forms.py")
REF = "d82b865"
nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    if ok:
        nok += 1
        print(f"  G72 ok      {label:<8}{msg}")
    else:
        nfail += 1
        print(f"  G72 FAIL    {label:<8}{msg}")


def build(root):
    rnd = random.Random(72)
    for form in ("legacy", "v2", "weighted", "gated"):
        for s in range(1, 21):
            d = os.path.join(root, form, f"seed_{s}")
            os.makedirs(d)
            with open(os.path.join(d, "arm_summary.csv"), "w", newline="") as f:
                w = csv.writer(f, lineterminator="\n")
                w.writerow(["arm", "cep50M", "totalJ", "escalations"])
                w.writerow(["ca_tbl_full", f"{10 + rnd.random():.3f}", f"{50 + rnd.random():.3f}", rnd.randint(5, 9)])
            with open(os.path.join(d, f"trace_ca_tbl_full_seed{s}.csv"), "w", newline="") as f:
                w = csv.writer(f, lineterminator="\n")
                w.writerow(["hadFix", "errM", "pcs", "traceSigma"])
                for _ in range(60):
                    e = rnd.uniform(1, 50)
                    p = 0.5 if form == "gated" else max(0.0, 1.0 - e / 60 + rnd.gauss(0, 0.05))
                    w.writerow([1, f"{e:.3f}", f"{p:.4f}", f"{e * e / 4 + rnd.uniform(0, 30):.3f}"])


def run(script, root, *opts):
    # the script reads ROOT relative to the cwd; the reference has no --root, so run it
    # from a dir where results/pcs_forms is the fixture
    cwd = os.path.dirname(os.path.dirname(root))
    return subprocess.run([sys.executable, script, *opts], cwd=cwd, capture_output=True, text=True).stdout


def verdicts(out):
    return {m.group(1): m.group(2).strip() for m in re.finditer(r"^  (legacy|v2|weighted|gated) +ca_tbl_full +\S+ +\S+ +\S+ +\[[^\]]*\]  (.*)$", out, re.M)}


def main():
    with tempfile.TemporaryDirectory(dir=os.environ.get("TMPDIR")) as t:
        root = os.path.join(t, "results", "pcs_forms")
        build(root)
        ref = os.path.join(t, "ref.py")
        src = subprocess.run(["git", "-C", REPO, "show", f"{REF}:scratch/python_handoff/analyse_pcs_forms.py"], capture_output=True, text=True).stdout
        open(ref, "w").write(src)
        a = run(SCRIPT, root)
        b = run(ref, root)
        va = verdicts(a)
        report(a == b and a and va.get("gated") == "indistinguishable", "DEFAULT", f"no option: identical to {REF}'s output {a == b}; gated reads {va.get('gated')!r}")
        c = run(SCRIPT, root, "--strict-undefined")
        vc = verdicts(c)
        cyc = re.findall(r"fix cycles (\d+) over (\d+) seeds", c)
        same = all(vc.get(f) == va.get(f) for f in ("legacy", "v2", "weighted"))
        ok = vc.get("gated") == "undefined (rho undefined on 20 of 20 seeds)" and same and len(cyc) == 4 and all(x == ("1200", "20") for x in cyc)
        report(ok, "STRICT", f"--strict-undefined: gated reads {vc.get('gated')!r}; other forms unchanged {same}; cycle counts {cyc}")
    print(f"  G72: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
