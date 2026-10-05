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

WHICH RADIO THE DOWNLINK IS BILLED ON (TECH_DEBT M.10, --active). Since 2026-09-30 the
downlink is billed on the tier the fix came from (--edgeDownlinkTier=fix), not on the
active tier (=active, the old bill). The two differ only on a hybrid arm's floor-solved
fix cycle, where the fix came from the 868 MHz floor while 2.4 GHz or UWB was active; there
one downlink moves from the active tier onto 868 MHz. Given a third arm_summary.csv, from
the same binary, seed and duration at --edgeDownlinkTier=active, this checks that decision
the way checks 1-4 check the estimator location. On disk (rule 9):
  * PRESENCE -- the bill moved something other than energy, moved a downlink OFF 868 MHz
    or ONTO a tier other than 868 MHz, charged 868 MHz where it had not transmitted, or
    moved energy on a row whose arm never had a tier other than 868 active;
  * ABSENCE -- the default reverted to the active tier, or the fix-tier branch went dead,
    which shows as no row moving at all.
  5. Same header and the same (arm, seed) rows, in order, as the defaults file.
  6. Every column outside the movable set is byte-identical on every row.
  7. Per row, a downlink can only TRANSFER onto 868 MHz from the tier that was active:
     j_868 never falls, j_2p4 and j_uwb never rise, and a row that moved at all gains on
     868 MHz AND loses on 2.4 GHz or UWB -- energy that appears on one side only is a
     charge from nowhere. Each tier's rx_ and idle_ follow its j_ in direction.
  8. The sect 6 invariant for the move: a row whose 868 MHz energy rose has tx_868 > 0,
     so the downlink went onto a radio that transmitted in that run. (The per-cycle form
     -- 868 MHz ranged on every cycle whose bill moved -- needs the trace and was checked
     at the change, M.10.) A row with fixCycles == 0, or whose arm had only 868 MHz active
     (cycles_2p4 == cycles_uwb == 0), is identical in every movable column: there fix tier
     and active tier cannot differ.
  9. Reachability: at least one row moved.
It does not test how many downlinks moved or what each cost; the change measured that
from the traces (0.9042 mJ onto 868 MHz, 0.2005 mJ off 2.4 GHz, 0.3224 mJ off UWB, per
floor-solved fix at the defaults), and a gate that encoded it would be a threshold on a
radio profile.

usage: edge_default_gate.py <arm_summary at defaults> <arm_summary with --gatewayEdgeServer=false>
                            [--active <arm_summary with --edgeDownlinkTier=active>]
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


def check_active(ha, ra, pc):
    """Checks 5-9: the defaults file against the same run at --edgeDownlinkTier=active.
    Returns a list of failure strings; empty is a pass."""
    try:
        hc, rc = read(pc)
    except OSError as e:
        return [f"cannot read the --active input: {e}"]
    if not hc or hc != ha:
        return ["the --active file's header differs or is missing -- not the same binary"]
    if len(rc) != len(ra):
        return [f"--active row counts differ, {len(ra)} at defaults vs {len(rc)}"]
    col = {c: i for i, c in enumerate(ha)}
    for need in ("arm", "seed", "fixCycles", "cycles_2p4", "cycles_uwb", "tx_868"):
        if need not in col:
            return [f"no {need} column"]
    bad = []
    moved_rows = []
    moved_cols = set()
    for n, (x, y) in enumerate(zip(ra, rc), start=2):
        key = f"{x[col['arm']]} seed {x[col['seed']]}"
        if (x[col["arm"]], x[col["seed"]]) != (y[col["arm"]], y[col["seed"]]):
            bad.append(f"--active line {n}: rows are not the same arm and seed ({key} vs "
                       f"{y[col['arm']]} seed {y[col['seed']]})")
            continue
        if len(x) != len(ha) or len(y) != len(ha):
            bad.append(f"--active line {n}: {key} has the wrong number of fields")
            continue
        # 6. nothing but energy
        for c, i in col.items():
            if x[i] == y[i]:
                continue
            if movable(c):
                moved_cols.add(c)
            else:
                bad.append(f"--active line {n}: {key}: {c} moved, {x[i]} vs {y[i]} -- the "
                           f"downlink's tier changed something other than energy")
        moved = any(x[i] != y[i] for c, i in col.items() if movable(c))
        if not moved:
            continue
        moved_rows.append(key)
        # 8. rows where fix tier and active tier cannot differ
        if int(float(x[col["fixCycles"]])) == 0:
            bad.append(f"--active line {n}: {key}: no fix cycles and energy moved -- the "
                       f"downlink is billed only on a fix")
        if (int(float(x[col["cycles_2p4"]])) == 0 and int(float(x[col["cycles_uwb"]])) == 0):
            bad.append(f"--active line {n}: {key}: only 868 MHz was ever active and energy "
                       f"moved -- there the fix's tier IS the active tier")
        # 7. a transfer onto 868 MHz from the active tier, never the reverse
        d = {}
        for t in TIERS:
            for pre in MOVABLE_PREFIX:
                c = f"{pre}{t}"
                if c in col:
                    d[c] = float(x[col[c]]) - float(y[col[c]])
        for pre in MOVABLE_PREFIX:
            c = f"{pre}868"
            if d.get(c, 0.0) < 0.0:
                bad.append(f"--active line {n}: {key}: {c} FELL at defaults ({d[c]:+.9f}) -- "
                           f"a downlink moved off 868 MHz")
            for t in ("2p4", "uwb"):
                c = f"{pre}{t}"
                if d.get(c, 0.0) > 0.0:
                    bad.append(f"--active line {n}: {key}: {c} ROSE at defaults "
                               f"({d[c]:+.9f}) -- a downlink moved onto a tier other than "
                               f"868 MHz")
        gain = d.get("j_868", 0.0) > 0.0
        loss = d.get("j_2p4", 0.0) < 0.0 or d.get("j_uwb", 0.0) < 0.0
        if gain != loss:
            bad.append(f"--active line {n}: {key}: energy moved on one side only (j_868 "
                       f"{d.get('j_868', 0.0):+.9f}, j_2p4 {d.get('j_2p4', 0.0):+.9f}, j_uwb "
                       f"{d.get('j_uwb', 0.0):+.9f}) -- a moved downlink leaves one tier "
                       f"and arrives on another")
        # 8. sect 6: the downlink went onto a radio that transmitted
        if gain and not float(x[col["tx_868"]]) > 0.0:
            bad.append(f"--active line {n}: {key}: j_868 rose with tx_868 = "
                       f"{x[col['tx_868']]} -- a downlink billed on a radio that never "
                       f"transmitted (sect 6)")
    # 9. reachability
    if not moved_rows:
        bad.append("--active: no row moved -- the downlink is still billed on the active "
                   "tier (reverted default) or the fix-tier branch never ran")
    print(f"G21 --active rows whose energy moved: {len(moved_rows)} "
          f"({', '.join(moved_rows)}); columns that moved: {sorted(moved_cols)}")
    return bad


def main(argv):
    args = list(argv[1:])
    pc = None
    if "--active" in args:
        k = args.index("--active")
        if k + 1 >= len(args):
            print("usage: --active needs a file")
            return 2
        pc = args[k + 1]
        del args[k:k + 2]
    if len(args) != 2:
        print("usage: edge_default_gate.py <arm_summary at defaults> "
              "<arm_summary with --gatewayEdgeServer=false> "
              "[--active <arm_summary with --edgeDownlinkTier=active>]")
        return 2
    pa, pb = args[0], args[1]
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
    if pc is not None:
        bad.extend(check_active(ha, ra, pc))
    if bad:
        for b in bad[:12]:
            print(f"G21 FAIL: {b}")
        if len(bad) > 12:
            print(f"G21 FAIL: ... and {len(bad) - 12} more")
        return 1
    print("G21 PASS: only energy moved, on every row with a fix and on no row without one")
    if pc is not None:
        print("G21 PASS: the downlink is billed on the fix's tier -- it moved onto 868 MHz "
              "from the active tier, on energy only, and only where 868 MHz transmitted")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
