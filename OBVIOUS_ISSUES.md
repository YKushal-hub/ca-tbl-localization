# Obvious issues to fix — repository hygiene audit, 2026-10-10

Audited at `4ca0dff9` (main, 2026-10-09) by static inspection of a fresh clone. No build
was attempted: the audit machine has no CMake, so nothing below rests on running the
simulator or the gate suite. Everything here is either visible in the tree or was found
with `python3 -m py_compile`, `bash -n`, `pyflakes`, `codespell` and `git grep`.

The engine itself is deliberately out of scope. `RULES.md` §4 requires a byte-for-byte
null-control run for any engine change, and that cannot be shown from this machine, so
engine faults are **listed, not fixed**. The fixes that were made touch only scripts,
documentation and packaging, and none of them can move a number in `results/`.

How to read this file: §1 is what the accompanying change fixed. §2 is what is left,
ordered by how much it costs the next person, with the suggested fix for each. §3 is
noise worth knowing about but not worth a ticket.

---

## 1. Fixed in this change

| # | fault | where | fix |
|---|---|---|---|
| 1.1 | **A tracked Python file did not parse.** The f-strings had lost their quotes and the file ended mid-statement (`print(fM{m}`), so it was a truncated paste, not a script. | `scratch/fast_rates.py` | Rewritten to do what its surviving lines intended: per-variant `FUSED` / `IMU_ONLY` / `LORA_ONLY` row fractions, repo-relative paths, a message instead of a crash on a missing CSV. |
| 1.2 | **37 files hardcoded one developer's home directory** (`/home/kushal/ns/ns-3-dev`). Every one of them failed on any other machine, including the WSL tree `SETUP_NEW_MACHINE.md` describes. | 10 × `scripts/sweeps/run_*.sh`, `scripts/diagnostics/run_all.sh`, 5 × `scratch/*.sh`, 18 × `scratch/*.py` (one only in its docstring), 2 × `scratch/python_handoff/*.py`, `average.py` | Shell runners `cd` to the repository root resolved from their own path. Python one-offs define `ROOT = Path(__file__).resolve().parents[1]` and build paths from it. `average.py` takes the CSV as an argument. `scripts/p10_plane.sh` is **untouched**: its sidecar pins it byte-for-byte as the script that produced `results/p10_plane_fix`. |
| 1.3 | **`scratch/run_full_pipeline.sh` changed directory one level too far.** It did `cd "$(dirname "$0")/../.."` from `scratch/`, which lands *above* the clone, then silently fell back to the hardcoded path. | `scratch/run_full_pipeline.sh:14` | `cd "$(dirname "$0")/.."`. |
| 1.4 | **`ml_pipeline/run_training.sh` only worked when invoked from inside `ml_pipeline/`.** It ran `python3 train.py` with no `cd`. | `ml_pipeline/run_training.sh` | `cd` to its own directory first. (The dataset it names is still not in the repository; see §2.4.) |
| 1.5 | **README said the gate suite was five gates and that G3 fails by design.** The suite is 88 gates and G3 passes (`RULES.md` §3 and the script header agree on the G3 point; README did not). | `README.md` | Paragraph rewritten with the count, the expected reds and the recount command. |
| 1.6 | **Three places still said 58 gates at `c79fffd`.** `grep -c '^finish ' scripts/catbl_gates.sh` gives 88 at `4ca0dff9` (G61–G90 landed after the count). | `scripts/catbl_gates.sh:2`, `SETUP_NEW_MACHINE.md:95`, and `RULES.md` §3/§7 (owner-only; see §2.2) | Script header and setup guide updated. `SETUP_NEW_MACHINE.md` no longer quotes a PASS figure, because the suite was not run here. |
| 1.7 | `/build/` was listed twice in `.gitignore`. | `.gitignore` | Duplicate removed, with a note. |
| 1.8 | The drift in `RULES.md` is recorded in `docs/TECH_DEBT.md` M.11, which is where the project puts it. | `docs/TECH_DEBT.md` | Dated addendum listing each stale line and its current value. |

---

## 2. Left for the owner, ordered by risk

### 2.1 Hardcoded debug print in all three physics engines — **highest priority**

```
src/lora-physics-engine/model/lora-physics-engine.cc:607
src/lora-physics-engine/model/lora-2p4-physics-engine.cc:610
src/lora-physics-engine/model/lora-uwb-physics-engine.cc:636
```

Each engine's `ProcessTransmission` contains, unconditionally:

```cpp
if (txNodeId == 10 && rxGatewayId == 5) {
    std::cout << "\n[ENGINE DEBUG] Node 9 GW 4 hidden losses:" ...
```

It is not behind `m_config.verboseLogging` (the block 380 lines later is), it fires on
every packet on that one link in every run, it writes to the simulator's stdout, and the
label does not even match the condition. It is a leftover from one debugging session on
one topology.

**Why it is not fixed here.** It is engine code. Removing it cannot change
`arm_summary.csv` or any `trace_*.csv` — it only writes to stdout — but `RULES.md` §4
says that is shown by a null-control diff, not asserted, and this machine cannot build.
The fix is a one-line deletion in each file plus the diff of one run's outputs against the
pre-change binary. Do it as its own commit so it can be reverted in isolation.

### 2.2 `RULES.md` line references are stale (owner-only file)

`docs/TECH_DEBT.md` M.11 says `RULES.md` changes are the owner's, so the drift is recorded
there (2026-10-10 addendum) rather than edited. In short, at `4ca0dff9`:

| `RULES.md` says | actual |
|---|---|
| 58 gates at `c79fffd` (§3, §7) | 88 |
| `cmd.AddValue` block at `:13190-15363` (§4, §8) | `:15028-17577`, 230 flags |
| `macRng` seeded at `:6275` (§4) | created `:6720`, seeded `:6725` |
| `hopWeight` at `ca-tbl-policy.cc:103` (§2) | `:107` |
| `BuildDeployment()` at `:197` (§8) | `:203` |
| terrain hardcodes 868 MHz at `lora-terrain-submodule.cc:69-70` (§5) | now on `TerrainParams` (`:72-74`), selected by `--terrainPerTierFreq`; 868 MHz is still the default |

The file already warns that its own line numbers go stale and tells the reader to re-grep.
That is fine for a reader; it is not fine for the landmine list in §5, where a stale
"hardcodes 868 MHz" entry describes a defect that has a flag now.

### 2.3 CI is upstream ns-3's and has not run green since August

`.github/workflows/per_commit.yml` is the unmodified ns-3 workflow: a full build of all of
ns-3 plus every `scratch/*.cc` on Ubuntu, two Windows toolchains and two macOS runners,
on **every push to every branch**, followed by upstream's `test.py`. It never runs
`scripts/catbl_gates.sh`, the twelve unit suites, or `verify_causal_sim.py`. The last three
runs (2026-08-08) ended `cancelled`; nothing since has run at all. The Codecov job uses the
retired bash uploader.

Suggested shape:

- drop the Windows and macOS matrix (the project builds on gcc-11/Linux and nowhere else,
  per `SETUP_NEW_MACHINE.md`);
- add a cheap lint job that would have caught §1.1 and §1.2: `python3 -m py_compile` over
  tracked `.py`, `bash -n` over tracked `.sh`, and `git grep -n '/home/'` over scripts;
- build with `--build-profile=optimized` and run the unit suites on pull requests; the
  25-minute gate suite on `main` only or on manual dispatch.

### 2.4 The ML pipeline points at data that is not in the repository

`ml_pipeline/run_training.sh` trains on `results/ml_training_dataset_test_500k.csv`;
`train.py` and `verify_ml.py` default to `results/ml_training_dataset.csv`; about fifteen
`scratch/*.py` read `results/ml_training_dataset.csv`, `results/smoke_test_50k.csv`,
`results/multimodal_M*_T*.csv`, `results/tdoa_processed_dataset_fixed.csv` or
`datasets/dataset_[C-E]*.csv`. None of these is tracked (`.gitignore` keeps only summaries
and sidecars under `results/`, and `datasets/` does not exist), and nothing says which
generator produces each one. Either add a `ml_pipeline/README.md` that names the
generator invocation per file, or move the pipeline out if it is no longer part of the
deliverable (the README does not mention it).

### 2.5 `scratch/` is a junk drawer, and every `.cc` in it is a build target

`scratch/` holds 184 files: the scenario and its twelve test suites, plus 66 one-off
`check_*.py`, `diag*.py`, `verify_*.py`, `patch_*.py`, `fix_*.py` and `print_*.py`
scripts, three `run_rng_trace*.sh` variants, a 2 MB `dataset_partial.csv`, a `task.md`, and
`dataset_5M_full_report.md`. ns-3's `scratch/CMakeLists.txt` compiles **every** `.cc` in
`scratch/` and its subdirectories into its own executable, so the 29 `check_*.cc`,
`diag_*.cc`, `test_*.cc` and generator programs are rebuilt on every `./ns3 build`.

Suggested: Python one-offs to `scripts/oneoff/` (or delete the ones whose input no longer
exists); `.cc` diagnostics that are not cited by a result to `tools/` or deleted
(subdirectories of `scratch/` are still built, so moving them within `scratch/` does not
help); `dataset_partial.csv` out of the tree.

### 2.6 Root-level clutter sidesteps the handoff rule

`.gitignore` and gate G8 keep `CA_TBL_HANDOFF_v*.md` untracked because "a working log is
never a deliverable". The root now carries five `HANDOFF_2026_MM_DD.md` files that match
neither the pattern nor the gate, plus `simstatus.md`, two audits, `average.py` and
`run_final_validation.py`. Either the rule is wrong and should be dropped, or the handoffs
belong in `docs/handoffs/` with the gate's pattern widened. Right now the rule exists and
is not enforced, which is exactly the state `.gitignore` says it was written to end.

### 2.7 README's headline finding predates the current result

`README.md` "What it found, as of 2026-08-05" reports the escalation architecture losing
to static provisioning. `HANDOFF_2026_10_05.md` and the FIXES-N26 commits report CA-TBL
with the four N.26 fixes beating Poucet and LoRa fusion but not adopted on 5 km energy.
Both are honest at their dates; the README is the one a visitor reads. It should say
which result is current and point at the handoff, or carry the date more prominently.

---

## 3. Minor, no ticket needed

- **`pyflakes`** over the project's Python (`scripts/`, `scratch/`, `analysis/`,
  `ml_pipeline/`, `src/catbl-propagation-study/`): 78 unused imports, 21 unused locals,
  22 f-strings with no placeholders, one syntax error (§1.1, fixed). None is a bug; they
  are noise that hides the next real one. Command:
  `python3 -m pyflakes scripts scratch analysis ml_pipeline src/catbl-propagation-study`.
- **`codespell`** flags `shs`, `abl`, `mis`, `gaus2`, `wit`, `empted` across the docs and
  the scenario. All are false positives (IMU-SHS, CA-TBL, pre-empted, a variable, a
  wavelet name). Add them to `utils/codespell-ignored-words` so the upstream pre-commit
  hook stays usable.
- **`scripts/catbl_gates.sh` needs bash ≥ 4** (`declare -A`). macOS ships 3.2. The
  `#!/usr/bin/env bash` shebang picks up a Homebrew bash if present; the setup guide
  should say so.
- **`demo/console.html` (1.5 MB) and `demo/run.json` (1.4 MB)** are tracked and
  regenerable from `demo/build_console.sh`; the demo README also links a [hosted page]
  artifact as the "live copy", which is not reachable by anyone else.
- **`SETUP_NEW_MACHINE.md`** is one person's WSL layout (`\\wsl.localhost\...\nethi`),
  which is fine for its stated audience but is the only setup document the repository
  has. The generic part (toolchain, build, verify) would stand on its own at the top.
- **156 of the shell scripts under `scripts/`** run without `set -e`. The gate runners say
  why in their headers; the sweep runners mostly do not. Not a fault, just worth knowing
  before trusting a `DONE` line.
