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
