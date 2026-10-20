# INTEG2 results (exploratory)

Fitted integrityChi2 = 9303.83 (preset 1992.9).

## base (1992.9)

## E: the SY-5 fault matrix (tag 0; label = the fault window)

| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |
|---|---|---|---|---|---|---|---|
| antenna | downweight | ca_tbl_full | 0.037 | 0.010 | 27.4 | 201.4 | 10 |
| antenna | downweight | ca_tbl_no_integrity | nan | 0.000 | 86.7 | 200.2 | 10 |
| rb_common | downweight | ca_tbl_full | 0.275 | 0.071 | 67.1 | 242.1 | 10 |
| rb_common | downweight | ca_tbl_no_integrity | nan | 0.000 | 93.2 | 198.8 | 10 |
| rb_single | downweight | ca_tbl_full | 0.292 | 0.074 | 90.8 | 200.3 | 10 |
| rb_single | downweight | ca_tbl_no_integrity | nan | 0.000 | 114.0 | 199.0 | 10 |
| txdrop | downweight | ca_tbl_full | 0.126 | 0.029 | 30.9 | 192.4 | 10 |
| txdrop | downweight | ca_tbl_no_integrity | nan | 0.000 | 89.0 | 198.8 | 10 |

## fit

## E: the SY-5 fault matrix (tag 0; label = the fault window)

| fault | recovery | arm | integrity precision | integrity recall | CEP95 in window (m) | CEP95 outside (m) | seeds |
|---|---|---|---|---|---|---|---|
| antenna | downweight | ca_tbl_full | 0.041 | 0.003 | 85.9 | 199.5 | 10 |
| antenna | downweight | ca_tbl_no_integrity | nan | 0.000 | 86.7 | 200.2 | 10 |
| rb_common | downweight | ca_tbl_full | 0.290 | 0.023 | 91.4 | 198.8 | 10 |
| rb_common | downweight | ca_tbl_no_integrity | nan | 0.000 | 93.2 | 198.8 | 10 |
| rb_single | downweight | ca_tbl_full | 0.246 | 0.018 | 114.9 | 207.1 | 10 |
| rb_single | downweight | ca_tbl_no_integrity | nan | 0.000 | 114.0 | 199.0 | 10 |
| txdrop | downweight | ca_tbl_full | 0.095 | 0.006 | 83.8 | 198.8 | 10 |
| txdrop | downweight | ca_tbl_no_integrity | nan | 0.000 | 89.0 | 198.8 | 10 |


mean over faults, ca_tbl_full: precision 0.182 -> 0.168; recall 0.046 -> 0.012

## Predictions

- **I1**: not held
- **I2**: not held
- **I3**: held

Registered recommendation (I1-I3 all hold): do not recommend
