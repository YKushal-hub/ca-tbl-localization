# K10 — CA-TBL against published baselines

Registered `report/PREREG_K9_SAR_FACTORIAL.md`, extended with four rival estimators
(`51a2e5e`) and the last-known-position filter init (`5d20de9`). Six cells, 20 paired
seeds, 120 rows each, single commit per cell, `dirty 0`, 4 simulated hours.

Every baseline is computed from the **byte-identical measurement set** the EKF was handed
on the same cycle, so each comparison isolates the estimator with nothing else moving.

---

## 1. Moving subject — CA-TBL beats the published baselines

cep95, metres. Positive sign test = CA-TBL better.

| cell | CA-TBL | MLAT | WCL | PROX | LKP |
|---|---|---|---|---|---|
| WAN only | **6 665** | 13 762 ✱ | 8 700 ✱ | 8 941 ✱ | 8 413 ✱ |
| sparse WAN+mesh | 6 759 | 38 599 ✱ | **4 537** | **4 693** | 8 413 |
| WAN+mesh | **2 016** | 20 831 ✱ | 2 963 | 3 403 | 8 413 ✱ |

✱ = significant at α 0.05.

- **Against MLAT — snapshot multilateration, how most LoRaWAN localisation papers report —
  CA-TBL wins in all three cells**, 19+/1−, 18+/2− and 20+/0−, p ≤ 0.0004. Between 2× and
  10× better.
- **Against LKP ("no system")** it wins significantly in WAN-only and WAN+mesh.
- **Against WCL and PROX** it is better in two cells of three and worse in `sparse_move`,
  neither significantly. **The range-free baselines are competitive** and should not be
  dismissed — where anchors are few and ranging is poor, a weighted centroid is
  respectable.

**The honest qualifier stays.** Beating a memoryless snapshot solver is a low bar: any
filter does it. The strong comparison is `static_868` — WAN ranging with our own EKF — and
there CA-TBL is **statistically indistinguishable** in every moving cell (p 0.82–1.00).
The tiering still does not earn its complexity; what earns it is the filter, and the mesh.

## 2. Still subject — total failure, and worse than doing nothing

| cell | CA-TBL | LKP | sign test | availability |
|---|---|---|---|---|
| WAN only | 17 726 m | **11 625 m** | 1+/13− **p 0.0018** | 0.1% |
| sparse WAN+mesh | 16 590 m | **11 625 m** | 2+/11− **p 0.0225** | 0.0% |
| WAN+mesh | 17 971 m | **11 625 m** | — | 0.1% |

`sparse_still` also loses to MLAT at 3+/17−, p 0.0026.

**A stationary lost person is never found, and the system ends up significantly worse than
simply reporting where they were last seen.** `ca_tbl_full` is byte-identical to
`static_868` in every still cell (0+/0−): with nothing in range, nothing escalates.

### The deadlock, and it is a design gap rather than a bug

The finder logic converges on the casualty's *reported* position and, with no entry
received, **deliberately holds position** — the source says so, and says why: *"it makes
dissemination failure visible as searchers standing still rather than as a silent
success."* Good diagnostic; not a search.

So: the casualty is 11.6 km out and out of range, therefore it can tell nobody where it
is; the finders have no entry, therefore they do not move; nothing ever comes into range.
**A moving subject breaks the deadlock by wandering into someone. A still one cannot.**

The second half is that with no measurements at all, the filter's process noise walks the
estimate *away* from the last known position — which is why it ends up worse than a system
that just names the last known position and stops.

**There is no search behaviour in the architecture.** There is a *convergence* behaviour,
which needs an initial contact to bootstrap. A real search party sweeps outward from the
last known position precisely because it does not have that contact yet. `finderMode=2`
adds it.

## 3. What this campaign establishes

- **Against the published estimators, on a moving subject, CA-TBL is better** — decisively
  against snapshot multilateration, and better than no system at all.
- **Against a strong WAN baseline that shares our own filter, it is not** distinguishable.
- **Against a stationary subject it fails**, significantly, and the cause is a missing
  behaviour rather than a broken one.

The first of those is the claim the project can defend. The second is the limit of it. The
third is the next piece of work and it is scoped.
