# CA-TBL demo console

A recorded search-and-rescue run, replayed frame by frame. Every position, packet outcome
and tier change on the page is what the simulator produced — nothing is re-enacted.

**Open `console.html` in a browser. That is the whole of "using it".** It is a single
self-contained file with the run data inlined; no server, no network, no build step.

Live copy: <https://[hosted page]/code/artifact/a500156c-61b2-4677-a49c-6f8dc43db042>

---

## 1. What is in this folder

| file | |
|---|---|
| `console.html` | the built page, data inlined. 1.5 MB, open it directly |
| `run.json` | the packed run — everything the page reads. 1.4 MB |
| `arm_summary.csv` | the run's own summary row, so the numbers below are checkable |
| `run_id.txt` | provenance: commit, build profile, SHA-256 per output file |
| `run_demo.sh` | the exact simulator invocation that produced `run.json` |
| `build_console.sh` | run directory → packed JSON → single-file page |

The raw run (trace and event CSVs, tens of MB) is **not** included. `RULES.md` forbids
tracking `trace_*.csv`, and it is regenerable from `run_demo.sh`.

---

## 2. What the demo shows

An arc of five gateways at a trailhead. A casualty walks the trail, leaves it, and later
stops. A search party holds at base camp until dispatched, then converges.

| | |
|---|---|
| Casualty leaves the trail | 150 s |
| Party dispatched | 470 s |
| Searcher drift **before** dispatch | **0.0 m** |
| Searchers holding a casualty entry | **4 / 4**, mean error 1.9 m |
| Closest approach | **1–8 m** |
| Median fix error (cep50) | 18.2 m |
| 95th percentile (cep95) | 320 m |
| Energy | 16.5 J |

The searchers move toward the **estimate**, never the true position — nobody knows that —
so poor localisation shows up as distance walked rather than only as a number.

---

## 3. Why this deployment, and not the obvious ones

This is the part worth reading before quoting any number.

| placement | cep50 | why it is not a fair test |
|---|---|---|
| 3×3 grid | **28 m** | puts anchors in the **interior** of the search area. No SAR team carries a repeater into the middle; containment goes on access routes. It flatters the geometry enough to mislead |
| ridge (collinear) | **1932 m** | all anchors on a line, so the solve is near-singular off-axis. Honest about a trail repeater chain, and geometrically degenerate |
| **arc (used here)** | **18 m** | gateways follow a curving access route — a road or valley trail, units where a vehicle reaches. Real 2-D spread so the solve is conditioned, but every anchor on **one side**, so conditioning degrades as the subject leaves it in either direction |

`--gwArcCentreDeg=180` centres the arc on the trailhead the subject walks from. This was a
measurement, not a preference: centred **north**, the subject starts outside the arc's span
and on-trail error is 993 m against 3.9 m off-trail — the geometry got *better* when the
subject got lost, and only because the searchers had by then arrived as pseudo-anchors.

---

## 4. Read this before calling the alert an alarm

The party is dispatched by `--searchLaunchSigmaM=100`: deploy when the network can still
*hear* the subject but no longer *place* them. **On this deployment that threshold is not
principled**, and the honest position is recorded rather than hidden.

`BEHAVIOUR_ANOMALIES` A.10 established that contact loss is not an observable event here —
868 MHz reaches at least 4.5 km, so silence never comes and a silence-based alarm never
fires. The σ alarm was the answer to that. Measured on this run:

| `sqrt(traceSigma)` | p50 | p90 | p99 |
|---|---|---|---|
| on trail | 102.5 | 132.7 | 148.2 |
| off trail | 6.7 | 128.3 | 149.2 |

Sustained-60 s crossings: **150 m fires at t=720; 175 m and above never fire at all.** A
bound of 100 fires *before* the deviation unless the deviation is moved earlier, which is
what this run does. A bound of 150 sits **inside the noise band**.

**Why:** what σ measures on the trail is tier-1 range accuracy — RSSI inversion at roughly
50 % of range, so ~100 m on a 260 m link — **not geometry**. The deviation does not move it,
because the subject stays inside the arc's angular span: the same anchors at similar angles
either way. And off-trail σ looks *better* only because the searchers had arrived.

`--searchLaunchAtS` exists as the honest alternative — an overdue report, which is how a
real search actually starts. It holds the party correctly but `finderMode=2` does not
converge under it (807–1010 m instead of 1–8 m) and that is unexplained. See
`HANDOFF_2026_09_09.md` §6.1.

**Nothing from a demo run is a measurement.** Demo geometry is chosen for legibility.

---

## 5. Rebuilding it

Build the simulator first — see `SETUP_NEW_MACHINE.md`. Then, from the repository root:

```bash
./demo/run_demo.sh demo/out          # ~2 min, one arm
./demo/build_console.sh demo/out demo/console.html
```

To rebuild only the page — after editing `viz/console_template.html`, say — skip the
simulator and reuse the packed run:

```bash
python3 - <<'PY'
h = open("viz/console_template.html", encoding="utf-8").read()
j = open("demo/run.json", encoding="utf-8").read().strip()
assert h.count("/*__DATA__*/ null") == 1
open("demo/console.html", "w", encoding="utf-8").write(h.replace("/*__DATA__*/ null", j))
PY
```

**Preview before publishing.** A previous console shipped visibly broken because nobody
loaded it first:

```bash
python3 -m http.server 8899 --directory demo
# then open http://localhost:8899/console.html
```

---

## 6. How the page works

`scripts/pack_demo.py` reduces a run to one JSON. What is complete and what is sampled
matters, because a viewer will reasonably assume the picture is the run:

- **packets** — complete per cycle up to a drawn cap (40). Every cycle carries its true
  counts alongside, so the totals in the rail are the run's totals even when the map shows
  a sample of one cycle's frames.
- **tables** — sampled at `--vizTablePeriodS`. The period travels in the JSON so the
  console can say so rather than implying continuity.
- **tracks** — every cycle. Two floats per node; they cost nothing.

`viz/console_template.html` carries a `/*__DATA__*/ null` placeholder; the build replaces
it and **fails loudly** if the placeholder is missing or the data is still null.

Some defects fixed in that page, since each will otherwise be reintroduced:

- **`requestAnimationFrame` is scheduled FIRST inside the loop**, and init runs in
  `try/catch` with a visible error banner. Previously rAF sat at the *end* and init was
  unguarded, so a single throw killed the animation chain silently — the page rendered,
  Play did nothing, and nothing said why.
- **The map container owns the aspect ratio, not the canvas.** Putting `aspect-ratio` and
  `max-height` on the same element makes them fight; `max-height` won, the canvas went
  wide-and-short, and the telemetry rail was pushed below the fold.
- **The view fits to content, not to the configured box.** The world is ±2000 m and the
  action occupies a fraction of it; framing the whole box is what made the nodes look tiny.
  A scale bar is drawn because the frame is cropped.
- **Cycles are keyed from 1**, not 0. Opening on 0 showed an empty rail.
- **Searchers are drawn last.** Before dispatch they sit at base camp within metres of the
  casualty, so drawing them earlier hid all four under the casualty disc — precisely the
  state the demo exists to show.
