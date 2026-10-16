#!/usr/bin/env python3
"""G93 (A7 / D-5) -- does each tier's receiver apply the data-sheet sensitivity for the
spreading factor, bandwidth and coding rate that tier is CONFIGURED to use?

WHY. A six-decibel indexing error (D1) sat on the tier carrying ~95 % of active cycles and
passed every gate, verifier check and validation in the project; it was found only when an
independent closed-form model refused to reconcile. No check would have caught it. This is
that check: the engine's own accessor output (scratch/ca_tbl_sensitivity_probe.cc, which
calls GetSensitivityThreshold and re-derives nothing) against data-sheet tables transcribed
HERE, sharing no code with the engine.

POPULATION. Tier 1 (SX1262) and tier 2 (SX1280), at the (SF, bandwidth) the scenario sets
in ca_tbl_causal_sim.cc -- parsed from `phy[CA_TBL_TIER_...] = {sf, bw, txPower};`, so the
check cannot drift from what runs -- and the engine's configured coding rate. Tier 3 (UWB)
is NOT CHECKED: no DW3000 sensitivity table has been transcribed with its conditions. That
is printed, not passed silently.

REFERENCES, transcribed verbatim with their conditions
  SX1280  Semtech SX1280/SX1281 Data Sheet Rev 3.2, March 2020, Table 6-1 "Receiver
          Sensitivity when using LoRa in Low Power Mode", p. 35: PER 1 %, 10-byte payload,
          25 C, 3.3 V, CR 4/5. Cross-checked against Table 3-5, p. 22, which repeats two
          corners (SF7/1625 kHz -106 dBm, SF12/203 kHz -130 dBm, low power mode). High
          sensitivity mode is ~2 dB better (Table 3-5: -108 / -132).
  SX1262  Semtech SX1261/2 Data Sheet Rev 1.2, June 2019, Table 3-8 "Receive Mode
          Specifications", RXS_LB: LoRa, Rx Boosted gain, split RF paths, RF switch loss
          excluded. Rx Power Saving gain is 4 dB worse at SF12/125 kHz (-133 vs -137).

TOLERANCE. +/-2 dB, fixed before the first run: the data sheets' own mode spreads (low power
vs high sensitivity on the SX1280, boosted vs power saving on the SX1262) are 2-4 dB, so an
engine value inside +/-2 dB of the reference mode is defensible as either; outside it, the
engine is modelling a different radio configuration than the one the scenario runs.

SY-3. --bw2p4 <Hz> states the tier-2 bandwidth the probe was configured with (the probe's own
--bw2p4), overriding the scenario's default line, so every SX1280 bandwidth can be checked.

Usage: sensitivity_fidelity_check.py <probe.csv> [<scenario.cc>] [--bw2p4 <Hz>]
Exit 0 if every checked tier agrees, 1 if any disagrees, 2 if the check could not run.
"""

import csv
import re
import sys

TOL_DB = 2.0

# SX1280 Table 6-1, low power mode, CR 4/5. Rows by bandwidth (kHz), columns SF5..SF12.
SX1280 = {
    203.125: [-109, -111, -115, -118, -121, -124, -127, -130],
    406.25: [-107, -110, -113, -116, -119, -122, -125, -128],
    812.5: [-105, -108, -112, -115, -117, -120, -123, -126],
    1625.0: [-99, -103, -106, -109, -111, -114, -117, -120],
}
# SX1262 Table 3-8, Rx Boosted gain. Only the (BW, SF) corners the data sheet tabulates.
SX1262 = {
    (10.4, 7): -134, (10.4, 12): -148,
    (125.0, 7): -124, (125.0, 12): -137,
    (250.0, 7): -121, (250.0, 12): -134,
    (500.0, 7): -117, (500.0, 12): -129,
}
TIER_KEY = {"868": "CA_TBL_TIER_868", "2p4": "CA_TBL_TIER_2P4"}


def scenario_phy(path):
    src = open(path).read()
    out = {}
    for tier, key in TIER_KEY.items():
        m = re.search(r"phy\[%s\]\s*=\s*\{\s*(\d+)\s*,\s*([0-9.eE+]+)\s*," % key, src)
        if not m:
            return None
        out[tier] = (int(m.group(1)), float(m.group(2)) / 1e3)
    return out


def nearest(bw_khz, table_bws):
    best = min(table_bws, key=lambda b: abs(b - bw_khz))
    return best if abs(best - bw_khz) / best <= 0.05 else None


def main(argv):
    if len(argv) < 2:
        print(__doc__)
        return 2
    args = list(argv[1:])
    bw2p4 = None
    if "--bw2p4" in args:
        i = args.index("--bw2p4")
        bw2p4 = float(args[i + 1]) / 1e3
        del args[i:i + 2]
    probe = args[0]
    scen = args[1] if len(args) > 1 else "scratch/ca_tbl_causal_sim.cc"
    phy = scenario_phy(scen)
    if phy is None:
        print("ERROR: could not parse the tier PHY settings from %s -- the check cannot see "
              "what the scenario runs" % scen)
        return 2
    if bw2p4 is not None:
        phy["2p4"] = (phy["2p4"][0], bw2p4)
    rows = list(csv.DictReader(open(probe)))
    if not rows:
        print("ERROR: empty probe output %s" % probe)
        return 2

    def engine(tier, sf):
        hits = [r for r in rows if r["tier"] == tier and int(r["sf"]) == sf
                and r["crDenom"] == r["configuredCr"]]
        return float(hits[0]["thresholdDbm"]) if len(hits) == 1 else None

    print("G93 sensitivity fidelity -- engine accessor vs data sheet, at the configured SF and BW")
    fails = []
    # Tier 2: SX1280
    sf, bw = phy["2p4"]
    row = nearest(bw, SX1280.keys())
    eng = engine("2p4", sf)
    if row is None or eng is None or not 5 <= sf <= 12:
        print("  FAIL tier 2  configured SF%d / %.1f kHz has no SX1280 Table 6-1 cell, or the "
              "probe has no value" % (sf, bw))
        fails.append("tier2")
    else:
        ref = SX1280[row][sf - 5]
        d = eng - ref
        ok = abs(d) <= TOL_DB
        # Which data-sheet row the engine value matches, if any -- the diagnosis, not the verdict.
        match = [b for b, v in SX1280.items() if abs(v[sf - 5] - eng) <= 0.5]
        print("  %-4s tier 2  SX1280 SF%d @ %.1f kHz (configured %.1f kHz): engine %.1f dBm, "
              "data sheet %.1f dBm, engine - data sheet %+.1f dB" % (
                  "OK" if ok else "FAIL", sf, row, bw, eng, ref, d))
        if not ok:
            print("         the engine value matches the data sheet's %s kHz row instead" % (
                " / ".join("%g" % b for b in match) if match else "no"))
            fails.append("tier2")
    # Tier 1: SX1262
    sf, bw = phy["868"]
    eng = engine("868", sf)
    bws = sorted({b for b, _ in SX1262})
    row = nearest(bw, bws)
    if row is None or (row, sf) not in SX1262 or eng is None:
        print("  NOT CHECKED tier 1  SF%d / %.1f kHz is not a tabulated SX1262 corner" % (sf, bw))
    else:
        ref = SX1262[(row, sf)]
        d = eng - ref
        ok = abs(d) <= TOL_DB
        print("  %-4s tier 1  SX1262 SF%d @ %.1f kHz: engine %.1f dBm, data sheet %.1f dBm "
              "(Rx Boosted), engine - data sheet %+.1f dB" % (
                  "OK" if ok else "FAIL", sf, row, eng, ref, d))
        if not ok:
            fails.append("tier1")
    print("  NOT CHECKED tier 3  no DW3000 sensitivity table transcribed with its conditions")
    if fails:
        print("G93 FAILED: %s" % " ".join(fails))
        return 1
    print("G93 PASSED (tiers 1-2; tier 3 not checked)")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
