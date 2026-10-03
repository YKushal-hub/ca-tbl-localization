# CAMP-SPEC — how this directory was made

Registered in report/PREREG_CAMP_SPEC.md (`c9db895`, engine `cef0fc6`) before any cell ran.

```
2026-10-03T12:51:56Z  bash scripts/camp_spec_run.sh results/spec_2026-10-03   (detached: setsid nohup)
```

A first launch was stopped seconds after it began, before any cell finished, and its partial directory
removed: it was tied to a tool call with a two-hour limit, so the campaign was relaunched detached. No
finished cell was discarded.

Provenance of every cell (`run_id.txt`):
```
    260 source c9db895 dirty 0 profile optimized
```

Tracked: this file, CAMP_SPEC.md (the registered analysis, unchanged), raw_manifest.sha256, and every
cell's `arm_summary.csv` and `run_id.txt` (the results policy in .gitignore).

Outputs (sha256):
```
d6429c216457c712450fc05cb8fd1d56789d64a822ef2f2eb981a88e12bc6d20  CAMP_SPEC.md
47801a463d780f833955774715884725e71ccac7df7ff0ea76369b6ba12b887c  raw_manifest.sha256
```
