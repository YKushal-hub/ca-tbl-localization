# PREREG ENV-SWEEP — the adopted preset in four environments, on fresh seeds

**Registered before any cell runs** (rule 13). Written 2026-11-14. This commit adds this file,
`scripts/envsweep_run.sh` and `scripts/envsweep_analyse.py`, and nothing else. Outputs go to
`~/ns/catbl-work/envsweep_2026-11-14/`, which does not exist at registration.

## Why

Every campaign since PRESET-5 ran in one environment, the dense forest (`--env=1`). The Technical Report and the
paper list that as a limitation. The engine already carries five environment classes (`--env`: 0 rural open,
1 rural forested, 2 suburban, 3 urban moderate, 4 urban dense), and `--envFacility` follows `--env`, so every band a
node carries sits in the same environment. This campaign runs the adopted preset, unchanged, in four of them. The
question is not "does the requirement hold everywhere" but **where tiering earns its place**: in which environment
does CA-TBL beat the single-band arms on availability, and where does it lose.

## Cells

- The preset exactly as in PRESET-5 NEW (`--specProfile=true`, 12 gateways, ring, 12 finders), **only `--env` changes**.
- **Environments:** E0 `--env=0` (rural open), E1 `--env=1` (rural forested; the PRESET-5 environment, re-run on
  these seeds so the contrast is paired), E2 `--env=2` (suburban), E3 `--env=3` (urban moderate).
- **Configurations:** A: 5 km. B: endgame, `--boxHalf=750 --finderMode=1`.
- All five arms. Seeds 361-380 (no earlier campaign): **160 cells** (4 x 2 x 20).
- **Binary:** the PRESET-5 frozen binary (`~/ns/catbl-work/preset5_2026-10-22/bin`, built from `8e92fe3a`), copied
  and verified against that campaign's `build.sha256`. Same binary as the headline numbers, so a difference between
  PRESET-5 and this campaign cannot come from the code. `--env=4` (urban dense) is not run: reserved as a stress case
  if E3 shows the requirement is already far out of reach.

## Predictions

| # | prediction | basis |
|---|---|---|
| P1 | CA-TBL five-node mean A(25 m) in A in E0 (open) is at least 0.95 | PRESET-5 forest 0.952; open is the easier channel |
| P2 | CA-TBL five-node mean A(25 m) in A in E3 (urban moderate) is below 0.95 | 300 s probe, seed 361: tag-0 CEP95 1135 m |
| P3 | CA-TBL five-node mean A(25 m) in A falls monotonically E0 >= E1 >= E2 >= E3 | propagation severity ordering |
| P4 | In every environment and configuration, CA-TBL beats `static_868` on five-node A(25 m): paired interval above 0 | PRESET-5 +0.617 |
| P5 | In at least one environment and configuration, CA-TBL beats `static_2p4` on five-node A(25 m): paired interval above 0 | the open question of the paper |
| P6 | In every environment and configuration, CA-TBL subject energy is below 10 % of `poucet_fusion`'s | PRESET-5: 1/34 and 1/15 |

P5 is the one the owner asked about: it names where tiering is a gain. It is registered as a prediction that may fail.

## Falsifiers and reporting

A prediction that fails is reported as failed. No environment, arm, seed or configuration is added or dropped after
the first cell. Single-seed differences are not reported (rule 8). Primary metric A(25 m) (`scripts/goal_report.py`),
five-node and tag 0; secondary CEP95 and subject energy. Paired 95 % bootstrap intervals over the 20 seeds.

## Abort conditions

- not 160 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch before any cell;
- a seed outside 361-380;
- a cell whose log does not show the preset lines of PRESET-5 (UWB mode 850k, ekf5 window smoother, magnetometer);
- a window solver failure;
- an arm with fewer than 20 seeds in any environment/configuration.
