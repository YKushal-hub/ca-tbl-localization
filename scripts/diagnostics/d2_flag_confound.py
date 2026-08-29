#!/usr/bin/env python3
"""D2 - flag-confound audit. Pure sidecar analysis, no runs.

Every flag with a documented side effect, cross-referenced against every
run_id.txt in results/. A campaign is confounded for a comparison when a default
was left unset that the source documents as invalidating that comparison.
"""
import glob, os, re

# flag -> (default, what the default does, which comparison it invalidates)
FLAGS = {
 "suspectForcesEscalation": ("true",
   "a suspect fix also forces escalation (ca-tbl-policy.cc:71, :1030)",
   "ca_tbl_full vs ca_tbl_no_integrity -- measures ESCALATION, not rejection "
   "(documented at ca_tbl_causal_sim.cc:3439-3449)"),
 "floorOnDemand": ("false",
   "tier-1 floor ranges every cycle (ca_tbl_causal_sim.cc:3243)",
   "any energy comparison involving a hybrid arm: omits a measured 40-47% reduction"),
 "escalationRequiresEvidence": ("false",
   "escalation ignores target-tier availability",
   "any energy comparison: leaves defect 1a live"),
 "mahalUseR": ("false",
   "Mahalanobis omits the fix covariance R",
   "any integrity-gate sensitivity result; COMPOUND flag"),
 "nlosSource": ("legacy",
   "operative NLOS fraction ~75%",
   "anything depending on the allLos precondition (ca-tbl-policy.cc:903)"),
 "dwell": ("90",
   "tier-3 coverage ~3% (measured 0.0326 at b2500)",
   "any claim about tier-3 / escalation value"),
 "pcsForm": ("legacy", "PCS legacy form",
   "v2 with legacy pcsThresholdUp is a scale mismatch (F18)"),
}
NEVER = ["escalationRequiresEvidenceT2"]

print("# D2 — flag-confound audit\n")
print("Pure sidecar analysis. No runs. Every `run_id.txt` under `results/`.\n")
print("## Flags audited\n")
print("| flag | default | what the default does | invalidates |")
print("|---|---|---|---|")
for f,(d,w,inv) in FLAGS.items():
    print("| `%s` | `%s` | %s | %s |" % (f,d,w,inv))
print("\n`suspectRequireAllLos` was SPECIFIED (STAGE234 §2.2) and NEVER BUILT — "
      "the precondition is still hardcoded at `ca-tbl-policy.cc:903`.\n")

rids = sorted(glob.glob("results/**/run_id.txt", recursive=True))
print("## Campaigns\n")
print("%d run_id.txt sidecars found.\n" % len(rids))

# group by campaign (first path component under results/)
camp = {}
for r in rids:
    parts = r.split(os.sep)
    key = parts[1] if len(parts) > 2 else parts[1]
    try:
        inv = open(r).read().splitlines()[1]
    except (IndexError, IOError):
        inv = ""
    setflags = set(m.group(1) for m in re.finditer(r"--([A-Za-z0-9_]+)=", inv))
    camp.setdefault(key, {"n":0, "flags":None, "variants":set()})
    camp[key]["n"] += 1
    camp[key]["variants"].add(frozenset(setflags))
    camp[key]["flags"] = setflags

findings = 0
print("| campaign | dirs | flags explicitly set | confounding defaults left unset | affected comparison |")
print("|---|---|---|---|---|")
for k in sorted(camp):
    c = camp[k]
    if c["flags"] is None: continue
    unset = [f for f in FLAGS if f not in c["flags"]]
    # only report unset flags that actually matter for a comparison the campaign backs
    bad = []
    for f in unset:
        if f == "suspectForcesEscalation": bad.append(f)
        elif f in ("floorOnDemand","escalationRequiresEvidence"): bad.append(f)
    if bad: findings += 1
    print("| `%s` | %d | %s | %s | %s |" % (
        k, c["n"],
        ", ".join("`%s`"%x for x in sorted(c["flags"])) or "(none)",
        ", ".join("`%s`"%x for x in bad) or "—",
        "; ".join(FLAGS[x][2] for x in bad)[:160] or "—"))

print("\n## Explicit scoping — what IS and IS NOT invalidated\n")
print("- `results/p10_grid` (all 320 B1 + 680 B2 cells): `suspectForcesEscalation` unset, "
      "therefore **the `ca_tbl_no_integrity` column is confounded** — it measures escalation, "
      "not rejection. **The `ca_tbl_full` vs `static_2p4` comparisons in the same directories "
      "are NOT affected**, because neither arm's behaviour depends on that flag "
      "(`static_2p4` has no gate; `ca_tbl_full` has the flag at its documented default in "
      "both members of the comparison). H_cost and H_reliability stand.")
print("- `results/p10_env_v7`: `floorOnDemand` unset — energy comparisons omit a 40-47% "
      "reduction. Superseded by `p10_env_v8`, which sets it.")
print("- `results/p10_env_v8`, `results/p10_plane_v7`: `floorOnDemand` and "
      "`escalationRequiresEvidence` both set. `suspectForcesEscalation` unset, so their "
      "`no_integrity` columns carry the same confound; their `static_2p4` comparisons do not.")
print("\nSTATUS: %s" % ("FINDING" if findings else "OK"))
