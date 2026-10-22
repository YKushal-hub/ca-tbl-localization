# Triggered-update check (static_868, endgame, seed 341, 1800 s; not a registered run)

Binary: the TDOA-TIMING-B manifest's (`build.sha256`). Command: `--specProfile=true --duration=1800 --trace=true --arms=static_868 --boxHalf=750 --finderMode=1 --seed=341`, plus `--tdoaPpsSigmaNs=0 --tdoaToaScale=0.01` for T0.

| config | triggered updates (all `large_correction`) | tag-0 airtime_868 (s) | tag-0 tx_868 (J) | cep95M |
|---|---|---|---|---|
| L | 8 (large_correction) | 45.7 | 6.79 | 34.1 |
| T0 | 544 (large_correction) | 176.6 | 26.22 | 0.9 |

Mechanism (scratch/ca_tbl_causal_sim.cc, MC-2 triggered update, on under --specProfile): a node pushes a 3-frame flooded update when its estimate moves more than k * sqrt(trace P) (k = 1). T0 tells the TDoA solver the stamps are near-exact, so P and the threshold collapse and ordinary estimate jitter fires the rule.
