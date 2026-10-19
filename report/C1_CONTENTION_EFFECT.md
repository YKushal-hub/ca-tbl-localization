# C1 verified — the handshake, once it contends, costs 41 % more collisions

**Written** 2026-09-05 from a paired seed-7 run at commit `ac41f6c`, flag off vs on.
Null control (`--sendHandshakeFrames=false`) reproduces `results/baseline_v7` byte-for-byte.

## What changed

| arm | escalations off → on | totalJ off → on |
|---|---|---|
| `static_868` | 0 → 0 | 13.597 → **13.597** |
| `static_2p4` | 0 → 0 | 18.136 → **18.136** |
| `static_uwb` | 0 → 0 | 650.309 → **650.309** |
| `fixed_threshold` | 180 → 180 | 24.395 → 24.856 |
| `ca_tbl_no_integrity` | 11 → **18** | 14.279 → 14.784 |
| `ca_tbl_full` | 220 → 216 | 26.644 → 27.347 |

At the MAC, on `ca_tbl_full`:

| counter | off | on | delta |
|---|---|---|---|
| `mac_tx` | 222 560 | 248 761 | **+26 201** |
| `mac_collisions` | 53 553 | 75 539 | **+21 986 (+41 %)** |
| `mac_deliveries` | 281 233 | 293 256 | +12 023 |

## The control is inside the result

**The three static arms are byte-identical across the flag.** They never escalate, so
they never issue a handshake, so no frame is added and nothing about them can move. That
is not a coincidence to note in passing — it is the internal control that says the change
is confined to arms that actually escalate, and that the flag is not perturbing the
scenario globally. `fixed_threshold` and both `ca_tbl` arms move; the statics do not.

## What it means

The escalation handshake has always been **billed** at the right size. What it has never
done is **contend**. Turning the frames on adds 26 201 transmissions and **41 % more
collisions** on the same channel, and shifts escalation counts on two of the three
escalating arms — `ca_tbl_no_integrity` from 11 to 18, `ca_tbl_full` from 220 to 216.

Every duty-cycle and contention finding in the tree was measured on the accounting path.
This says those findings describe a handshake that never competed for the medium, and that
when it does compete the collision load is materially higher. **That is a re-baseline, not
a regression** — nothing downstream of a contention or duty-cycle number should be compared
across this flag without regenerating it.

## What this does NOT establish

**One seed.** RULES.md rule 8: a single-seed difference is never reported as a result, and
arms consume the shared `macRng` stream at their own rate. The escalation-count and energy
deltas above are a smoke observation and need a paired multi-seed campaign before any of
them is quoted.

The counter deltas are on firmer ground — `mac_tx` rising by the number of frames added is
a mechanism witness rather than a statistical claim, and the run is deterministic. But the
*size* of the collision increase is still one realisation, and the 41 % figure should carry
a paired campaign behind it before it goes in a report.

**Nothing about accuracy or energy as claims.** Those need the campaign too.

## Next

A paired 20-seed campaign across the flag, on the escalating arms, reporting mean of
per-seed differences with a sign test — the same estimator every other comparison in this
tree uses, so the numbers are comparable.

---

# The paired campaign — 20 seeds, and it corrects the figure above

**Added** 2026-09-05 from `results/c1_contention`, 40 cells, 2 flag states x 20 paired
seeds, six arms, **40/40 recording `dirty 0`**. The section above was one seed and said so;
this is the campaign it asked for.

## The control holds, on every seed

| arm | across the flag |
|---|---|
| `static_868` | **byte-identical** |
| `static_2p4` | **byte-identical** |
| `static_uwb` | **byte-identical** |

Zero differences in `cep50M`, `totalJ` or `mac_tx` across 20 paired seeds. The static arms
never escalate, so no handshake is issued and no frame is added — the flag reaches only the
arms that escalate, and the falsifier registered in `ac41f6c` did not fire.

## The effect, mean of per-seed differences (on - off), sign test beside it

| arm | mac_tx | mac_collisions | totalJ | cep50M |
|---|---|---|---|---|
| `ca_tbl_full` | +18 148 (+8.3%) 20+/0- | **+17 186 (+33.7%)** 20+/0- | +2.214 J (+8.2%) 20+/0- | -0.010 m (n.s.) |
| `fixed_threshold` | +14 097 (+6.4%) 20+/0- | **+13 962 (+27.2%)** 20+/0- | +0.584 J (+2.4%) 20+/0- | +0.024 m (n.s.) |
| `ca_tbl_no_integrity` | +1 352 (+0.6%) n.s. | +1 182 (+2.3%) p=0.0026 | +0.039 J (n.s.) | +0.072 m (n.s.) |

## What changed against the single-seed number

The seed-7 smoke run gave **+41 %** collisions. The paired 20-seed mean is **+33.7 %** on
`ca_tbl_full`. **The single seed overstated it by about a fifth**, which is the entire reason
rule 8 exists and the reason this campaign was run rather than the smoke figure quoted.
Every table and any downstream text should carry 33.7 %, not 41 %.

## Reading the three arms together

The effect scales with **how often an arm escalates**, which is what it should do if the
mechanism is the handshake and nothing else. `ca_tbl_full` escalates ~220 times per run and
gains 33.7 % more collisions; `fixed_threshold` escalates ~180 times and gains 27.2 %;
`ca_tbl_no_integrity` escalates ~14 times and gains 2.3 %. Three arms, one ordering, no
extra assumption.

**Accuracy does not move.** `cep50M` is within ±0.07 m on all three arms and not significant
on two of them. Contention costs energy and collisions, not metres, at this geometry — the
frames compete for the medium but the ranging that produces the fix still lands.

**Energy rises 8.2 % on `ca_tbl_full`, on 20 of 20 seeds, and the charge did not change.**
`AccountEscalationHandshake()` is called identically either way, so that increase is MAC
activity the frames caused — CAD, listening, retransmission — and not the handshake being
billed twice. That is the number the accounting path could never have produced, because a
charge does not occupy the medium.

## What this still does not establish

One geometry, one seed set, `pcsForm=legacy`. And the duty-cycle findings in the existing
report were all measured on the accounting path: this says they describe a handshake that
never contended, it does not re-derive them. **Anything downstream of a contention or
duty-cycle number needs regenerating under the flag before it is compared.**
