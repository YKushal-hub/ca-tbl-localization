# CA-TBL search console

A browser UI for building a layout and running the simulator (`scratch/ca_tbl_causal_sim.cc`)
on it. The UI only wires things up: every position, packet and outcome it shows is read from
the files the simulator writes. The simulator changes made for it are listed at the end.

## Run it

```bash
./ns3 configure --enable-examples --build-profile=optimized -- -DCMAKE_CXX_FLAGS="-include cmath"
./ns3 build
python3 ui/server.py            # then open http://localhost:8765
```

The `-include cmath` flag works around `ca-tbl-imu-ekf.h` using `std::isfinite` without
including `<cmath>`, which GCC 13 rejects. It changes no source file.

## What it does

- **Layout**: the map fills the left 85 %; the panel is the right 15 %. Drag gateways, one
  missing node and one last known position from the palette onto the map; drag to move,
  double-click to remove. The simulator's whole area fills the screen; scroll to zoom, drag
  the empty map to pan freely in any direction (up to 4 map half-sizes from the centre, so
  you can't get lost), zoom out to a quarter of the whole-area view, ⤢ to fit and follow the
  action again. Things can only be placed inside the simulator's area (the lighter square);
  for more room, raise **Map half-size**. Gateways become ids 101, 102, … in placement order,
  as the simulator assigns them.
  The **base camp** can be dragged too (palette or map), anywhere inside the area: it is sent
  as `--baseCampX/--baseCampY` (change 10). Typing a **Camp position** fraction instead puts it
  back at (f × half-size, f × half-size). Double-clicking it removes the camp (and the last
  known position and relays, which need one). Gateways are not moved with it.
- **Scenario 1** (one click): a 20 km × 20 km area, base camp at (3000, 3000), 3 gateways
  around it, the missing node 7.0 km away at (−1950, −1950) and held still, last seen at
  (1100, 800) — 2.9 km out. 14 search nodes + 3 relays (was 9; with 14, a searcher made contact
  in 2 of 5 seeds, seeds 9 and 11, against 0 of 5 with 9). 5 hours, seed 7 (the simulator's
  default).
- **Scenario 2** (one click): the same camp and gateways; the missing node at (−387, −387),
  4.5 km from the nearest gateway — 3 km beyond the gateways' reliable 868 MHz range (r80
  1.5 km, the simulator's `--probe`). No last known position. 25 search nodes + 3 relays.
  3 hours, seed 7.
- **Scenario 3** (one click): the same camp and gateways; the missing node at (1028, 1028),
  2.5 km from the nearest gateway — about 1 km beyond its reliable 868 MHz range. No last known
  position. 12 search nodes + 3 relays. 3 hours, seed 7. Measured (seeds 7–11, current search):
  found in 5 of 5, at 1 h 02 – 1 h 17 (UWB range at the find 1.23–1.85 m; estimate 0.6–2.9 m off).
 before Start is enabled (the simulator's own rule: 3 anchors for a
  2D fix). The server refuses fewer too.
- **Start** writes the layout to `ui/runs/<id>/gateways.csv` and runs `--arms=ca_tbl_full
  --gwLayout=file --gwLayoutFile=… --boxHalf --duration --seed --trace=true --vizEvents=true
  --vizTablePeriodS=2.5`, plus, from the panel:
  - **Base camp** → `--baseCampFrac=f` (camp at (f·half-size, f·half-size)); search nodes
    start there and sweep out from it;
  - **Last known position** → "the base camp" uses the simulator's base-camp prior (starting
    sigma sized from the true distance, the simulator's rule); "unknown" passes
    `--unknownStart=true` (map centre, map-sized sigma);
  - **Missing node** placed → `--subjectStartX/Y` and `--speed=0` (held still);
  - **Search nodes** N > 0 → `--nFinders=N` and THE SEARCH: whenever the party holds a location
    for the missing node, every searcher moves toward it and re-targets the moment a newer one
    arrives, each to its own point on that location's 1-sigma circle (so they spread out, not
    cluster). All of it is the simulator's own except party sharing: `--finderMode=1`
    (converge on the location, no sweep), `--finderDispersal=1.0` (each searcher's point on the
    circle, at its own bearing), `--finderStaleSteerBound=0` (an older location keeps steering
    until a newer one arrives), `--finderPartyShare` (a searcher steers on the freshest
    location ANY searcher holds, so every searcher moves; with a last known position, on that
    until the party has one), and the settings of `demo/run_demo.sh`: `--finderPseudoAnchor
    --sendHandshakeFrames --finderDigestPeriodS=20`. Plus `--finderSpeed=1.2` when the missing
    node is held still. And FOUND → HOLD: `--rangeReplyPosition` (a searcher's ranging reply
    carries where it is now, so the missing node's fix is not anchored to positions minutes
    old; the simulator's own option), `--uwbEvidenceFromNeighbours` (UWB may switch on when a
    searcher is ranged within 20 m, instead of needing 3 gateways there; the simulator's own
    option), and `--finderHoldM=15` (a searcher with a direct 2.4 GHz or UWB range of at most
    15 m to the missing node stops following broadcast locations, until no such range for 60 s),
    then ANCHORS AND VERIFIERS and the STOPPING CONDITION: `--finderContactM=2
    --finderAnchors=3 --stopOnFound=true` — the first 3 searchers to range the missing node within
    15 m stay put as anchors (fixed reference points, shown with a white square on the map); every
    other close searcher walks straight to the party's location with no stand-off; the search
    ENDS when a searcher's own direct UWB range to the missing node is 2 m or less (FOUND). An
    anchor with no close range for 60 s is released and another can take its place. Also
    `--meshLinks` from the **Mesh** switch at the top of the right panel (off = star, the default),
    and `--digestSummary=true --finderDigestSummary=true` from the **Multi-hop** switch below it
    (off by default; `--digestSummary` is the simulator's own option, `--finderDigestSummary` is
    the UI's, change 11). Off, a node answering a digest always sends
    its own position, so the missing node's location travels one hop only. On, it sends the
    entry the asker lacks or holds older, so a location can be passed on; searcher-to-searcher
    passing also needs the Mesh switch on, because with star searchers have no links to each
    other. The **Gateways ask** switch adds `--gatewayDigestPeriodS=20` (change 12), so locations
    are pushed to the gateways too; it needs Mesh and Multi-hop on.
    The **Party share** switch (on by default) passes `--finderPartyShare`; off, each searcher
    steers only on its own table entry, received over the radio, as in the original simulator
    and the paper. (Teams and sweep patterns
    were removed from the UI; their simulator flags remain, off by default.);
  - **…of which stay back** K > 0 → `--finderRelays=K` (needs a base camp, K < N);
  - **Last known position** placed → `--lkpX/--lkpY`. Needs a base camp.
- **Live playback**: the server tails the output CSVs while the simulator writes them, and
  sends a cycle only once every file has moved past it. The page glides markers between the
  2.5 s records (display only), at 0.25×–400×, with a timeline to scrub.
- **On the map**: paths so far; where each searcher was told the missing node is ("heading
  to"); the relay chain from the camp; this moment's packets (green delivered, red failed);
  the missing node's estimate with a ±1σ area (from the trace's `traceSigma`; not drawn when
  larger than the map). The missing node is hidden until the first advancement. Hover any
  node for its actual position; for the missing node also the estimate and error.
- **Radio strip** (above the timeline): which radio was in use across the run, from the
  trace's `activeTier` (grey 868 MHz, blue 2.4 GHz, pink UWB; each pixel shows the finest
  radio in its span). Click it to jump there. **2.4 GHz ◀ ▶** and **UWB ◀ ▶** jump to the
  previous or next switch onto that radio, landing 10 s before it. A period on a radio ends
  only after 30 s back on a coarser one, so a radio that flickers on and off counts as one
  period (the preset's 723 switches onto 2.4 GHz are 30 periods). UWB was never used in any
  run measured: the probe gives it about 5 m of range here, and converging searchers stand
  off at 8 m (the simulator's rule). With FOUND → HOLD it switches on in 9 of the 10 runs
  measured, once searchers arrive.
- **Advancement**: a tier escalation (the trace's `escalated` flag), or a search node's first
  FRESH location entry for the missing node. By default playback slows to 0.5× for 3 cycles
  with the nodes involved highlighted; at most one such alert per 60 s of simulated time and
  10 s of real time. "Pause" and "Keep going" are the alternatives.
- **Packet log**: click a node on the map or in the list for every packet it sent or received
  up to the playback time, filterable by delivered or failed (capture or collision).

## Known results and limits

**What a seed is.** The simulator draws every random quantity — radio shadowing and fading,
which packets collide, ranging and IMU noise, random start choices — from one random-number
sequence, and `--seed` picks where that sequence starts. The same seed with the same settings
gives exactly the same run (byte for byte); another seed is the same scenario with different
luck. So one run says little, and results below are given over five seeds (7–11). Seed 7 is
the simulator's default. The preset uses it; it was not picked for its outcome. (An earlier
version of the preset used seed 10 because that seed succeeded; that was replaced.)

- **The current search: anchors and verifiers, stopping at FOUND** (seeds 7–11, star mesh).
  When the search stopped (a searcher's own UWB range to the missing node ≤ 2 m):

  | seed | Scenario 1 (14 searchers) | Scenario 2 (25 searchers) |
  |---|---|---|
  | 7 | not found | 1 h 26 (UWB 1.11 m) |
  | 8 | 3 h 43 (1.47 m) | 1 h 18 (1.16 m) |
  | 9 | 1 h 49 (1.68 m) | 1 h 19 (1.32 m) |
  | 10 | 1 h 54 (1.23 m) | 2 h 36 (1.48 m) |
  | 11 | 1 h 49 (1.34 m) | not found |

  Found in 8 of 10, against 5 of 10 with every close searcher holding (below), and 20–80 min
  sooner where both found it. The two failures are the runs the party was led the wrong way early
  (no searcher ever came within 15 m, so no anchors formed). The estimate at the stop is 0.2–4 m
  off: the search ends as soon as a verifier reaches the person, before UWB has refined the fix.
- **Before: every close searcher holding** (FOUND → HOLD; seeds 7–11):

  | | located within 50 m | searchers reached it (< 50 m) | stayed with it to the end | UWB switched on |
  |---|---|---|---|---|
  | Scenario 1 (7 km, 9 searchers, 5 h) | 5 of 5 (1 h 15 – 2 h 35) | 5 of 5 | 5 of 5 (all 9 within 20 m, estimate 3 m off) | 5 of 5 |
  | Scenario 2 (4.5 km, 25 searchers, 3 h) | 4 of 5 (40 min – 2 h 14) | 4 of 5 | 4 of 5 (estimate 0–3 m off) | 4 of 5 |

  Scenario 2 seed 11 fails: the location the party was given early was badly wrong and no
  searcher ever got close, which holding cannot help.

  How it got here, each step measured on the same seeds:
  1. **Party sharing + spreading** (no teams): Scenario 2 located 4 of 5, but in 2 of them the
     searchers then walked away again; UWB never switched on.
  2. **+ `--rangeReplyPosition` and `--uwbEvidenceFromNeighbours`**: the missing node's fix had
     been anchored to searcher positions 30–60 min old; with fresh ones, Scenario 2 stayed in
     every run that reached it, and UWB switched on. Scenario 1 still lost it in 2 of 5.
  3. **+ FOUND → HOLD**: in Scenario 1's two failures the searchers near the missing node had
     bunched on one side of it; the estimate then slid that way, they followed, and it slid
     further. Holding still stops that loop: 5 of 5 stay.

  The hold rule assumes a searcher knows the range of a two-way exchange it took part in (the
  simulator records it on the missing node's side).
- **Earlier versions, for comparison** (since removed from the UI): teams of 3 walking to one
  point each (Scenario 2: located 1 of 5, no searcher within 150 m); sweeps — a cone beyond the
  last known position (Scenario 1: located 5 of 5, reached 3 of 5) and spirals from the camp
  (located 1–3 of 5) — whose shapes, spacings and thresholds were tuned on these layouts.
- **The simulator's own sweep** (no teams): never located the preset's missing node (8 runs).
- **Relays** follow the searchers' mean position: when the searchers head one way the chain
  reaches up to 4.5 km; when they spread all round the camp it stays near the camp.
- **Not done (by decision):** search-node placement, a minimum search-node count, and a
  coverage radius (the simulator decides reception per packet and defines none).

## Multi-hop test (Scenario 3, seeds 7–11; `ui/runs/multihop/`, `analyse.py`)

"Hops" = how many nodes the missing node's location passed through before a searcher held it
(the simulator's own hop count, from loctable). Before change 11, the Multi-hop switch never
reached the search nodes' radios, so their digests were empty and everything stayed 1 hop.

| Setting | Found | Searchers' copies passed on (2+ hops) | Reached a gateway | Longest wait for a newer location |
|---|---|---|---|---|
| Star (the default) | 5/5, 1 h 02 – 1 h 17 | 0 % | never | 18 – 31 min |
| Star + Multi-hop | 5/5, 1 h 05 – 1 h 38 | 0 % (no searcher links) | never | 18 – 31 min |
| Mesh | 3/5 (46 min, 1 h 14, 2 h 48) | 0 % | never | 10 – 117 min |
| Mesh + Multi-hop | 4/5 (50 min, 1 h 12, 2 h 07, 2 h 55) | 64 – 76 %, up to 7 hops | never | 17 – 79 min |
| Mesh + Multi-hop, no party share | 1/5 (1 h 44) | 0 – 37 % | never | 17 – 55 min |
| Mesh + Multi-hop + Gateways ask | 4/5 (46 min, 1 h 00, 1 h 06, 1 h 19) | 65 – 82 %, up to 7 hops | 0 – 3 times per run (1 hop) | 14 – 120 min |
| Mesh + Multi-hop + Gateways ask, no party share | 0/5 | 41 – 90 % | 0 – 2 times per run (1 – 3 hops) | 17 – 157 min |

- Multi-hop works: with the mesh on, the location is passed searcher → searcher → relay, up to
  7 hops. It improved the mesh from 3/5 to 4/5, but the mesh is still worse than star.
- Without the instant party share it found 1 of 5: real passing over the air does not yet
  replace the shortcut. Only 0 – 5 location pushes per run were passed on (a passed-on copy
  is then held, and counted, for a long time), so copies are few and often old.
- It never reaches a gateway, by design of the exchange: a location is pushed only in answer to
  a digest, and only searchers, relays and the missing node send digests (B2 seed 7: 14,628,
  4,075 and 77; gateways 0). Gateways answer but never ask, so nothing is pushed to them.
  Not one location push was delivered to a gateway in any run.
- With **Gateways ask** (change 12) the gateways do receive pushes, 60 – 400 per gateway per run,
  but almost all are other nodes' own positions: the missing node's location reached a gateway
  only 9 times in 10 runs. A digest lists at most 12 entries and an answer carries one entry,
  preferring one the asker does not list at all, so with ~19 entries in play the answers are
  spent on the searchers' entries first.

## The simulator changes (`scratch/ca_tbl_causal_sim.cc`)

Each is off by default. With none of the new flags given, runs were checked byte-identical to
the unmodified binary on trace, nodes, events and table output (with and without search nodes,
with and without a base camp).

1. `--subjectStartX` / `--subjectStartY`: call the walker's existing `SetPosition()` after the
   other start rules.
2. The searchers' converge step read the subject's speed (`--speed`) instead of the searchers'
   (`finderSpeedMps`), so `--speed=0` froze them; commit a660ac20 had separated the two speeds
   but missed this line. Equal whenever `--finderSpeed` is unset.
3. `--finderRelays=K` / `--finderRelaySpacingM` (default 1500, the 868 MHz r80 the `--probe`
   sweep measures): the last K finders do not search. Relay j (1..K) walks at the searchers'
   speed toward the point on the line from the camp to the searching party's mean position,
   at j/(K+1) of that distance but no more than j × spacing from the camp. Movement only;
   radios, ranging and the MAC are untouched. With relays on, the mode 2 sweep spreads its
   bearings over the searching finders only, so the relays' bearings are not left unswept.
4. `--finderGroupSize=G` (+ `--finderGroupSpacingM` 300, `--finderGroupTrackM` 4000,
   `--finderGroupFineM` 150, `--finderGroupHandoverM` no bound), mode 2 with a base camp:
   GROUP SEARCH. The searching finders form floor(n/G) teams. Each team walks an
   Archimedean spiral from the camp at walking speed, the teams' spirals interleaved so the
   tracks of all teams are `TrackM` apart, members side by side `SpacingM` apart across the
   track (a fix needs three ranges in one cycle). A team leaves the spiral for an entry whose
   sigma is below both the radius swept so far and `HandoverM`; leaving is latched, because
   the sweep places finders on their spiral point and a return would be a jump. After that,
   while the entry's sigma is above `FineM`, the team sweeps a spiral round the entry (tracks
   one team-width apart, out to the sigma, then again) instead of walking to the point; below
   it, the existing converge step. Note: the simulator's own mode 2 "spiral" is a straight
   ray, since its bearing term 0.4·v·t / (0.9·v·t) is a constant 0.44 rad.
5. `--lkpX` / `--lkpY` / `--lkpConeDeg` (default 45), needs `--baseCampFrac`: the subject's
   LAST KNOWN POSITION. The filter's base-camp prior is centred there instead of the camp
   (same sigma rule). With group search, each team walks from the camp to the LKP (members
   side by side) and then sweeps its slice of the cone beyond it (±cone about the bearing
   camp → LKP, split evenly between teams) in arcs that move out by `--finderGroupTrackM`
   each pass. No hand-over while walking out; after that the rules of item 4.
11. `--finderDigestSummary` (default off): with `--digestSummary` also on, the search nodes' and
   relays' radios carry and answer the summary too (one guarded line where their MAC is
   configured). Before, only the missing node, gateways and peers got it, so a finder's digest was
   always empty and every answer to it was the responder's own entry: nothing was ever passed on.
   It is a flag of its own because `--specProfile` turns `--digestSummary` on: the first version of
   this change had no such flag, and a null control against `main` showed it moved every preset run
   (seed 341, 5 km: tag 0 CEP95 57.1 -> 31.1 m). Checked: with the flag off, default and
   `--specProfile` runs (endgame seed 7 and 5 km seed 341, 600 s) are byte-identical to `main`.
12. `--gatewayDigestPeriodS` (default 0 = off): every gateway's 868 MHz radio sends a DIGEST every
   this many seconds, so nodes holding an entry the gateway lacks or holds older push it to the
   gateway (with `--digestSummary`). Without it gateways only answer and nothing is pushed to
   them. Sent at fixed offsets 0.7 / 1.3 / 1.9 s after the cycle step, no random draw: a frame
   sent at the step instant itself, where every node's digest and reply starts, was drowned by
   the 868 engine's interference model (stage 13; 0 of 18 receivers after the first). The
   searchers' own digests are sent at that instant (unchanged). Also prints one
   `UI GATEWAY-DIGEST` line per gateway at the end of the run. Checked: without it, star and
   mesh + `--digestSummary` runs are byte-identical to the previous binary (all 7 files).
10. `--baseCampX` / `--baseCampY` (unset = off), need `--baseCampFrac > 0` (which still switches
   the camp on) and a point inside the box; not with `--gwRidge`: the camp at this point
   instead of (f × boxHalf, f × boxHalf). Only the two lines that set the camp's x and y read
   it, so everything that uses the camp (searchers' and relays' start, the base-camp prior, the
   keep-out, the default gateway shift, the MS-2 layouts' camp) uses the new point. Checked:
   without it, all 7 output files byte-identical to the previous binary (Scenario 3 settings,
   seed 7, 900 s); with `--baseCampX=-2000 --baseCampY=4000` the searchers start at (-2000, 4000).
9. `--finderContactM` / `--finderContactSigmaM`, `--finderAnchors=K`, `--stopOnFound`, `--vizFixSet`
   (all off by default): the contact step (the nearest holder walks in once the party's location
   is within `ContactSigmaM`; FOUND at a direct UWB range ≤ `ContactM`, printed as one "UI FOUND"
   line); with `finderAnchors` K > 0 instead, the first K close finders stay put as anchors and
   every other close finder walks in ("UI ANCHOR" / "UI ANCHOR-RELEASED" lines); `stopOnFound`
   ends the cycle loop at FOUND (end-of-run reports still written; the outputs up to the stop are
   identical to the full run); `vizFixSet` writes, output only, the nodes each fix used. Off:
   runs byte-identical to before (checked on the earlier runs at each step).
8. `--finderHoldM` (default 0 = off) / `--finderHoldReleaseS` (default 60): FOUND → HOLD. The
   subject's own ranging set records, per finder id, when a DIRECT (not relayed) 2.4 GHz or UWB
   range of at most `finderHoldM` was measured; a searching finder with such a range in the last
   `finderHoldReleaseS` stays where it is (still published to the radios) instead of steering.
   Recording only on the subject's side; the fix is untouched. Off: runs byte-identical to
   before (checked on a plain run, a team run and the 25-searcher Scenario 2 run).
7. `--finderPartyShare` (default false): each searching finder steers on the freshest table
   entry any searching finder holds (its own included), and with `--lkpX/Y` on the last known
   position until the party holds one. Steering only. With the default, runs are byte-identical
   to before (checked on the plain runs, the cone sweep and the team runs).
6. `--finderGroupSweep=false` (default true): ESTIMATE-LED team search, no base camp needed.
   With `--finderGroupFineM` unbounded (the UI's "Move toward") the search round a coarse
   location never runs: a team walks to the location it holds. With `--lkpX/Y`, a team that
   holds no location over the air yet steers on the last known position (the BRIEFING: it is
   told before setting out, not by radio); the first location received replaces it. No
   wide sweep: a team heads for the location it holds; while that is coarser than
   `--finderGroupFineM` it walks there, then sweeps a spiral round it; a newer estimate more
   than max(500 m, the spiral's radius) away makes it drop that and head for the new one;
   finer than the bound, the converge step as before. TEAM SHARING: a finder steers on the
   freshest entry any member of its team holds (its own included), since team-mates walk
   side by side and tell each other. With the default (true), runs are byte-identical to
   before (checked on the plain runs and the cone preset).
