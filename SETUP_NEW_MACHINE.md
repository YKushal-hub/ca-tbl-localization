# CA-TBL — setting up on a new machine

Written 2026-09-09 for a fresh agent on a new PC with the `nethi` tree copied over.
Read `RULES.md` first — it is short and every rule in it was earned by a real defect.
Read this second. Read the newest `HANDOFF_*.md` third (check `git log` before trusting it).

---

## 1. Where the code actually lives

**The build runs inside WSL, not on Windows.** There are two clones and they do different
jobs:

| path | role |
|---|---|
| `\\wsl.localhost\Ubuntu-20.04\home\nethi\projects\ca-tbl-localization` | **the real one.** Builds, runs, all campaigns |
| `C:\projects\ca-tbl-localization` | Windows clone, used **only** to reach GitHub |

The Windows clone has a remote `wsl` pointing at the WSL path. GitHub is not reachable from
inside WSL on this setup, which is why the push route is what it is (§6).

`results/` is **1.3 GB** and `.git` is 88 MB. If the copy dropped `results/`, the code still
builds and every suite still passes — you lose the ability to re-analyse past campaigns,
not the ability to run new ones.

---

## 2. Toolchain, exact versions that work

```
gcc-11 / g++-11   11.5.0   (Ubuntu 20.04 PPA)
python3           3.8.10
cmake             4.3.0    at ~/.local/bin/cmake, NOT the apt one
ns-3              3.47
```

**gcc-11 is not optional and not the default on Ubuntu 20.04.** The default is gcc-9 and
the build fails on it. Every invocation carries `CC=gcc-11 CXX=g++-11`.

If gcc-11 is missing on the new box:

```bash
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt update && sudo apt install -y gcc-11 g++-11
```

If `cmake` is too old (ns-3.47 wants ≥ 3.13), the working one is a user-local install at
`~/.local/bin/cmake`. Check `which cmake` resolves there before blaming the build.

---

## 3. Build

```bash
cd ~/projects/ca-tbl-localization
CC=gcc-11 CXX=g++-11 ./ns3 configure --enable-examples --build-profile=optimized
CC=gcc-11 CXX=g++-11 ./ns3 build
```

**`--build-profile=optimized` is required, not a preference.** Every published number was
produced under it, and the default profile enables logging — roughly 10× slower on a
3600 s run. If a run that should take 90 s is taking 15 minutes, check this first.

A full build from clean is a few minutes. An incremental build after editing
`scratch/ca_tbl_causal_sim.cc` is ~40 s.

---

## 4. Verify the copy arrived intact

Run all twelve suites. **356 checks, all should pass** (counted at `c79fffd`, 2026-10-03).
Build them first: a stale suite binary has reported results for a tree it was not built
from (TECH_DEBT N.5).

```bash
S="ca_tbl_packet_validator ca_tbl_session_test ca_tbl_routing_test ca_tbl_lifecycle_test \
   ca_tbl_netlayer_test ca_tbl_relay_admission_test ca_tbl_sparse_anchor_test \
   ca_tbl_loocv_test ca_tbl_tdoa_test ca_tbl_nlos_test ca_tbl_integrity_test ca_tbl_imu_test"
CC=gcc-11 CXX=g++-11 ./ns3 build $S
mkdir -p scratchpad/validator
for t in $S; do
    a=""; [ "$t" = ca_tbl_packet_validator ] && a="--outdir=scratchpad/validator"
    echo -n "$t: "; ./build/scratch/ns3.47-$t-optimized $a 2>&1 | tail -1
done
```

Expected: 62, 32, 27, 16, 30, 22, 33, 9, 33, 41, 38, 13 — all with 0 failed.

Then the gate suite:

```bash
bash scripts/catbl_gates.sh
```

**Expect 53/58 with five reds** (at `c79fffd`), **and they are not your problem:**

- **G4, G5** — red from the environment-split correction. Recorded as true findings
  (`TECH_DEBT` K.6).
- **G6** — fail *by design* while pre-D1 evidence is still cited. Making it green by
  overwriting `results/{causal,det_a,det_b}` reintroduces the exact defect `e21666d`
  removed: a suite that rewrites the evidence it verifies. **Do not do this.**
- **G11** — red by design (`TECH_DEBT` J.1). Terrain is transcription-only.
- **G24** — FAIL BY FINDING: mechanisms with no population under `--specProfile`
  (`TECH_DEBT` N.15).

G3 passes: it separates RNG contamination from the integrity check's treatment effect on
causal order (`scripts/catbl_g3_ablation.py`). The whole suite takes about 25 minutes; run
it with `TMPDIR` outside `/tmp`, and compare `git status --short` before and after.

---

## 5. Traps that cost real time on the old machine

These are not hypothetical. Each one ate 20+ minutes.

1. **Shell quoting through `wsl.exe -- bash -lc "..."` eats `$variables` and backticks.**
   It silently substitutes empty strings, so a flag becomes `--searchLaunchSigmaM=` and the
   run does something different from what you asked. **Write scripts to a file and run
   `bash /tmp/x.sh`.** This bit me at least four times in one session.
2. **`--outdir` must already exist** or the sim exits 1 with `FATAL: cannot open
   run_id.txt`.
3. **`grep -c` exits non-zero on a count of 0**, silently breaking an `&&` chain. It has
   produced a PASS from a loop that never ran.
4. **`git add -p` is interactive and hangs the harness.**
5. **Never name a scratch file after a stdlib module.** A `/tmp/inspect.py` shadowed
   Python's `inspect` and broke an unrelated import chain.
6. **stdout and `arm_summary.csv` are block-buffered.** A run in progress shows an empty
   log and zero rows, then everything appears at once. That is not a stall.
7. **A single-arm run is ~6× faster**: `--arms=ca_tbl_full`. It is byte-identical to that
   arm's row in a full run because `RunArm()` resets the RNG stream index per arm. Use it
   for every demo iteration.

---

## 6. Pushing to GitHub

`main` is protected and GitHub is unreachable from WSL. The route:

```powershell
# from Windows, in C:\projects\ca-tbl-localization
git fetch wsl main
git branch -f <branch-name> FETCH_HEAD
git push -u origin <branch-name>
gh pr create --base main --head <branch-name> --title "..." --body-file <file>
```

**Never put `Co-Authored-By` or any assistant attribution on a commit.** `RULES.md` §2
forbids it and gate **G12** turns the suite red on it. The harness will keep instructing
otherwise; the repo rule wins.

---

## 7. Running the demo console

```bash
# 1. run (single arm is enough, and much faster)
mkdir -p scratchpad/demo
./build/scratch/ns3.47-ca_tbl_causal_sim-optimized --arms=ca_tbl_full --seed=7 \
  --duration=1800 --trace=true --vizEvents=true --vizTablePeriodS=25 \
  ... (see HANDOFF §4 for the full flag set) --outdir=scratchpad/demo

# 2. pack
python3 scripts/pack_demo.py scratchpad/demo ca_tbl_full 7 scratchpad/demo.json

# 3. inline into the page (the template carries a /*__DATA__*/ placeholder)
python3 - <<'PY'
h = open("viz/console_template.html", encoding="utf-8").read()
j = open("scratchpad/demo.json", encoding="utf-8").read().strip()
assert h.count("/*__DATA__*/ null") == 1
open("scratchpad/console.html", "w", encoding="utf-8").write(
    h.replace("/*__DATA__*/ null", j))
PY
```

Then publish `scratchpad/console.html` as an artifact.

**Test it on a local server before publishing.** The in-app browser cannot drive
`file://` pages, and a previous console shipped visibly broken:

```bash
python3 -m http.server 8899 --directory scratchpad
# then open http://localhost:8899/console.html
```

**Live artifact:** <https://claude.ai/code/artifact/a500156c-61b2-4677-a49c-6f8dc43db042>
It has a **share pin** — republishing does not change what viewers see until the pin is
moved from the artifact's share menu. An older handoff (v19) records a different artifact
URL (`51e1d3c6…`); **that one does not exist.**

---

## 8. Reading order for a fresh agent

1. `RULES.md` — the rules, and why each exists
2. this file
3. the newest `HANDOFF_*.md` — current state and what to do next (check `git log` first)
4. `docs/TECH_DEBT.md` §L — the definition of done, three bars with observable criteria
5. `docs/BEHAVIOUR_ANOMALIES.md` — the register of things nobody intended
6. `finalyearprojectcontext.docx` — the architecture spec the system is measured against

**Check `git log` before trusting any status document.** Two prior handoffs were stale on
handover: v14's priority-1 item was already built, and a two-phase finder search was
written before discovering `finderMode=2` already was one, better.

---

## 9. A cloud container (added 2026-10-04)

The tree was continued once in a fresh cloud container (Ubuntu 24.04, 4 cores). The
differences from §1-§6:

- **gcc-11 is in the archive.** `apt-get install -y gcc-11 g++-11` gives 11.5.0 with no PPA.
  The default compiler there is gcc-13. Build with `CC=gcc-11 CXX=g++-11` as everywhere.
- **The clone was shallow.** G13 then reports sidecar SHAs as unresolved (3 on 2026-10-04)
  because the commits are not in the clone. Run `git fetch --unshallow origin` before the
  first suite.
- **Set the git identity to the owner's before the first commit.** The container's default
  identity named an assistant. G12 rejects that, and `RULES.md` §2 forbids it.
