# SHA map — work/2026-11-21-collapse onto main's rewritten history (2026-11-22)

work/2026-11-21-collapse was built on main's history from before the attribution rewrite of 2026-11-14
(SHA_MAP.md). To merge it without bringing back the 22 trailer-carrying commits that rewrite removed, its
33 own commits were re-created with each pre-rewrite parent replaced by its rewritten twin from that table.
**Metadata only:** every commit keeps its tree, message, author, committer and dates; `git diff` between the old
and new tips is empty. Sidecars and documents that quote an old SHA resolve through this table.

| old | new | subject |
|---|---|---|
| `7f3fbd6bfa4b` | `ff05d3e2ebf8` | TII-1: --benefitLambda, the predicted-benefit hand-off rule  |
| `0a794076e7d5` | `70df20c66f9d` | pre-register PARETO-1, before any cell: the predicted-benefi |
| `b658cfc47905` | `7588d8336dab` | PARETO-1 result: NO-GO (no predicted-benefit setting meets 0 |
| `573a438a6564` | `b4348af04b33` | TII-2: --benefitSkip868Min3 (default off): skip the 868 MHz  |
| `a74404c590b4` | `234afe3451fd` | pre-register PARETO-2, before any cell: the TII-2 rule (868  |
| `fbb4330fc589` | `41f784229fa1` | TII-3: --benefitNisInflate, --benefitHeartbeat868N, --benefi |
| `b6c88fdef056` | `279fa49c8abc` | DD 27: the preset deployment is three gateways on a line, a  |
| `24f921fa0b67` | `f28ce8dc4fca` | Overdue alarm: --searchLaunchAtS dispatches the party on its |
| `61f1262037a1` | `8c8dae8495c6` | Realistic study: 20 seeded random gateway layouts (site RAND |
| `db0a1347af88` | `b7da4e9196bc` | Gates written against DD 26's deployment pin it back with PI |
| `e200c52c1bdd` | `b742a6dbdb04` | Mast-height correction of ranges (--rangeHeightCorrect, defa |
| `01495eac7f53` | `13cc19dfba5a` | REALISTIC-1 pre-registration (before any cell): realistic si |
| `fbb4dd3331f6` | `a4183ef3001b` | PARETO-2 result: NO-GO (static_2p4 feasible on these seeds a |
| `d0b2b3b4ddbf` | `029514f94c54` | Merge branch 'work/2026-11-20-tii-pareto' into work/2026-11- |
| `7ee07e33abac` | `78283b18eb18` | REALISTIC-2 pre-registration (before any cell and before REA |
| `eaee45b4b3c8` | `cc11a3f77483` | The work area's side of a gateway line (--acquireSideOfRoad, |
| `1f16986117ed` | `609bef15c418` | REALISTIC-2 and REALISTIC-3 results (summaries and provenanc |
| `fcfae4466fca` | `a8fdfb775bf0` | Search party under a 2.4 GHz-only design: three defects behi |
| `51f16e329f3a` | `5bbe8d6a1eea` | Merge branch 'work/2026-11-21-fair' into work/2026-11-21-tii |
| `8da121c4db30` | `61473f66e51f` | REALISTIC-4 result: with the search party fixed, ours reache |
| `70e2dac9da67` | `543757605b64` | Gate suite: G172-G174 closed with their own finish lines so  |
| `6e7025ad2289` | `e070447003d4` | Lost-and-confident crew filters: position-only 868 MHz updat |
| `e6dd49ebdbef` | `333e4c7dddef` | REALISTIC-5 result: with lost filters re-acquiring, G1, G2,  |
| `b0b9cbbd1986` | `69ae7b0b588a` | A different incident on every seed (--incidentRandom, DD 35, |
| `ec656a27b0eb` | `092218fdd482` | REALISTIC-6 result: with a different incident on every seed  |
| `0e6745a06b81` | `9e073654fbba` | Collapse extension: source log (verified primary values with |
| `b976e2a46151` | `579844add5fd` | Source log: NISTIR 6055 concrete slopes and Bor 2016 collisi |
| `4b5d2ef7830d` | `8a3be104fe8a` | Source log: ITU-R P.1238-13 floor loss and GPS SPS PS 2020 a |
| `e3bfaf4a5a7f` | `6d181ad1218b` | Source log: derived solid fraction, UWB rubble brackets and  |
| `41d739735a7b` | `a97b7683472b` | Collapse extension, World: rubble model with per-band loss a |
| `15e71bcd9d4a` | `7651fc0e1669` | Source log: rubble pile height (alpha = 0.15), cone spread,  |
| `6cfd632ebf48` | `d3f389185ec3` | Collapse extension, World: lowest-loss rubble path with Snel |
| `2d17deb0b844` | `84199163e0bf` | Collapse extension, System: tof / rss / joint (B1) estimator |
