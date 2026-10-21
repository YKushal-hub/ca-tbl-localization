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

---

# RESULT — after the registration above (`accfb6d2`)

**Run.** Launched 2026-10-21T12:50:08Z from `accfb6d2`, dirty 0. Done 15:01:16Z. 120 of 120 cells, every `rc` 0, one
source, no window solver failure; **problems none**. Scored once (`results/dissem_2026-10-21/DISSEM.md`).

| # | verdict | measured (CA-TBL, ON − OFF, paired, 20 seeds) |
|---|---|---|
| D1 | **FAILS** | 5 km five-node −0.017 [−0.086, 0.068] |
| D2 | FAILS (on width) | 5 km tag 0 +0.028 [−0.002, 0.073]; tag-0 CEP95 18.1 → 12.9 m |
| D3 | FAILS (on width) | five-node: 5 km −0.017, endgame +0.006 [−0.001, 0.017], ring −0.013 [−0.096, 0.056] |
| D4 | **FAILS** | subject J ratio 5 km 1.069, endgame **1.286**, ring 1.106 |
| D5 | HOLDS | the steering entry's median age at 5 km: 50 → 10 s |

## Adoption, by the registered rule

D1, D3 and D4 fail. **`--beaconCarriesEntry` does not join the preset.** It stays a default-off flag (G165).

## What it says

- **The mechanism works, and the requirement does not move.** Finders now steer on entries a fifth as old. CA-TBL's
  five-node pool at 5 km is unchanged (0.840 → 0.822, an interval spanning ±0.08).
- **The pilot misled, as rule 8 warns.** Six seeds gave +0.058, up on six of six. Twenty fresh seeds give −0.017.
  The pilot's seeds 2 and 6 carried most of it. The registration is what stopped it entering the preset.
- **Where the 5 km gap is.** In the pilot's worst seeds, with the flag on, the shortfall was one subject per run with
  no fix in 96–97 % of cycles, 4–6 km east of all three gateways (clustered on the west edge, x = −3 000 to −3 750 m).
  Finders reach it only by walking to an estimate up to a kilometre wrong. That is the brief's geometry: the
  8-gateway ring meets the requirement for tag 0 and comes closer for the pool.
- **Energy rises.** Fresher entries move finders more and keep subjects escalating. The endgame's subject J rises
  29 %, so the rule's energy guard alone would have refused it.

## Exploratory measurements recorded with it (not citable; seeds 1–6, 5 km, CA-TBL)

| variant | five-node A(25 m) |
|---|---|
| pairs (the preset) | 0.828 |
| `--finderTasking=need`: chasing sigma / coverage first / static pairs over subjects / with beacon entries | 0.701 / 0.781 / 0.846 / 0.784 |
| `--finderStaleSearch` | 0.742 |
| `--digestOwnFirst` | 0.819 |
| a 30 m cap on the finder dispersal offset (not committed) | 0.732 |

None is a candidate. A subject's reported sigma is small while a finder is near it, so need-driven rules pull
finders off the subjects they are serving. Moving finders bias every subject that ranges them, and a wide spread does
useful work at kilometre sigma.

## After the result: two more exploratory levers, and where the 5 km shortfall comes from (2026-10-21, not citable)

**Finder-side beacon localization.** A finder logs each beacon it hears from its subject as an 868 MHz RSSI ring
around its own GNSS position, fits the subject's position to the rings (log-range least squares), and steers on the
fit when the subject's own entry is uncertain or stale. 5 km, seeds 1–6, five-node A(25 m):
- pairs: 0.828;
- with the fit: 0.802;
- with the fit and beacon entries: 0.851.

The fit lands farther from the subject than the subject's own entry (median 0.8–1.9 km). At a kilometre an RSSI ring
carries about 70 % range error, the shadowing is correlated along the finder's walk, and only received frames are
sampled. **Not committed.**

**A 30 m cap on the finder spread:** 0.732. **Not committed.**

**Where the shortfall is, over this campaign's 20 OFF seeds** (100 node-runs, five-node A(25 m) 0.840):

| median distance to the nearest gateway | node-runs | A(25 m) | fix share |
|---|---|---|---|
| 1.5–3 km | 32 | 0.921 | 0.61 |
| 3–4.5 km | 64 | 0.827 | 0.67 |
| over 4.5 km | 4 | 0.391 | 0.46 |

- **No node is within 1.5 km of a gateway** in the brief's 5 km geometry, so every node depends on finders.
- **11 node-runs are nearly lost** (A(25 m) below 0.5), a median 4.25 km from any gateway, and carry two thirds of
  the shortfall.

Every finder-side lever measured tonight moves the pool by a few hundredths at most, some downward: tasking (four
rules), stale search, digest order, beacon entries (registered), a beacon fit, a spread cap.

**The requirement at 5 km is a supply limit.** Eight finders cannot keep two precise anchors within reach of four or
five subjects scattered over 5 km when no gateway reaches any of them. The 8-gateway ring, which puts
infrastructure within reach, meets it for tag 0 and comes within 0.04–0.05 for the pool.
