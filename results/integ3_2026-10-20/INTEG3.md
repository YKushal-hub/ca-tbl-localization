# INTEG3 — seeds 181-200 (scripts/integ3_analyse.py)

Sources: 3de64c22.

| config | variant | arm | A(25 m) tag 0 | A(25 m) five-node | runs >= 0.95 (five-node) | CEP95 tag 0 (m) | subject J |
|---|---|---|---|---|---|---|---|
| A | P | ca_tbl_full | 0.669 | 0.476 | 0.00 | 459.1 | 18.87 |
| A | P | ca_tbl_no_integrity | 0.654 | 0.452 | 0.00 | 631.7 | 19.15 |
| A | P | fixed_threshold | 0.679 | 0.474 | 0.00 | 551.3 | 18.85 |
| A | P | poucet_fusion | 0.748 | 0.414 | 0.00 | 621.6 | 458.68 |
| B | P | ca_tbl_full | 0.994 | 0.918 | 0.50 | 3.1 | 20.01 |
| B | P | ca_tbl_no_integrity | 0.994 | 0.916 | 0.55 | 2.8 | 20.25 |
| B | P | fixed_threshold | 0.994 | 0.929 | 0.60 | 2.8 | 20.35 |
| B | P | poucet_fusion | 0.993 | 0.821 | 0.00 | 2.6 | 374.19 |
| C | P | ca_tbl_full | 0.885 | 0.657 | 0.00 | 87.6 | 16.34 |
| C | P | ca_tbl_no_integrity | 0.930 | 0.693 | 0.00 | 25.0 | 16.76 |
| C | P | fixed_threshold | 0.930 | 0.695 | 0.00 | 26.9 | 16.50 |
| C | P | poucet_fusion | 0.929 | 0.624 | 0.00 | 55.4 | 456.59 |

## Paired contrasts against ca_tbl_full

| config | arm − ca_tbl_full | Δ A(25 m) tag 0 | Δ A(25 m) five-node | Δ CEP95 (m) | Δ subject J |
|---|---|---|---|---|---|
| A | ca_tbl_no_integrity | -0.015 [-0.062, 0.016] | -0.024 [-0.056, 0.006] | 172.5 [2.3, 388.8] | 0.28 [-0.84, 1.56] |
| A | fixed_threshold | 0.010 [-0.057, 0.079] | -0.001 [-0.046, 0.045] | 92.2 [-28.7, 247.5] | -0.02 [-3.54, 3.12] |
| A | poucet_fusion | 0.080 [-0.095, 0.255] | -0.061 [-0.137, 0.015] | 162.5 [-346.0, 801.6] | 439.81 [435.19, 443.88] |
| B | ca_tbl_no_integrity | 0.000 [0.000, 0.000] | -0.002 [-0.007, 0.003] | -0.3 [-0.5, -0.1] | 0.25 [-0.72, 1.18] |
| B | fixed_threshold | 0.000 [0.000, 0.000] | 0.012 [-0.034, 0.063] | -0.3 [-0.6, 0.1] | 0.34 [-0.88, 1.64] |
| B | poucet_fusion | -0.001 [-0.004, 0.001] | -0.097 [-0.143, -0.050] | -0.5 [-0.9, -0.2] | 354.18 [339.86, 366.51] |
| C | ca_tbl_no_integrity | 0.045 [-0.006, 0.142] | 0.036 [0.003, 0.069] | -62.6 [-188.2, 0.4] | 0.42 [-0.67, 1.34] |
| C | fixed_threshold | 0.045 [-0.006, 0.142] | 0.037 [-0.005, 0.083] | -60.7 [-188.1, 5.9] | 0.15 [-1.37, 1.49] |
| C | poucet_fusion | 0.044 [-0.011, 0.146] | -0.034 [-0.146, 0.086] | -32.1 [-196.8, 100.4] | 440.25 [436.81, 443.50] |

## ca_tbl_no_integrity against the comparators

| config | comparator | Δ A(25 m) five-node | Δ A(25 m) tag 0 | Δ subject J |
|---|---|---|---|---|
| A | fixed_threshold | -0.022 [-0.069, 0.017] | -0.025 [-0.078, 0.015] | 0.30 [-1.98, 3.22] |
| A | poucet_fusion | 0.038 [-0.043, 0.115] | -0.094 [-0.273, 0.088] | -439.53 [-443.48, -435.16] |
| B | fixed_threshold | -0.014 [-0.065, 0.033] | 0.000 [0.000, 0.000] | -0.10 [-1.43, 1.24] |
| B | poucet_fusion | 0.095 [0.051, 0.141] | 0.001 [-0.001, 0.004] | -353.94 [-365.88, -339.88] |
| C | fixed_threshold | -0.002 [-0.055, 0.039] | 0.000 [0.000, 0.000] | 0.27 [-0.60, 1.14] |
| C | poucet_fusion | 0.069 [-0.048, 0.188] | 0.001 [-0.006, 0.011] | -439.83 [-442.91, -436.56] |

## Registered predictions

- I1 no-integrity raises five-node A(25 m) in A: FAILS (-0.024 [-0.056, 0.005])
- I2 no-integrity lowers five-node A(25 m) in neither B nor C (lower bounds > -0.02): HOLDS (B -0.002 [-0.007, 0.003]; C 0.036 [0.004, 0.070])
- I3 no-integrity level with fixed_threshold in A (interval contains 0): HOLDS (-0.022 [-0.068, 0.017])
- I4 no-integrity costs no material energy in A (upper bound < +1.0 J): FAILS (0.28 [-0.79, 1.55])

Window refusals in the two comparator arms (reported, not an abort): 0 in 120 arm-runs

Problems: 0 (none)
