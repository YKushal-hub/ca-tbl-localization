#!/usr/bin/env python3
"""Closed-form check of CaTblImuModel against the Monte-Carlo statistics that
scratch/ca_tbl_imu_validator.cc prints.

Independence is the point. The expected values here are derived from the PRINTED
parameters with no code shared with the model: different language, different
derivation, written from the error equations rather than from the implementation.
The project's six-decibel sensitivity defect passed every check that was written
beside the code it checked; it fell to an independent model that refused to
reconcile. That is the role this file plays for the IMU.

The model, as a reader should be able to verify it from first principles
-----------------------------------------------------------------------
Straight walk along +x, n cycles of length dt, distance d = v*dt per cycle. The
raw (unfiltered) dead-reckoned position is sum_k dhat_k * (cos th_k, sin th_k),
where th_k is the integrated-heading error after cycle k:

    th_k = delta + sum_{j=1..k} ( b0*dt + g_{j-1}*dt + a_j )        [+ s_g * turns]

    delta  alignment error, N(0, sd_align^2), per run
    b0     turn-on bias, N(0, sd_b0^2), per run
    g      Gauss-Markov bias, stationary N(0, sd_g^2), lag correlation beta^|i-j|,
           beta = exp(-dt/tau); g_{j-1} is the value held during cycle j
    a_j    angle random walk, N(0, N^2 dt), white
    dhat_k = (1 + s_d) d + e_k,  s_d ~ N(0, sd_sd^2) per run,
             e_k ~ N(0, sd_e^2 * L * d) per cycle (per-step errors summed)

so th is jointly Gaussian with covariance

    C_km = sd_align^2 + k m dt^2 sd_b0^2 + min(k,m) N^2 dt
           + dt^2 sd_g^2 sum_{i<k} sum_{j<m} beta^|i-j|

and for jointly Gaussian zero-mean th, EXACTLY (no small-angle step):

    E[sin th_k sin th_m] = exp(-(C_kk + C_mm)/2) sinh(C_km)
    E[cos th_k cos th_m] = exp(-(C_kk + C_mm)/2) cosh(C_km)
    E[cos th_k]          = exp(-C_kk / 2)

That exactness matters: the nominal turn-on bias reaches ~18 degrees in 120 s, where
a linearised variance would already be off by more than the test tolerance.

Exit status: 0 if every scored statistic agrees, 1 otherwise. Every failing line names
its case, so a fixture can assert WHICH term it broke.
"""

import math
import sys

DEG = math.pi / 180.0


def parse(lines):
    params, cases, stats, fixture = {}, {}, {}, None
    for raw in lines:
        f = raw.strip().split(",")
        if not f or not f[0]:
            continue
        if f[0] == "fixture":
            fixture = f[1]
        elif f[0] == "param":
            params.setdefault(f[1], {})[f[2]] = float(f[3])
        elif f[0] == "case":
            kv = dict(zip(f[2::2], f[3::2]))
            cases[f[1]] = {k: float(v) for k, v in kv.items()}
        elif f[0] == "stat":
            stats[(f[1], f[2])] = (float(f[3]), float(f[4]))
    return params, cases, stats, fixture


def heading_cov(p, n, dt):
    sa2 = (p["gyroNoiseDpsRtHz"] * DEG) ** 2 * dt
    sb2 = (p["gyroBiasInitDps"] * DEG) ** 2
    sg2 = (p["gyroBiasGmDps"] * DEG) ** 2
    sd2 = (p["headingInitDeg"] * DEG) ** 2
    tau = p["gyroBiasTauS"]
    beta = math.exp(-dt / tau) if tau > 0 else 0.0
    # S[k][m] = sum_{i<k} sum_{j<m} beta^|i-j|, built by 2-D prefix sums.
    S = [[0.0] * (n + 1) for _ in range(n + 1)]
    for k in range(1, n + 1):
        for m in range(1, n + 1):
            S[k][m] = S[k - 1][m] + S[k][m - 1] - S[k - 1][m - 1] + beta ** abs((k - 1) - (m - 1))
    C = [[0.0] * (n + 1) for _ in range(n + 1)]
    for k in range(1, n + 1):
        for m in range(1, n + 1):
            C[k][m] = sd2 + k * m * dt * dt * sb2 + min(k, m) * sa2 + dt * dt * sg2 * S[k][m]
    return C


def expected_straight(p, c):
    n, dt, v = int(c["n"]), c["dt"], c["speed"]
    d = v * dt
    C = heading_cov(p, n, dt)
    sdd2 = p["strideScaleErr"] ** 2
    se2 = p["strideStepErr"] ** 2 * p["strideLengthM"] * d
    ey = 0.0
    excc = 0.0
    mean_cos = 0.0
    for k in range(1, n + 1):
        mean_cos += d * math.exp(-C[k][k] / 2.0)
        for m in range(1, n + 1):
            edd = d * d * (1.0 + sdd2) + (se2 if k == m else 0.0)
            w = math.exp(-(C[k][k] + C[m][m]) / 2.0)
            ey += edd * w * math.sinh(C[k][m])
            excc += edd * w * math.cosh(C[k][m])
    ex_var = excc - mean_cos * mean_cos
    ex_mean = mean_cos - n * d
    return {
        "ey": (0.0, ey),
        "ex": (ex_mean, ex_var),
        "theta": (0.0, C[n][n]),
    }


def expected_turning(p, c):
    # Only the gyro scale factor is enabled on the turning path. The first cycle
    # defines the frame and integrates no turn, so n-1 turns of omega*dt are scaled.
    n, dt, w = int(c["n"]), c["dt"], c["turnRate"]
    return {"theta": (0.0, ((n - 1) * w * dt * p["gyroScaleErr"]) ** 2)}


def judge(name, metric, mc, sample, expect, k_var, failures):
    smean, svar = sample
    emean, evar = expect
    if evar == 0.0:
        # An exact zero is a control-flow claim: the term is structurally absent from
        # this metric. A tolerance would hide a leak, so none is given.
        ok = svar == 0.0 and smean == emean
        msg = "exact zero" if ok else "expected exactly 0, got var %.3e mean %.3e" % (svar, smean)
    else:
        ratio = svar / evar
        se_var = math.sqrt(2.0 / (mc - 1))
        se_mean = math.sqrt(evar / mc)
        ok_var = abs(ratio - 1.0) <= k_var * se_var
        ok_mean = abs(smean - emean) <= 5.0 * se_mean
        ok = ok_var and ok_mean
        msg = "var ratio %.4f (tol +/-%.4f)  mean %+.3e vs %+.3e (tol %.1e)" % (
            ratio, k_var * se_var, smean, emean, 5.0 * se_mean)
    print("  %-4s %-7s %-6s %s" % ("OK" if ok else "FAIL", name, metric, msg))
    if not ok:
        failures.append("%s/%s" % (name, metric))


def adev_check(path, p, c, failures):
    dt = c["dt"]
    y = [float(s) for s in open(path) if s.strip()]
    N = p["gyroNoiseDpsRtHz"] * DEG
    m = 1
    while m <= 64:
        K = len(y) // m
        avg = [sum(y[j * m:(j + 1) * m]) / m for j in range(K)]
        a2 = sum((avg[j + 1] - avg[j]) ** 2 for j in range(K - 1)) / (2.0 * (K - 1))
        tau = m * dt
        got = math.sqrt(a2)
        exp_ = N / math.sqrt(tau)
        tol = 5.0 / math.sqrt(2.0 * (K - 1))
        ok = abs(got / exp_ - 1.0) <= tol
        print("  %-4s adev    tau=%-6g ADEV/expected %.4f (tol +/-%.4f)" % (
            "OK" if ok else "FAIL", tau, got / exp_, tol))
        if not ok:
            failures.append("adev/tau=%g" % tau)
        m *= 2


def main(argv):
    if len(argv) < 2:
        print("usage: imu_closed_form_check.py <validator-stdout> [adev-stream]")
        return 2
    params, cases, stats, fixture = parse(open(argv[1]))
    print("IMU closed-form check  (fixture reported by validator: %s)" % fixture)
    if not cases:
        print("  ERROR no cases in %s -- the validator did not run" % argv[1])
        return 2
    failures = []
    for name, c in cases.items():
        if name == "adev":
            continue
        p = params[name]
        mc = int(c["mc"])
        if c["turnRate"] != 0.0:
            exp = expected_turning(p, c)
            scored = {"theta": 5.0}
        else:
            exp = expected_straight(p, c)
            # ex is a weighted sum of cosines, not Gaussian, when heading terms are on;
            # its variance has a larger sampling spread, so it is scored only where it
            # is Gaussian (stride-only cases) and loosely in the combined case.
            heading_on = any(p[k] != 0.0 for k in
                             ("gyroNoiseDpsRtHz", "gyroBiasInitDps", "gyroBiasGmDps",
                              "headingInitDeg"))
            scored = {"ey": 5.0, "theta": 5.0}
            if not heading_on:
                scored["ex"] = 5.0
            elif name == "all":
                scored["ex"] = 10.0
        for metric, kv in scored.items():
            if (name, metric) not in stats:
                print("  FAIL %-7s %-6s missing from validator output" % (name, metric))
                failures.append("%s/%s" % (name, metric))
                continue
            judge(name, metric, mc, stats[(name, metric)], exp[metric], kv, failures)
    if "adev" in cases:
        if len(argv) < 3:
            print("  FAIL adev    stream path not given")
            failures.append("adev/missing")
        else:
            adev_check(argv[2], params["adev"], cases["adev"], failures)
    if failures:
        print("IMU CHECK FAILED: %s" % " ".join(failures))
        return 1
    print("IMU CHECK PASSED")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
