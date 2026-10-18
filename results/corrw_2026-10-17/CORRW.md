# CORRW results (exploratory)

| config | variant | arm | CEP95 (m) | CEP95 after 300 s (m) | coverage | fix share | tag-0 J |
|---|---|---|---|---|---|---|---|
| A | BASE | static_868 | 447.6 | 412.4 | nan | 0.787 | 8.16 |
| A | BASE | static_2p4 | 402.7 | 89.1 | nan | 0.046 | 13.38 |
| A | BASE | ca_tbl_full | 365.1 | 259.8 | nan | 0.851 | 19.18 |
| A | BASE | fixed_threshold | 361.7 | 239.6 | nan | 0.851 | 18.92 |
| A | CORR | static_868 | 376.7 | 338.4 | nan | 0.788 | 8.12 |
| A | CORR | static_2p4 | 402.7 | 89.1 | nan | 0.046 | 13.38 |
| A | CORR | ca_tbl_full | 401.9 | 293.0 | nan | 0.842 | 20.17 |
| A | CORR | fixed_threshold | 418.2 | 301.0 | nan | 0.836 | 19.50 |
| B | BASE | static_868 | 27.5 | 24.6 | nan | 0.964 | 9.34 |
| B | BASE | static_2p4 | 12.3 | 10.6 | nan | 0.833 | 8.98 |
| B | BASE | ca_tbl_full | 5.0 | 4.0 | nan | 0.996 | 27.17 |
| B | BASE | fixed_threshold | 4.6 | 3.9 | nan | 0.996 | 28.25 |
| B | CORR | static_868 | 23.7 | 11.9 | nan | 0.964 | 9.25 |
| B | CORR | static_2p4 | 12.3 | 10.6 | nan | 0.833 | 8.98 |
| B | CORR | ca_tbl_full | 4.5 | 3.9 | nan | 0.995 | 29.72 |
| B | CORR | fixed_threshold | 4.8 | 3.8 | nan | 0.996 | 31.85 |
| W | BASE | static_868 | 27.5 | 24.6 | nan | 0.964 | 9.34 |
| W | BASE | static_2p4 | 28.5 | 25.4 | nan | 0.763 | 8.76 |
| W | BASE | ca_tbl_full | 34.4 | 31.9 | nan | 0.995 | 83.05 |
| W | BASE | fixed_threshold | 35.7 | 33.8 | nan | 0.995 | 79.99 |
| W | CORR | static_868 | 23.7 | 11.9 | nan | 0.964 | 9.25 |
| W | CORR | static_2p4 | 28.5 | 25.4 | nan | 0.763 | 8.76 |
| W | CORR | ca_tbl_full | 23.8 | 18.2 | nan | 0.996 | 80.03 |
| W | CORR | fixed_threshold | 25.4 | 19.8 | nan | 0.996 | 78.34 |

W1 W ca_tbl_full dCEP95 -10.5 [-18.1, -3.5] m on 34.4
W2 B ca_tbl_full dCEP95 -0.5 [-1.8, +0.5] m on 5.0
W3 A ca_tbl_full dCEP95 +36.8 [-58.5, +167.1] m on 365.1
W4 A static_868 dCEP95 -71.0 [-147.6, -10.8] m on 447.6

## Predictions

- **W1**: held
- **W2**: held
- **W3**: not held
- **W4**: held

Adoption rule (W1, W2, W3 all hold): DO NOT ADOPT
