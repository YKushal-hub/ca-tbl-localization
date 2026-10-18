#!/usr/bin/env python3
"""G138: V1 --rngKeyed (master plan v3; TECH_DEBT 1.1).
The perturbation is --rngKeyedProbeExtraTx: one extra 868 MHz engine evaluation on a link no node uses.
  RUNS       a run's rc is not 0, or a directory is missing.
  INVARIANT  keyed: an output (run_id.txt excepted) differs between the run with and without the perturbation.
  RESIDUE    keyed: a run does not print 'RNGKEYED unkeyed_draws=0' (draws outside any link context).
  REAL       unkeyed: the perturbation changes NO output -- then the perturbation is inert and INVARIANT is vacuous.
  EFFECT     the keyed and unkeyed runs have the same arm_summary.csv -- the flag changed nothing.
"""
import filecmp, os, re, sys


def differing(d, a, b):
    fa = sorted(f for f in os.listdir(os.path.join(d, a)) if f != "run_id.txt")
    fb = sorted(f for f in os.listdir(os.path.join(d, b)) if f != "run_id.txt")
    out = sorted(set(fa) ^ set(fb))
    out += [f for f in fa if f in fb and not filecmp.cmp(os.path.join(d, a, f), os.path.join(d, b, f), shallow=False)]
    return out


def main():
    d = sys.argv[1]; res = []
    runs = ("on", "onp", "off", "offp")
    bad = [r for r in runs if not os.path.isdir(os.path.join(d, r)) or not os.path.exists(os.path.join(d, r + ".rc"))
           or open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"bad: {bad}" if bad else "4 runs exit 0"))
    if bad:
        return report(res)
    dk = differing(d, "on", "onp")
    res.append(("INVARIANT", not dk, f"{len(dk)} outputs differ under the perturbation {dk[:3]}"))
    lines = {r: re.findall(r"RNGKEYED unkeyed_draws=(\d+)", open(os.path.join(d, r + ".log"), errors="replace").read()) for r in ("on", "onp")}
    ok = all(v and v[-1] == "0" for v in lines.values())
    res.append(("RESIDUE", ok, f"unkeyed draws {dict((k, v[-1] if v else None) for k, v in lines.items())}"))
    du = differing(d, "off", "offp")
    res.append(("REAL", bool(du), f"unkeyed: {len(du)} outputs move under the perturbation"))
    same = filecmp.cmp(os.path.join(d, "on", "arm_summary.csv"), os.path.join(d, "off", "arm_summary.csv"), shallow=False)
    res.append(("EFFECT", not same, "keyed and unkeyed arm_summary differ" if not same else "identical"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G138 {'PASS' if ok else 'FAIL'} {label:9s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
