#!/usr/bin/env python3
"""B3 regeneration -- control-flow checks by byte identity. SUPPLEMENTARY, not registered.

Byte identity to an original is admissible under report/PREREG_B3_REGEN.md section 1: it
is a control-flow fact (nothing on that path moved), and it licenses no magnitude claim.
This file only says WHICH COLUMNS of which rows are byte-identical between two cells. It
computes no difference and no test.

    python3 scripts/b3/b3_identity_checks.py

Sections:
  E3-cols   regen27_k10/wanmesh_move vs results/L1_imuoff (registered expectation E3):
            per seed, which columns differ, so a failed E3 names its code path
  K10-orig  regen27_k10/<cell> vs results/k10_<cell> (the K10 originals, 5d20de9)
  IMU-orig  regen27_imu/on vs results/L2_imuon (4302918), disclosed non-identical at seed 7
  WANONLY   ca_tbl_full vs static_868 inside regen27_k10/wanonly_move, per seed: the ties
            in P3.2 are exact zeros, so they are control-flow claims (rule 4)
  EDGE      regen27_edge nodeside vs edge, per arm, which columns differ
"""
import csv
import os
import sys

CHUNKS = (7, 12, 17, 22)


def rows_of(path):
    with open(path, newline="") as f:
        return list(csv.DictReader(f))


def chunked(prefix):
    out = []
    for c in CHUNKS:
        out.extend(rows_of("%s_s%d/arm_summary.csv" % (prefix, c)))
    return {(r["arm"], int(r["seed"])): r for r in out}


def colsdiff(a, b):
    return sorted(k for k in a if a[k] != b.get(k))


def compare(label, A, B):
    print("### %s" % label)
    keys = sorted(set(A) & set(B), key=lambda k: (k[1], k[0]))
    if len(keys) != len(A) or len(keys) != len(B):
        print("  row sets differ: %d vs %d, %d shared" % (len(A), len(B), len(keys)))
    ident = sum(1 for k in keys if A[k] == B[k])
    print("  rows byte-identical (every column): %d of %d" % (ident, len(keys)))
    colcount = {}
    seeds_by_col = {}
    for k in keys:
        for c in colsdiff(A[k], B[k]):
            colcount[c] = colcount.get(c, 0) + 1
            seeds_by_col.setdefault(c, set()).add(k[1])
    if colcount:
        print("  columns that differ (rows affected, seeds affected):")
        for c in sorted(colcount, key=lambda c: -colcount[c]):
            print("    %-26s %3d rows  seeds %s" % (c, colcount[c], sorted(seeds_by_col[c])))
    arms = sorted(set(k[0] for k in keys))
    for arm in arms:
        ks = [k for k in keys if k[0] == arm]
        print("  %-20s rows identical %2d/%d" % (arm, sum(1 for k in ks if A[k] == B[k]), len(ks)))
    print()


def main():
    compare("E3-cols: regen27_k10/wanmesh_move vs results/L1_imuoff",
            chunked("results/regen27_k10/wanmesh_move"), chunked("results/L1_imuoff"))
    for cell in ("wanonly_move", "sparse_move", "wanmesh_move"):
        compare("K10-orig: regen27_k10/%s vs results/k10_%s" % (cell, cell),
                chunked("results/regen27_k10/%s" % cell), chunked("results/k10_%s" % cell))
    compare("IMU-orig: regen27_imu/on vs results/L2_imuon",
            chunked("results/regen27_imu/on"), chunked("results/L2_imuon"))

    print("### WANONLY: ca_tbl_full vs static_868 inside regen27_k10/wanonly_move")
    w = chunked("results/regen27_k10/wanonly_move")
    for s in range(7, 27):
        a, b = dict(w[("ca_tbl_full", s)]), dict(w[("static_868", s)])
        a.pop("arm"); b.pop("arm")
        d = colsdiff(a, b)
        print("  seed %2d  escalations full=%s  cycles_2p4 full=%s  cycles_uwb full=%s  differing columns: %s"
              % (s, a["escalations"], a["cycles_2p4"], a["cycles_uwb"], d if d else "none"))
    print()

    print("### EDGE: nodeside vs edge, per arm")
    n = {(r["arm"], int(r["seed"])): r for r in rows_of("results/regen27_edge/nodeside/arm_summary.csv")}
    e = {(r["arm"], int(r["seed"])): r for r in rows_of("results/regen27_edge/edge/arm_summary.csv")}
    arms = sorted(set(k[0] for k in n))
    for arm in arms:
        cols = {}
        for s in range(7, 27):
            for c in colsdiff(n[(arm, s)], e[(arm, s)]):
                cols[c] = cols.get(c, 0) + 1
        dj = [float(n[(arm, s)][c]) - float(e[(arm, s)][c]) for s in range(7, 27)
              for c in ("j_868",)]
        dj2 = [float(n[(arm, s)]["j_2p4"]) - float(e[(arm, s)]["j_2p4"]) for s in range(7, 27)]
        print("  %-20s differing columns %s | mean nodeside-edge j_868 %+.4f J, j_2p4 %+.4f J"
              % (arm, cols if cols else "none", sum(dj) / 20, sum(dj2) / 20))
    return 0


if __name__ == "__main__":
    sys.exit(main())
