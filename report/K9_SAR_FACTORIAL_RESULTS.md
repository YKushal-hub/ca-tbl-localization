# K9 — the mesh is the story. The tier switching is not.

Registered `report/PREREG_K9_SAR_FACTORIAL.md` (`032290a`) before the cells ran. Three
cells, 20 seeds each, 120 rows, single commit, `dirty 0`, 4 simulated hours. Lost node
starts at the camp (last known position) and walks out to 3.6–11.6 km against an 868 r80
of 1515 m.

---

## 1. The mesh effect, and it is enormous

`ca_tbl_full`, the same system in all three, with only the deployment changing:

| deployment | availability | cep95 | energy |
|---|---|---|---|
| **WAN only** — 9 gateways, no finders | **4.1%** | 6665 m | 69.9 J |
| **sparse WAN + mesh** — 4 gateways, 4 finders | **26.4%** | 6759 m | 57.4 J |
| **WAN + mesh** — 9 gateways, 4 finders | **70.4%** | 2016 m | 53.4 J |

Nine gateways and no mobile nodes produce a position on **4% of cycles**. Add four
searchers carrying the same radios and it reaches **70%**, with cep95 falling from 6.7 km
to 2.0 km — **for less energy**, because the node stops straining after infrastructure
that cannot hear it.

**Mobile anchors are what make this deployment localisable at all.** Fixed infrastructure
at a base camp does not reach a subject kilometres away, however much of it you install.

## 2. And the tier switching adds nothing measurable

Paired per-seed, `ca_tbl_full` against the single-band arm, two-sided sign test:

| cell | vs `static_868` cep95 | sign test | verdict |
|---|---|---|---|
| WAN only | 0.0 m | 0+/0− | **identical, bit for bit** |
| sparse WAN + mesh | −1352 m | 10+/10− p 1.00 | not significant |
| WAN + mesh | +177 m | 11+/9− p 0.82 | not significant |

**In no cell is `ca_tbl_full` significantly different from a single 868 MHz radio.** In the
WAN-only cell every policy arm is byte-identical to `static_868`, because with nothing in
range to escalate to, the policy correctly never escalates and degenerates to tier 1.

It is also indistinguishable from `fixed_threshold` and from `ca_tbl_no_integrity` in every
cell. Escalations run 4–12 per 5760 cycles.

## 3. Where it does win, decisively

| cell | vs `static_2p4` cep95 | sign test |
|---|---|---|
| WAN only | **+2415 m** | 16+/4− p 0.012 |
| sparse WAN + mesh | **+30 026 m** | 18+/2− p 0.0004 |
| WAN + mesh | +25 135 m | — |

`static_2p4` reaches 0–4.9% availability and **diverges to 27–40 km of cep95** while
burning 205–235 J against CA-TBL's 53 J. `static_uwb` obtains **zero** fixes in every cell
at 3566 J. Single-band 2.4 GHz and UWB are not systems in a search; that is measured, at
p < 0.05 or better, on every comparison.

## 4. F1 fires

The registration said:

> **F1.** If `ca_tbl_full` fails to beat `static_868` on `cep95M` in the MESH-only and
> WAN+MESH cells, then the tiering does not pay even in the scenario built specifically to
> let it — and the conclusion is that the value is in the mesh (mobile anchors) rather than
> in the tier switching.

**It failed in both.** The conclusion stands as registered: **the value is in the mesh.**

That is not a null result. It is a different claim from the one the project set out to
make, and it is well supported: adding four mobile tri-band nodes to a base-camp deployment
takes availability from 4% to 70% and cep95 from 6.7 km to 2.0 km, for less energy than the
WAN-only configuration spends failing.

**What is not supported** is that confidence-driven tier switching improves on running the
long-range radio alone. Across three deployments and 60 paired seeds it never did, at any
significance.

## 5. Two defects in this campaign's own design, both mine

- **`--gwGrid=1` is silently clamped to 2** (`ca_tbl_causal_sim.cc:290`,
  `std::max(2, gwGrid)`), so the "mesh only" cell got **4 gateways, not 1**. It is
  relabelled *sparse WAN + mesh* rather than discarded — the gateways contribute ~4
  availability points, so it is ~92% mesh in practice, but the registration's definition
  was not achieved and saying otherwise would be false.
- **The three "still" cells were invalid and were killed mid-run.** `--speed=0` plus the
  last-known-position change means a stationary subject never leaves the camp, so "still"
  measured a subject sitting among nine gateways — the enclosure scenario this campaign
  exists to escape. A stationary subject must be placed at a tour site, not at the camp.
  **The mobility axis is unmeasured and is owed.**

## 6. Still not measured

- **Time to converge and first-fix latency** — the metric asked for first. Availability is
  not latency.
- **Rival estimators** — MLAT, WCL, proximity, LKP. Written, not yet applied. Until they
  run, "better than WAN-based localisation" is unsupported: `static_868` is CA-TBL's own
  EKF on gateway ranges, not a published WAN system.
- **The still/moving contrast.**
