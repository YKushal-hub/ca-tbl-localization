# ML-PATHLOSS-POS — Bonn 868 MHz, packet multilateration (scripts/pathloss_ml_bonn_pos.py)

21292 packets heard by >= 3 gateways (of 56540), 107 spatial blocks; ranges from ML-PATHLOSS's blocked 5-fold cross-validation. Gateways per packet: 3: 12149, 4: 6154, 5: 2337, 6: 597, 7: 51, 8: 4.

| position from | median error (m) | 68 % error (m) | 95 % error (m) | mean log10 error |
|---|---|---|---|---|
| C1 global LDPL inverted | 2860 | 4763 | 9763 | 3.3939 |
| C2 per-gateway LDPL inverted | 2296 | 3691 | 7678 | 3.3027 |
| C3 per-gateway inverse regression (RSSI) | 1706 | 2557 | 4967 | 3.1883 |
| C4 per-gateway inverse regression (RSSI, SNR) | 1365 | 2143 | 4941 | 3.1085 |
| ML gradient boosting (RSSI, SNR, gateway) | 1337 | 2175 | 4952 | 3.1061 |
| G0 centroid of receiving gateways | 1652 | 2277 | 4234 | 3.1902 |

Best classical (lowest mean log10 error, chosen on the test packets): **C4 per-gateway inverse regression (RSSI, SNR)**. Best inversion: **C2 per-gateway LDPL inverted**.

| contrast (mean log10 position error) | difference [95 %] | median error ratio | verdict |
|---|---|---|---|
| ML - best classical | -0.0024 [-0.0085, +0.0040] | 0.980 | **NO GAIN** |
| ML - best inversion | -0.1966 [-0.2413, -0.1436] | 0.582 | **GAIN** |
| C4 - best inversion | -0.1942 [-0.2373, -0.1426] | 0.594 | **GAIN** |

Bootstrap over 107 blocks, 10000 resamples. HELPS: interval below 0 and median ratio <= 0.90; SMALL: interval below 0, ratio > 0.90.
