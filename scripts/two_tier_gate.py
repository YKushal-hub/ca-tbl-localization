#!/usr/bin/env python3
"""Gate G167: the ca_tbl_two_tier arm (report/PREREG_LIMITS_2026-11-14.md, L2). Reads the run set written by
scripts/two_tier_gate.sh: <dir>/all (the default arms list), <dir>/pair (--arms=ca_tbl_full,ca_tbl_two_tier),
<dir>/off (the pair, --tier1ExtraLossDb absent) and <dir>/zero (the pair, --tier1ExtraLossDb=0 given).

POPULATION, stated because a check over an empty set passes vacuously (rule 4): the checks on tier 2 are made on
ca_tbl_two_tier's tag-0 row and on every subject row of that arm; they are only meaningful because the same run's
ca_tbl_full row is shown to USE tier 2 (cycles_2p4 > 0, tx_2p4 > 0) and the two-tier row is shown to REACH UWB
(cycles_uwb > 0, escalations > 0) -- an arm that never left 868 MHz would show zero tier 2 trivially.

  ARM      the pair run has a ca_tbl_two_tier row, and 'all' has none (the arm is by name only)
  TIER2    two-tier: cycles_2p4 = 0 and tx/rx/cad/idle_2p4 = 0 on tag 0 and on every subject; its tier-2 joules are
           the sleep current alone (j_2p4 == sleep_2p4)
  REACH    two-tier: cycles_uwb > 0 and escalations > 0; ca_tbl_full: cycles_2p4 > 0 and tx_2p4 > 0
  ONETIER  two-tier: cycles_868 + cycles_uwb == steps (exactly one tier per cycle)
  DEFAULT  the pair's ca_tbl_full row is byte-identical to the 'all' run's (the arm does not move another arm)
  FLAG     --tier1ExtraLossDb given as 0 leaves every non-provenance output byte-identical to the flag absent
  PROBE    --tier1ExtraLossDb=30 cuts the probe's 868 MHz r80 to under half, and leaves the 2.4 GHz and UWB r80 equal
           (<dir>/probe0.log, <dir>/probe30.log: --probe=true runs without and with the flag)
usage: two_tier_gate.py <dir>
"""
import csv
import filecmp
import os
import sys

bad = []


def fail(code, msg):
    bad.append(code)
    print(f"  G167 FAIL  {code:<8} {msg}")


def rows(path):
    with open(path) as f:
        return list(csv.DictReader(f))


def one(rs, arm):
    m = [r for r in rs if r["arm"] == arm]
    return m[0] if m else None


def r80s(path):
    import re
    return [float(x) for x in re.findall(r"usable range: r80=([0-9.]+) m", open(path, errors="replace").read())]


def main(d):
    for sub in ("all", "pair", "off", "zero", "probe0", "probe30"):
        rc = os.path.join(d, sub + ".rc")
        if not os.path.exists(rc) or open(rc).read().strip() != "0":
            fail("RUNS", f"{sub}: run did not exit 0")
            return 1
    allr = rows(os.path.join(d, "all", "arm_summary.csv"))
    pair = rows(os.path.join(d, "pair", "arm_summary.csv"))
    two, full = one(pair, "ca_tbl_two_tier"), one(pair, "ca_tbl_full")
    if two is None:
        fail("ARM", "the pair run has no ca_tbl_two_tier row")
        return 1
    if one(allr, "ca_tbl_two_tier") is not None or len(allr) != 6:
        fail("ARM", f"'all' has {len(allr)} rows or includes ca_tbl_two_tier")
    if full is None:
        fail("REACH", "no ca_tbl_full row to show the check can see tier 2")
        return 1
    f = lambda r, k: float(r[k])
    for k in ("cycles_2p4", "tx_2p4", "rx_2p4", "cad_2p4", "idle_2p4"):
        if f(two, k) != 0.0:
            fail("TIER2", f"tag 0 {k} = {two[k]}")
    if abs(f(two, "j_2p4") - f(two, "sleep_2p4")) > 1e-9:
        fail("TIER2", f"tag 0 j_2p4 {two['j_2p4']} is not the sleep joules {two['sleep_2p4']}")
    subs = [r for r in rows(os.path.join(d, "pair", "subject_summary.csv")) if r["arm"] == "ca_tbl_two_tier"]
    if not subs:
        fail("TIER2", "no subject rows for ca_tbl_two_tier (population empty)")
    for r in subs:
        for k in ("cycles_2p4", "tx_2p4"):
            if f(r, k) != 0.0:
                fail("TIER2", f"subject {r['subjectId']} {k} = {r[k]}")
        if abs(f(r, "j_2p4") - f(r, "sleep_2p4")) > 1e-9:
            fail("TIER2", f"subject {r['subjectId']} j_2p4 is not the sleep joules")
    if not (f(two, "cycles_uwb") > 0 and f(two, "escalations") > 0):
        fail("REACH", f"two-tier never reached UWB (cycles_uwb {two['cycles_uwb']}, escalations {two['escalations']})")
    if not (f(full, "cycles_2p4") > 0 and f(full, "tx_2p4") > 0):
        fail("REACH", "ca_tbl_full shows no tier-2 use in the same run: the zero check could not have fired")
    if f(two, "cycles_868") + f(two, "cycles_uwb") != f(two, "steps"):
        fail("ONETIER", "cycles_868 + cycles_uwb != steps")
    fa = one(allr, "ca_tbl_full")
    if fa is None or fa != full:
        fail("DEFAULT", "ca_tbl_full's row differs between the default-arms run and the pair run")
    skip = {"run_id.txt"}
    names = sorted(n for n in os.listdir(os.path.join(d, "off")) if n not in skip)
    if not names:
        fail("FLAG", "off run is empty")
    for n in names:
        a, b = os.path.join(d, "off", n), os.path.join(d, "zero", n)
        if not os.path.exists(b) or not filecmp.cmp(a, b, shallow=False):
            fail("FLAG", f"{n} differs with --tier1ExtraLossDb=0")
            break
    a, b = r80s(os.path.join(d, "probe0.log")), r80s(os.path.join(d, "probe30.log"))
    if len(a) != 3 or len(b) != 3:
        fail("PROBE", f"expected 3 r80 lines per probe, got {len(a)} and {len(b)}")
    else:
        if not b[0] < 0.5 * a[0]:
            fail("PROBE", f"868 MHz r80 {a[0]} -> {b[0]} with +30 dB: not cut to under half")
        if b[1] != a[1] or b[2] != a[2]:
            fail("PROBE", f"2.4 GHz / UWB r80 moved: {a[1:]} -> {b[1:]}")
    if bad:
        print(f"  G167 {len(bad)} failure(s)")
        return 1
    print("  G167 ok: ca_tbl_two_tier by name only; zero tier-2 cycles/tx/rx/cad/idle and sleep-only tier-2 joules "
          f"on tag 0 and {len(subs)} subjects; reached UWB ({two['cycles_uwb']} cycles); ca_tbl_full uses tier 2 "
          f"({full['cycles_2p4']} cycles); flag at 0 identical on {len(names)} files; "
          f"+30 dB: 868 MHz r80 {a[0]} -> {b[0]} m, tiers 2/3 unchanged")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1]))
