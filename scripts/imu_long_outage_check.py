#!/usr/bin/env python3
"""G94 -- the IMU filter stays honest through a long outage.

G91's filter test exercises 50 s outages and could not see the defect report/IMU_PRIOR_RESULT.md
§3 measured: the EKF drops the second-order along-track shortening a heading error causes, and
is overconfident beyond ~2 minutes of outage even with a perfect channel (NEES 11.8 at 500 s,
178 at 1000 s). This scores the validator's site-dwell diagnostic -- a 90 s dwell with five
anchors at the filter's own R, then a 1000 s outage -- on two numbers:

  dwell NEES in [1.6, 2.6]       consistent while fixes arrive (the band G91 uses)
  end-of-outage NEES <= 3.0      one-sided: a conservative filter is safe, an overconfident one
                                 is what makes the integrity check reject a correct fix

Usage: imu_long_outage_check.py <validator-stdout>    exit 0 pass, 1 fail, 2 could not run
"""
import sys

def main(argv):
    if len(argv) != 2:
        print(__doc__)
        return 2
    vals = {}
    for line in open(argv[1]):
        f = line.strip().split(",")
        if f and f[0] == "dwelldiag":
            for k, v in zip(f[1::2], f[2::2]):
                try:
                    vals[k] = float(v)
                except ValueError:
                    pass
    if "neesDwellMean" not in vals or "neesEndOfOutageMean" not in vals:
        print("ERROR: no dwell diagnostic in %s -- the validator did not run it" % argv[1])
        return 2
    dwell, end = vals["neesDwellMean"], vals["neesEndOfOutageMean"]
    ok1 = 1.6 <= dwell <= 2.6
    ok2 = end <= 3.0
    print("  %-4s dwell NEES %.3f (band 1.6-2.6)" % ("OK" if ok1 else "FAIL", dwell))
    print("  %-4s end-of-outage NEES %.3f after %d cycles (<= 3.0)" % (
        "OK" if ok2 else "FAIL", end, int(vals.get("nTransit", 0))))
    if ok1 and ok2:
        print("G94 PASSED")
        return 0
    print("G94 FAILED: %s" % " ".join(n for n, ok in (("dwell", ok1), ("outage", ok2)) if not ok))
    return 1

if __name__ == "__main__":
    sys.exit(main(sys.argv))
