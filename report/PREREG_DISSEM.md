# PREREG DISSEM — beacons carry the sender's entry, judged on the requirement

**Registered before any cell runs** (rule 13). Written 2026-10-21. This commit adds this file,
`scripts/dissem_run.sh` and `scripts/dissem_analyse.py`, and nothing else. Outputs go to
`~/ns/catbl-work/dissem_2026-10-21/`, which does not exist at registration.

## Why

The preset's remaining shortfall is the five-node pool at 5 km (UWB-MODE: 0.831) and on the ring (0.909). Diagnosis
of the preset at 5 km, seed 6 (`da0202b4`'s message):
- **Finders steer on stale entries.** A finder 1.1 km from its subject steered on that subject's entry, frozen for
  810 s, while the subject was localised to 7 m. Parked on a stale point at the edge of 868 MHz reach, it never heard
  a fresher one.
- **An entry travels only in the digest round trip.** The subject's DISCOVERY reached the finder 181 times. The
  finder's DIGEST reached the subject 3 times.

`--beaconCarriesEntry` (plan item 24, N.32 widened to every node; G165) puts the sender's own entry on its DISCOVERY
beacon, +28 B on the air. **Only finders merge it.** A subject merging its neighbours' entries ranged off them in
the subject chain, and wrong-root estimates spread.

## Disclosure: an exploratory pilot was read before this registration

5 km, CA-TBL only, seeds 1–6, 1800 s, uncommitted builds; not citable. Five-node A(25 m):

| variant | mean |
|---|---|
| pairs (the preset) | 0.828 |
| `--beaconCarriesEntry`, finders merge | **0.886** (up on 6/6 seeds; steering entry age, median, 115 → 10 s) |
| `--beaconCarriesEntry`, everyone merges | 0.762 (wrong-root contagion) |
| `--digestOwnFirst` | 0.819 |
| `--finderTasking=need` | 0.701–0.846 by rule version |
| `--finderStaleSearch` | 0.742 |

In the endgame (3 seeds) every variant stayed at about 0.987. Only `--beaconCarriesEntry` is registered here. The
pilot set the predictions, so the seeds here are fresh.

## Cells

- **OFF:** the preset as it stands (`da0202b4`, DESIGN_DECISIONS 18–24).
- **ON:** OFF + `--beaconCarriesEntry=true`. It applies to every arm: it is a network property, so every arm's finders
  get it.

**Configurations:**
- A: 5 km.
- B: endgame, `--boxHalf=750 --finderMode=1`.
- C: the 8-gateway ring, `--nGateways=8 --gwLayout=legacy`.

All five arms; `--duration=1800 --trace=true --traceCovariance=true --reportAirtime=true`. Seeds 281–300: 120 cells,
one frozen binary checked before every cell.

## Predictions

| # | prediction | basis |
|---|---|---|
| D1 | ON raises CA-TBL five-node A(25 m) in A: paired interval above 0 | pilot +0.058, 6/6 seeds |
| D2 | ON moves CA-TBL tag-0 A(25 m) in A by under 0.02: interval inside [−0.02, 0.02] | pilot −0.005; tag 0 is not finder-limited |
| D3 | ON lowers CA-TBL five-node A(25 m) nowhere: every lower bound above −0.02 | pilot B level |
| D4 | ON's CA-TBL subject J ≤ 1.10 × OFF's in every configuration | beacon bytes are not billed in joules; pilot J −2 % at 5 km |
| D5 | ON more than halves the median age of the entry a CA-TBL finder steers on, in A | pilot 115 → 10 s |

## Adoption rule (registered)

`--beaconCarriesEntry` joins `kSpecProfile` if **D1, D3 and D4 all hold**. It is a protocol design choice, not a
correction, so it is adopted on the requirement. If it fails, it stays a default-off flag with its result recorded.

## Abort conditions

- not 120 cells, or a cell's `rc` not 0;
- a sidecar not `dirty 0`, or more than one source;
- a manifest mismatch;
- a seed outside 281–300;
- a cell whose variant or preset lines do not match;
- any window solver failure;
- an arm with fewer than 20 seeds.

## What this cannot establish

- **Real hardware.**
- **Billing:** the beacon's extra bytes are billed in airtime, duty cycle and collision exposure, not joules, as for
  every MAC beacon (the MACs carry no energy model).
