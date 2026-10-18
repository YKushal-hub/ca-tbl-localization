#!/usr/bin/env python3
"""G140 checks; see scripts/linkleak_gate.sh for the labels."""
import csv, filecmp, os, sys


def pdr(d, run):
    return [(r["distance_m"], r["pdr"]) for r in csv.DictReader(open(os.path.join(d, run, "probe_range_sweep.csv")))
            if r["tier"].startswith("868")]


def main():
    d = sys.argv[1]; res = []
    runs = ("base", "offx", "leak1", "fix0", "fix1")
    bad = [r for r in runs if open(os.path.join(d, r + ".rc")).read().strip() != "0"]
    res.append(("RUNS", not bad, f"bad {bad}" if bad else "5 probes exit 0"))
    if bad:
        return report(res)
    res.append(("LEAK", pdr(d, "base") != pdr(d, "leak1"), "S1 changes 868 delivery without the fix"))
    res.append(("NOLEAK", pdr(d, "fix0") == pdr(d, "fix1"), "868 delivery identical with S1 on/off under the fix"))
    t = os.path.join(d, "fix0", "probe_terms.csv")
    tiers = {r["tier"] for r in csv.DictReader(open(t))} if os.path.exists(t) else set()
    dig = "probe_terms.csv" in open(os.path.join(d, "fix0", "run_id.txt")).read()
    res.append(("TERMS", len(tiers) == 3 and dig, f"tiers {sorted(tiers)}, digested {dig}"))
    same = filecmp.cmp(os.path.join(d, "base", "probe_range_sweep.csv"), os.path.join(d, "offx", "probe_range_sweep.csv"), shallow=False)
    res.append(("DEFAULT", same and not os.path.exists(os.path.join(d, "base", "probe_terms.csv")),
                "explicit false identical; no terms file by default" if same else "explicit false differs"))
    return report(res)


def report(res):
    for label, ok, why in res:
        print(f"G140 {'PASS' if ok else 'FAIL'} {label:8s} {why}")
    return 0 if all(ok for _, ok, _ in res) else 1


if __name__ == "__main__":
    sys.exit(main())
