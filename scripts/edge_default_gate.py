#!/usr/bin/env python3
"""L.2.3 / G21 -- the estimator-location default moves energy, and nothing else.

WHY THIS EXISTS. --gatewayEdgeServer became TRUE by default on 2026-09-29, and the whole
case for flipping a default that every result in the tree predates was that it changes
what the node is BILLED and not what the system DOES: the estimator's inputs and
arithmetic are the same wherever it is modelled to run. B3 C6 measured that on 20 seeds.
Nothing kept it true afterwards. The moment energy feeds back into control flow at
defaults -- a battery term read by a decision, a duty budget that counts receive time --
the default would start moving accuracy, and every energy-only statement made about it
would be wrong while looking right. This gate asks the question on every suite run.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9). Two arm_summary.csv files from one binary at
the same seed and duration: the first at defaults, the second with
--gatewayEdgeServer=false. The defect has two forms, and both are visible in rows that
must be present:
  * PRESENCE -- a non-energy column differs between the two files;
  * ABSENCE -- the edge path stopped running (a reverted default, a dead branch), which
    shows as rows that should differ and do not. So reachability is checked by
    requiring a DIFFERENCE on every row that had a fix, never by scanning for one.

WHAT IT CHECKS
  1. Both files read, same header, same (arm, seed) rows in the same order, and at
     least one row (rows COUNTED, rule 12).
  2. Every column outside the movable set is byte-identical on every row. The movable
     set is exactly what the edge path writes: totalJ, accuracyPerJoule, and the
     per-tier j_, rx_ and idle_ energies. tx_, cad_ and sleep_ are NOT movable: the edge
     path adds no transmission, no CAD and no sleep.
  3. Reachability (rule 4): every row with fixCycles > 0 has a different totalJ -- the
     downlink is charged on every fix -- and at least one such row exists. Every row
     with fixCycles == 0 is identical in every movable column: the path runs only on a
     fix, so energy moving on a row without one is a charge from nowhere.
  4. The sect 6 invariant, per tier: where cycles_<tier> == 0 the row's j_, rx_ and
     idle_ for that tier are identical in both files. A tier the arm never used is not
     charged, whichever node runs the estimator.

It does not test the SIZE or SIGN of the saving: that is a measurement (B3 C6), and a
gate that encoded it would turn a finding into a threshold.

usage: edge_default_gate.py <arm_summary at defaults> <arm_summary with --gatewayEdgeServer=false>
exit 0 PASS, 1 FAIL, 2 usage.
"""
import csv
import sys

TIERS = ("868", "2p4", "uwb")
MOVABLE_EXACT = {"totalJ", "accuracyPerJoule"}
MOVABLE_PREFIX = ("j_", "rx_", "idle_")


def movable(col):
    return col in MOVABLE_EXACT or col.startswith(MOVABLE_PREFIX)


def read(path):
    with open(path, newline="") as fh:
        rd = csv.reader(fh)
        header = next(rd, None)
        rows = [r for r in rd if r]
    return header, rows


def main(argv):
    if len(argv) != 3:
        print("usage: edge_default_gate.py <arm_summary at defaults> "
              "<arm_summary with --gatewayEdgeServer=false>")
        return 2
    pa, pb = argv[1], argv[2]
    try:
        ha, ra = read(pa)
        hb, rb = read(pb)
    except OSError as e:
        print(f"G21 FAIL: cannot read an input: {e}")
        return 1
    if not ha or ha != hb:
        print("G21 FAIL: the two headers differ or one is missing -- not the same binary")
        return 1
    print(f"G21 rows: {len(ra)} at defaults, {len(rb)} node-side")
    if not ra:
        print("G21 FAIL: zero rows -- nothing to compare")
        return 1
    if len(ra) != len(rb):
        print(f"G21 FAIL: row counts differ, {len(ra)} vs {len(rb)}")
        return 1
    col = {c: i for i, c in enumerate(ha)}
    for need in ("arm", "seed", "fixCycles", "totalJ"):
        if need not in col:
            print(f"G21 FAIL: no {need} column")
            return 1
    bad = []
    fixed_rows = 0
    moved_rows = 0
    moved_cols = set()
    for n, (x, y) in enumerate(zip(ra, rb), start=2):
        key = f"{x[col['arm']]} seed {x[col['seed']]}"
        if (x[col["arm"]], x[col["seed"]]) != (y[col["arm"]], y[col["seed"]]):
            bad.append(f"line {n}: rows are not the same arm and seed ({key} vs "
                       f"{y[col['arm']]} seed {y[col['seed']]})")
            continue
        if len(x) != len(ha) or len(y) != len(ha):
            bad.append(f"line {n}: {key} has the wrong number of fields")
            continue
        for c, i in col.items():
            if x[i] == y[i]:
                continue
            if movable(c):
                moved_cols.add(c)
            else:
                bad.append(f"line {n}: {key}: {c} moved, {x[i]} vs {y[i]} -- the default "
                           f"changed something other than energy")
        nfix = int(float(x[col["fixCycles"]]))
        moved = any(x[i] != y[i] for c, i in col.items() if movable(c))
        if moved:
            moved_rows += 1
        if nfix > 0:
            fixed_rows += 1
            if x[col["totalJ"]] == y[col["totalJ"]]:
                bad.append(f"line {n}: {key}: {nfix} fix cycles and totalJ identical -- "
                           f"the edge path did not run")
        elif moved:
            bad.append(f"line {n}: {key}: no fix cycles and energy moved -- the downlink "
                       f"is charged only on a fix")
        for t in TIERS:
            cc = f"cycles_{t}"
            if cc in col and x[col[cc]] == y[col[cc]] and int(float(x[col[cc]])) == 0:
                for pre in MOVABLE_PREFIX:
                    c = f"{pre}{t}"
                    if c in col and x[col[c]] != y[col[c]]:
                        bad.append(f"line {n}: {key}: {c} moved on a tier with 0 cycles -- "
                                   f"a tier that was not used was charged (sect 6)")
    if fixed_rows == 0:
        bad.append("no row has a fix cycle -- the edge path cannot have been exercised")
    print(f"G21 rows with a fix: {fixed_rows}; rows whose energy moved: {moved_rows}; "
          f"columns that moved: {sorted(moved_cols)}")
    if bad:
        for b in bad[:12]:
            print(f"G21 FAIL: {b}")
        if len(bad) > 12:
            print(f"G21 FAIL: ... and {len(bad) - 12} more")
        return 1
    print("G21 PASS: only energy moved, on every row with a fix and on no row without one")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
