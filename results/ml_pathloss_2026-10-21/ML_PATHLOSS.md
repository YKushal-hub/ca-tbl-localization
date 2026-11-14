# ML-PATHLOSS — Bonn 868 MHz measurements (scripts/pathloss_ml_bonn.py)

131057 packets with d >= 10 m, 9 gateways, 128 spatial blocks of 1000 m, 5-fold cross-validation by block (no test block seen in training).

| model | median factor | 68 % factor | median abs error (m) | mean abs log10 error |
|---|---|---|---|---|
| C1 global LDPL inverted | x/÷2.69 | x/÷4.41 | 2576 | 0.5778 |
| C2 per-gateway LDPL inverted | x/÷1.90 | x/÷2.68 | 1469 | 0.4531 |
| C3 per-gateway inverse regression (RSSI) | x/÷1.48 | x/÷1.75 | 842 | 0.1968 |
| C4 per-gateway inverse regression (RSSI, SNR) | x/÷1.38 | x/÷1.61 | 704 | 0.1692 |
| ML gradient boosting (RSSI, SNR, gateway) | x/÷1.38 | x/÷1.61 | 707 | 0.1682 |

Best classical (lowest mean abs log10 error, chosen on the test predictions): **C4 per-gateway inverse regression (RSSI, SNR)**.

ML - best classical, mean abs log10 error: -0.0010 [-0.0031, +0.0009] (95 %, bootstrap over 128 blocks). 68 % factor ratio ML / best classical: 0.999.

**Verdict: NO GAIN** (HELPS: interval below 0 and ratio <= 0.90; SMALL: interval below 0, ratio > 0.90).

## By true distance (68 % factor, cross-validated by block)

| distance | packets | C1 | C2 | C3 | C4 | ML |
|---|---|---|---|---|---|---|
| 10-500 m | 2794 | 51.64 | 80.02 | 3.44 | 2.61 | 2.32 |
| 500-1000 m | 15225 | 11.44 | 5.06 | 2.42 | 1.91 | 1.90 |
| 1000-2000 m | 33261 | 6.35 | 2.77 | 1.64 | 1.57 | 1.63 |
| 2000- m | 79777 | 2.83 | 2.23 | 1.67 | 1.54 | 1.52 |

## Contrast (not the registered test): random-packet folds, which leak location

| model | 68 % factor |
|---|---|
| C1 global LDPL inverted | x/÷4.32 |
| C2 per-gateway LDPL inverted | x/÷2.60 |
| C3 per-gateway inverse regression (RSSI) | x/÷1.70 |
| C4 per-gateway inverse regression (RSSI, SNR) | x/÷1.56 |
| ML gradient boosting (RSSI, SNR, gateway) | x/÷1.54 |
