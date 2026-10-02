# GEO-01 stage 1 — results of the geometric screen

**Registered in `report/PREREG_GEO01_STAGE1.md` (`cc058eb`) before the screen ran.** Run
on 2026-10-02 from a clean tree at `cc058eb` (engine `759fe48`). The outputs, the exact
commands and the SHA-256 of every output are in `results/geo01_stage1_2026-10-02/`; the
probe's `run_id.txt` reads `dirty 0`. No amendment was made after the registration.

**What this is and is not.** A model's prediction of tier-1 coverage over the spec's
10 km square, built from the build's own PDR and range-error curves with independent links
(the registration's §1). It is a screen that decides what stage 2 simulates. It is **not**
a measurement of the system, and no claim in the report rests on it.

## 1. The pruned set (the registered rule, §4)

Per N, the two realistic shapes with the highest mean P(≥3) over the area (ties within
0.005 broken by the lower median predicted CEP50):

| N | first | second |
|---|---|---|
| 3 | star (0.106) | cluster (0.096) |
| 4 | arc 90° (0.243) | star (0.221) |
| 5 | arc 90° (0.372) | arc 140° (0.367) |
| 6 | arc 180° (0.489) | arc 240° (0.488) |
| 8 | ring (0.709) | arc 240° (0.699) |

At N = 3, arc 90° (0.097) and cluster (0.096) sit within 0.005. The tie goes to the lower
median predicted CEP50: the cluster has one (1 298 m), and arc 90° has none, because no
lattice point reaches P(≥3) ≥ 0.5.

These ten go to stage 2 (report/PREREG_GEO01.md, P5, on the P4 build).

## 2. The predictions

- **P1 — held.** At N = 3 the largest area with P(≥3) ≥ 0.95 is 0.003, from the realistic
  optimiser, and the largest mean P(≥3) is 0.106, from star. Both are far below 0.5.
- **P2 — FAILED at N = 3, held at N = 4, 5, 6 and 8.** Star (0.106) beats the best ring,
  arc or perimeter shape (arc 90°, 0.097) at N = 3. From N = 4 the best arc or ring leads
  star by 0.022–0.083 and cluster by 0.056–0.234. With three gateways over 10 km, a
  compact geometry still out-covers a spread one, because P(≥3) needs all three in range.
  The registration names independence as the assumption that flatters compact layouts,
  so this failure is the screen's least robust call.
- **P3 — held.** The unrestricted optimiser beats the best realistic shape at every N:
  0.150 against 0.106 at N = 3, 0.297 against 0.243, 0.428 against 0.372, 0.542 against
  0.489, and 0.724 against 0.709 at N = 8.
- **P4 — held.** Within every shape family, mean P(≥3) rises with N.

## 3. What else the table says (descriptive, not a result)

- **Tier 1 alone does not cover the spec's area at these counts.** Even at N = 8 the best
  realistic shape covers mean P(≥3) = 0.71, and at most 21% of the area (arc 90°) reaches
  P(≥3) ≥ 0.95. Stage 2 runs with the four finders, sparse cases and relays (the P4 build)
  for this reason. A tier-1-only screen cannot pick the winner; it can only prune.
- **The realistic optimiser is not the realistic optimum.** It may only use the box's
  perimeter band (0.9 × boxHalf), and it scores below the arcs, which sit at
  0.6 × boxHalf. Arcs count as access routes (registration §3). So the two definitions of
  "a vehicle can reach it" disagree, and stage 2's prereg must pick one. The optimiser's
  own placements are compact rows on one edge, for the same reason as P2.
- **The predicted CEP50 is metres to kilometres** where any is defined (median 750–
  11 000 m). Tier-1 RSSI ranging cannot meet the spec's R ≤ 25 m at this scale, as the
  plan said (§3.4). Only UWB via finders can.

## 4. Limits, restated

Independent links (shadowing is correlated, so compact layouts are flattered). PDR and
range error are extrapolated log-linearly past the probe's 6 km. The predicted CEP is an
approximation from the Fisher information. The CEP is undefined where no point has
P(≥3) ≥ 0.5, shown as "inf".
