# CAL-02 fit (scripts/cal02_analyse.py fit)

Fix cycles read: 54097; excluded with no geometry (gdop 0, PCS 0): 537.

FIT-R direction -(1, -0.7499, -0.4217); clamped to >= 0 on [0]; weights (sum 3) (0.0000, 1.9202, 1.0798); rank agreement with t1 on the fit seeds -0.0717.

Held-out seeds 21-40 (main): rho FIT -0.1038, rho control 0.3151; A = control - FIT = +0.4189 [+0.2290, +0.5911] (95 % seed bootstrap) -> BETTER.

| threshold | tier | pooled fit cycles | positive | negative | status | value | LOSO F1 | held-out F1 |
|---|---|---|---|---|---|---|---|---|
| pcsThrUp868 | 868MHz | 1914 | 39 | 1875 | FITTED | 3.0000 | 0.039 | 0.045 |
| pcsThrUp2p4 | 2.4GHz | 23598 | 4 | 23594 | UNFITTED (legacy rate 0.969) | 2.0238 | nan | nan |
| pcsThrUpUwb | UWB | 1469 | 7 | 1462 | UNFITTED (legacy rate 0.999) | 1.5011 | nan | nan |
| pcsThrDown2p4 | 2.4GHz | 23598 | 23598 | 0 | UNFITTED (legacy rate 0.030) | 2.3611 | nan | nan |
| pcsThrDownUwb | UWB | 1469 | 1398 | 71 | FITTED | 0.5874 | 0.976 | 0.998 |

Down thresholds raised to keep hysteresis: [('pcsThrDownUwb', 0.5873873530732903, 1.68120317582028)].

Flags for the calibrated preset: --pcsW1=0.0 --pcsW2=1.920195 --pcsW3=1.079805 --pcsThrUp868=3.000016 --pcsThrUp2p4=2.023835 --pcsThrUpUwb=1.501074 --pcsThrDown2p4=2.3611 --pcsThrDownUwb=1.681203
