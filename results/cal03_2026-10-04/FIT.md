# CAL-03 fit (scripts/cal03_analyse.py fit)

Fix cycles read: 40833; excluded with no geometry (gdop 0, PCS 0): 2678.

FIT-R direction +(1, -0.1, 0.001778); clamped to >= 0 on [1]; weights (sum 3) (2.9947, 0.0000, 0.0053); rank agreement with t1 on the fit seeds 0.9931.

Held-out seeds 21-40 (main): rho FIT -0.2798, rho control -0.2807; A = control - FIT = -0.0008 [-0.0027, +0.0010] (95 % seed bootstrap) -> INDISTINGUISHABLE.

| threshold | tier | pooled fit cycles | positive | negative | status | value | LOSO F1 | held-out F1 |
|---|---|---|---|---|---|---|---|---|
| pcsThrUp868 | 868MHz | 1885 | 163 | 1722 | FITTED | 1.6424 | 0.188 | 0.153 |
| pcsThrUp2p4 | 2.4GHz | 17044 | 0 | 17044 | UNFITTED (legacy rate 0.980) | 1.3769 | nan | nan |
| pcsThrUpUwb | UWB | 724 | 0 | 724 | UNFITTED (legacy rate 1.000) | 1.6788 | nan | nan |
| pcsThrDown2p4 | 2.4GHz | 17044 | 12510 | 4534 | FITTED | 0.9206 | 0.954 | 0.966 |
| pcsThrDownUwb | UWB | 724 | 85 | 639 | FITTED | 0.5456 | 0.156 | 0.239 |

Down thresholds raised to keep hysteresis: [('pcsThrDown2p4', 0.9206166472388462, 1.5420978164146768), ('pcsThrDownUwb', 0.5456275358496551, 1.8802210695625314)].

Flags for the calibrated preset: --pcsW1=2.994675 --pcsW2=0.0 --pcsW3=0.005325 --pcsThrUp868=1.64235 --pcsThrUp2p4=1.376873 --pcsThrUpUwb=1.678769 --pcsThrDown2p4=1.542098 --pcsThrDownUwb=1.880221
