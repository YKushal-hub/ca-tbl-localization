# CA-TBL Phase 8 — scope, from the code

Written 2026-08-04, after the decision-layer question closed negative (Part 5 and
Part 6 of `report/PCS_REDESIGN_RESULTS.md`). **Scope only. Nothing here was
implemented.**

Every claim below is a file path and a line number, or a grep result stated as such.
Where I could not establish something from the code I say so rather than estimate.

---

## 0. Why this document exists

The stated novelty is a four-part combination. Its current state:

| Component | State |
|---|---|
| Self-deployable mesh topology | **Absent.** `hopWeight = 0` (`ca-tbl-policy.cc:76`); `hopCount` is passed as a literal `0` (`ca_tbl_causal_sim.cc:2438`) |
| Hop-aware confidence degradation | **Absent**, and dependent on the above |
| Switching-point integrity validation | Built and measured. **No benefit under any form tested** — adverse under legacy PCS (+24.64 m RMSE, n=20), statistically neutral under v2 |
| Identity-scoped prioritisation | **Absent.** `QUERY`/`RESPONSE` never transmitted; one tag; `U(r)` scored offline only |

One part measured and negative, one absent, two absent-and-dependent. Further
sweeps of the decision layer will not change that arithmetic.

---

## 1. Q1 — the smallest change that puts a second tag in the scenario

### What is already multi-node

This is the good news and it is substantial. **The MAC and channel layers are
already multi-node.**

- Every anchor gets its own `CaTblMac`, configured and registered on the shared
  channel: `ca_tbl_causal_sim.cc:1973-2000`. At `gwGrid=3` that is 9+ MAC instances
  per tier already contending.
- The physics engines are keyed by node id and already expose
  `RegisterNode(uint32_t nodeId, Vector)` and
  `UpdateNodePosition(uint32_t nodeId, Vector)`
  (`lora-physics-engine.h:361,364`).
- `CaTblMacConfig` already carries `identityClass`, set to `CA_TBL_ID_TARGET` for
  the field node (`:1959-1960`) and `CA_TBL_ID_ANCHOR` for anchors (`:1985`).

So a second tag does **not** require building a multi-node MAC layer. It already
exists and is exercised.

### What is singular

The **field-node stack** is one of everything, all inside `RunArm()`:

| Object | Line | Notes |
|---|---|---|
| `const uint32_t nodeId = 1` | `:1799` | the tag's identity, a compile-time constant |
| `CaTblEnergyModel energy` | `:1832` | one energy budget, one duty governor |
| `CaTblPolicy policy` | `:1893` | one EKF, one tier state machine, one PCS |
| `CaTblMac* nodeMac[CA_TBL_TIER_COUNT]` | `:1951` | three MACs, all for node 1 |
| `NodeTrajectory` | `:744` | one facility tour |

The cycle loop then reads `policy`, `energy` and `nodeMac[...]` as free variables
throughout (e.g. `:2325`, `:2436`, `:2560`).

### The change

Promote the five singular objects into a per-tag struct and loop the cycle body over
tags. The engines, channel, anchors and provider are shared and need no change.

**Files touched:** `scratch/ca_tbl_causal_sim.cc` only. No `src/` change is required
for two tags — I found nothing in `CaTblPolicy`, `CaTblMac` or `CaTblEnergyModel`
that assumes a single instance.

**Does it perturb existing channel realisations?** **Yes, unavoidably, unless
gated.** `macRng` is a single shared stream seeded at `:1932`
(`SetStream(seed * 7919 + 104729)`), and every MAC draws from it (`:1969`, `:1999`,
`:2008`). A second tag issues additional draws and shifts the stream for everything
after it. It also adds transmissions that change contention outcomes for the
existing node — which is the *point* of the experiment, but means no result before
and after is comparable.

**Can it sit behind a flag?** Yes, and it must. `--nTags=1` must leave the draw
order byte-identical, which is testable the same way `--arms` was: run `--nTags=1`
and diff `arm_summary.csv` and all six traces against `results/baseline_v6`. If the
loop is structured so that a single tag executes exactly the current call sequence,
this holds by construction; if it does not hold, the refactor changed something and
the diff says so.

**What it unblocks:** multi-target arbitration. `arbitration_harness.py` already
scores `U(r)` offline; nothing in the scenario instantiates a second target for it
to arbitrate between.

**Effort:** moderate and mechanical. The risk is not the refactor, it is proving
`--nTags=1` byte-identity.

---

## 2. Q2 — the smallest change that makes `hopCount` non-zero at the policy

**This is a routing layer, not wiring. This is the answer that decides whether
hop-provenance is reachable in this project, and it is the discouraging one.**

### The plumbing genuinely does already exist

- `CaTblPolicy::Step(double dt, const std::vector<CaTblRangeMeasurement>&, uint32_t
  hopCount, ...)` — `ca-tbl-policy.cc:693`.
- `ComputePcs(..., uint32_t hopCount, ...)` applies the penalty on both the legacy
  and v2 paths — `:648-650` (v2), `:669-671` (legacy).
- `CaTblHeader::GetHopsTraversed()` is implemented as
  `m_initialHopLimit - m_hopLimit` — `ca-tbl-header.cc:413-419`.
- `CaTblMac` already floods with dedup and decrements the hop limit, and already
  forwards `hdr.GetHopsTraversed()` into the PHY (`ca-tbl-mac.cc:842`, `:909`,
  `:929`).
- The scenario passes a literal `0` — `ca_tbl_causal_sim.cc:2438`.

Seeing only the above, the obvious conclusion is "wire the field through and set
`hopWeight > 0`". **That conclusion is wrong.**

### Why the literal `0` is correct, not a stub

`CaTblMac::NoteNeighbour()` **rejects any frame that has traversed a hop**:

```cpp
// ONE HOP ONLY, and this guard is the whole correctness of the table.
if (hdr.GetHopsTraversed() != 0)
{
    return;
}
```
— `ca-tbl-mac.cc:337-356`

The documented reason is an energy defect, not a simplification: a relayed frame
carries the *originator* in `origin_id` (the relay does not rewrite it, because
flood dedup keys on the originator), so admitting it would insert a peer the node
may be unable to hear into the poll set, and every subsequent poll burns a full
`rxTimeoutS` against it — 0.93 mJ on UWB, about three times a *successful* exchange.
The comment states this reproduces the exact defect the neighbour table was added to
remove.

Consequently the poll set is one-hop **by construction**, `rangeTier()` ranges only
against directly-heard neighbours, and every ranging measurement genuinely has
`hopsTraversed == 0`. Passing `0` is reporting the truth.

### What would actually be required

Either:

1. **Relayed ranging** — a two-way ranging exchange through an intermediate node,
   with the range and its uncertainty composed across hops. That is a new protocol
   and a new error model, not a wiring change; or
2. **Admit multi-hop neighbours to the poll set** *and* add per-peer reachability
   tracking so unreachable peers are not polled. The MAC comment argues this
   reintroduces the removed defect unless the reachability half is done properly,
   and the reachability half is the hard part.

**Files touched:** `ca-tbl-mac.cc/.h` (neighbour admission, reachability),
`ca-tbl-policy.cc/.h` (composing multi-hop range uncertainty),
`ca_tbl_causal_sim.cc` (multi-node relays — so **this depends on Q1**).

**Does it perturb existing channel realisations?** Yes — new transmissions and new
RNG draws.

**Can it sit behind a flag?** The admission rule can (`--allowMultiHopNeighbours`),
but the honest answer is that a flag does not make this cheap. It is the largest of
the three items and it is gated behind Q1.

**Recommendation:** treat hop-provenance confidence as **out of reach for this
project** unless Q1 lands first and a relayed-ranging error model is written. Do not
set `hopWeight > 0` before then — a non-zero weight applied to a hop count that is
structurally always 0 changes nothing, and applied to an admitted-but-unreachable
peer it would degrade the score for an energy defect rather than a provenance one.

---

## 3. Q3 — putting `ESC_REQUEST`/`ESC_GRANT` and `QUERY`/`RESPONSE` on the wire

### Current state, verified by grep

`CA_TBL_MSG_ENTRY_PUSH`, `CA_TBL_MSG_ESC_REQUEST`, `CA_TBL_MSG_ESC_GRANT`,
`CA_TBL_MSG_QUERY` and `CA_TBL_MSG_RESPONSE` appear **only** in
`ca-tbl-header.cc` — in the switch statements that give each type its name
(`:27-37`), its ACK policy (`:52`), its access category (`:105-114`) and its payload
size (`:121`, `:162`). They are **never constructed and never sent** anywhere in
`src/` or `scratch/`.

What is actually transmitted: `CaTblMac::StartHandshake()`
(`ca-tbl-mac.cc:977-1010`) sends `DISCOVERY` then `DIGEST` on the stationary path,
or a single `TRIGGERED_UPDATE` on the receding path. `ACK` is referenced in the MAC.

Two structural blockers, and the second is the one that is easy to miss:

1. **Nothing sets mobility state**, so `m_mobility` is never `CA_TBL_MOB_RECEDING`
   and the receding branch at `:988` is dead. Claim 4 is exercised on its stationary
   path only.
2. **There is no receive-side dispatch on message type.** The MAC counts and charges
   per type (`Counters(f.hdr.GetMsgType())`, e.g. `:465`, `:498`, `:528`) but
   implements no request→response semantics. `ENTRY_PUSH` is described in the
   `StartHandshake` comment as arriving "as a reply" — no code generates that reply.
   So `ENTRY_PUSH` is blocked by missing reply logic *in addition to* needing a peer.

`ESC_REQUEST`/`ESC_GRANT` are **billed but never sent**: the escalation handshake is
charged through `CaTblEnergyModel::AccountEscalationHandshake()` as airtime and
energy, without frames existing. Part 5's duty-cycle findings are therefore about a
correctly-sized *accounting* of a handshake, not about one that contended for the
channel.

### The change

- A `HandleReceive` dispatch keyed on `GetMsgType()`, with reply logic per type.
- `ESC_REQUEST`/`ESC_GRANT`: replace the accounting call with real frames, and give
  anchors grant/deny logic. This makes the handshake subject to contention, CAD
  and the duty governor **as frames** rather than as a charge.
- `QUERY`/`RESPONSE`: an identity-scoped flood plus proxy-response state. Needs
  `identityClass` filtering, which already exists in `CaTblMacConfig`, and more than
  one target to scope *between*, so it **depends on Q1**.

**Files touched:** `ca-tbl-mac.cc/.h` primarily; `ca_tbl_causal_sim.cc` at the
handshake call site (`:2560-2563`); `ca-tbl-header.cc` needs no change — sizes,
access categories and ACK policies are already defined for all nine types.

**Does it perturb existing channel realisations?** Yes for `ESC_REQUEST`/`GRANT` —
real frames contend where a charge did not, so every escalating arm changes.

**Can it sit behind a flag?** Yes, and cleanly: `--sendHandshakeFrames=false` keeps
the current accounting path and reproduces `baseline_v6`. This is the best-isolated
of the three items.

**What it is worth:** this is the only one of the three that directly tests a
*claimed* mechanism (claim 4's custom handshake, claim 5's packet taxonomy) rather
than building new capability. It is also the cheapest. If Phase 8 does one thing,
this is the one with the best ratio of claim-coverage to work.

---

## 4. Recommended order, and the honest caveat

1. **Q3's `ESC_REQUEST`/`ESC_GRANT` on the wire** — cheapest, best isolated by a
   flag, and the only item that tests an existing claim rather than adding
   capability.
2. **Q1, two tags** — moderate, mechanical, unblocks multi-target arbitration and is
   a prerequisite for the rest of Q3.
3. **Q2, hop provenance** — largest, gated behind Q1, and requires a relayed-ranging
   error model that does not exist. Consider descoping the hop-provenance claim
   rather than building this.

**Caveat, stated plainly:** none of this rehabilitates the decision layer. Claims 1
and 2 are closed negative at 20 seeds across three geometries, and PCS v2 lost to
both the literature baseline and to static 2.4 GHz provisioning. Phase 8 would be
building the *unbuilt* parts of the novelty claim, on top of a decision layer that
has been measured and does not pay for itself. That is a legitimate thing to do —
the mesh and packet-taxonomy claims have never been tested and might hold — but it
should be undertaken knowing that the switching-policy contribution is not going to
be rescued by it.
