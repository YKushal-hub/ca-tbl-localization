#!/usr/bin/env python3
"""G177 rubble gate (collapse extension, World side).

Reads the key=value output of build/scratch/ns3.47-ca_tbl_rubble_selftest-optimized
(argv[1]) and checks:
  1. ROW: every model value equals the number written in its row of
     docs/SOURCES_COLLAPSE.md (the row is located by ID and the number must appear
     in that row's text), to the row's printed precision.
  2. A5: the validation targets (NIST bounds, Oestges detection range).
  3. GEO: exact geometry fixtures.
  4. NEG: refusals (overlap, degenerate, void outside / overlapping, bad bracket).
  5. LEAK: no system-side source file includes the World header.
Exit 0 only if all pass. Run via scripts/rubble_gate.sh.
"""
import glob
import os
import re
import sys

ROOT = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
LOG = os.path.join(ROOT, "docs", "SOURCES_COLLAPSE.md")

# (selftest key, row ID prefix, value as printed in the row)
ROW_CHECKS = [
    ("itu_1000", "ITU2040-T3-CONC", "33.02"),
    ("itu_2000", "ITU2040-T3-CONC", "56.78"),
    ("itu_2400", "ITU2040-T3-CONC", "65.49"),
    ("itu_6500", "ITU2040-T3-CONC", "142.76"),
    ("itu_8000", "ITU2040-T3-CONC", "167.94"),
    ("A_low_868", "OES-LAYER-PLAIN", "6.63"),
    ("A_low_2400", "OES-LAYER-PLAIN", "25.45"),
    ("A_central_868", "OES-LAYER-FERRO", "11.90"),
    ("A_central_2400", "OES-LAYER-FERRO", "38.21"),
    ("A_high_868", "OES-MEAS-80", "30.71"),
    ("A_high_2400", "OES-MEAS-80", "98.60"),
    ("eta_low", "ETA-LOW", "0.3522"),
    ("eta_central", "ETA-CEN", "0.5459"),
    ("eta_high", "ETA-HIGH", "0.7706"),
    ("A_low_6500", "UWB65-LOW", "50.28"),
    ("A_central_6500", "UWB65-LOW", "77.94"),
    ("A_high_6500", "UWB65-LOW", "186.74"),
    ("A_low_8000", "UWB8-LOW", "59.15"),
    ("A_central_8000", "UWB8-LOW", "91.68"),
    ("A_high_8000", "UWB8-LOW", "219.7"),
    ("bias_low", "SLOW-BR", "0.454"),
    ("bias_central", "SLOW-BR", "0.704"),
    ("bias_high", "SLOW-BR", "1.541"),
]
# Anchors the code reproduces exactly (Oestges points, by construction).
EXACT = [("A_low_1000", 8.0), ("A_low_2000", 20.0), ("A_central_1000", 14.0),
         ("A_central_2000", 31.0), ("A_high_2000", 80.0)]
GEO = [("add_pile", 1), ("add_void", 1), ("geo_vert_void", 3.0), ("geo_vert", 4.0),
       ("geo_partial", 1.0), ("geo_diag", 5.0), ("geo_out", 0.0), ("geo_sym", 3.0),
       ("in_void", 0), ("in_pile", 1), ("pos_parse", 1)]
NEG = ["neg_overlap", "neg_degenerate", "neg_void_outside", "neg_void_overlap", "neg_parse"]

# Files that are WORLD side or the physics engines (allowed to include the header).
LEAK_ALLOWED = {"ca-tbl-rubble-world.h", "ca-tbl-rubble-world.cc", "lora-physics-engine.h",
                "lora-2p4-physics-engine.h", "lora-uwb-physics-engine.h",
                "ca_tbl_rubble_selftest.cc", "ca_tbl_causal_sim.cc"}
# System-side code: estimators, policy, MAC, filters. Must never see the World header.
SYSTEM_GLOBS = ["src/lora-physics-engine/model/ca-tbl-*.cc", "src/lora-physics-engine/model/ca-tbl-*.h"]


def row_text(log, rid):
    for line in log.splitlines():
        if line.startswith("| " + rid):
            return line
    return None


def decimals(s):
    return len(s.split(".")[1]) if "." in s else 0


def main():
    if len(sys.argv) != 2:
        print("usage: rubble_gate.py SELFTEST_OUTPUT")
        return 2
    vals = {}
    for line in open(sys.argv[1]):
        if "=" in line:
            k, v = line.strip().split("=", 1)
            vals[k] = float(v)
    log = open(LOG, encoding="utf-8").read()
    fails = []

    for key, rid, printed in ROW_CHECKS:
        row = row_text(log, rid)
        if row is None:
            fails.append(f"ROW {key}: row {rid} not found")
            continue
        cells = row.split("|")
        value_cell = cells[2] if len(cells) > 2 else ""
        if "**" in value_cell:
            # the row states its result in the Value column: the number must be there
            if printed not in value_cell:
                fails.append(f"ROW {key}: '{printed}' not in the Value column of row {rid}")
                continue
        elif not re.search(r"[=→]\s*(\*\*)?" + re.escape(printed) + r"(?![0-9])", row):
            # otherwise it must appear as a computed result ("= x") in the row
            fails.append(f"ROW {key}: '= {printed}' not in row {rid}")
            continue
        got = round(vals[key], decimals(printed))
        if abs(got - float(printed)) > 10 ** (-decimals(printed)) / 2 + 1e-12:
            fails.append(f"ROW {key}: code {vals[key]:.6f} != row {rid} {printed}")
    for key, exp in EXACT:
        if abs(vals[key] - exp) > 1e-9:
            fails.append(f"EXACT {key}: {vals[key]} != {exp}")

    # A5.1 NIST: 868 MHz totals over 2-5 m of pile must span both measured lower bounds,
    # ">= 30 to 40 dB" (TN 1542, voids remained) and ">= 60 to 80 dB" (TN 1540).
    lo2 = 2 * vals["A_low_868"]
    hi5 = 5 * vals["A_high_868"]
    if not (lo2 <= 30.0 and hi5 >= 80.0):
        fails.append(f"A5.1 868 totals [{lo2:.1f}, {hi5:.1f}] dB do not span [30, 80]")
    # Central 2-5 m must reach the void-rich bound (30 dB) somewhere in that depth range.
    if not (5 * vals["A_central_868"] >= 30.0):
        fails.append("A5.1 central 868 at 5 m below 30 dB")
    # A5.4 Oestges Table III: GSM-900, 130 dB dynamic range, concrete + mixed ~10 m.
    rng = 130.0 / vals["A_central_868"]
    if abs(rng - 10.0) / 10.0 > 0.25:
        fails.append(f"A5.4 detection range {rng:.2f} m not within 25% of ~10 m")

    for key, exp in GEO:
        if abs(vals[key] - exp) > 1e-9:
            fails.append(f"GEO {key}: {vals[key]} != {exp}")
    if abs(vals["loss_2m_868"] - 2 * vals["A_central_868"]) > 1e-6:
        fails.append("GEO loss_2m_868 != 2 m x central 868")
    if abs(vals["bias_2m"] - 2 * vals["bias_central"]) > 1e-6:
        fails.append("GEO bias_2m != 2 m x central bias")
    for key in NEG:
        if vals.get(key, 1) != 0:
            fails.append(f"NEG {key}: not refused")

    for pat in SYSTEM_GLOBS:
        for f in glob.glob(os.path.join(ROOT, pat)):
            if os.path.basename(f) in LEAK_ALLOWED:
                continue
            if re.search(r'#include\s+"(ns3/)?ca-tbl-rubble-world\.h"', open(f, encoding="utf-8").read()):
                fails.append(f"LEAK {os.path.relpath(f, ROOT)} includes the World header")

    n = len(ROW_CHECKS) + len(EXACT) + 3 + len(GEO) + 2 + len(NEG) + 1
    if fails:
        print("G177 FAIL")
        for f in fails:
            print("  " + f)
        return 1
    print(f"G177 PASS ({n} checks; detection range {rng:.2f} m; 868 span [{lo2:.1f}, {hi5:.1f}] dB)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
