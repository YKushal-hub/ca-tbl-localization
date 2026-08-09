#!/usr/bin/env bash
# v6 ladder: the 2x2 of --nlosSource x --arbitrateDutyBudget at seed 7.
#
# Two changes are outstanding against baseline_v5 -- arbitration and the NLOS
# source. Running both at once yields one number and no way to attribute it.
# This runs the 2x2 so each effect is separable and the interaction is visible.
#
#   A  legacy      / false  -- control; must reproduce baseline_v5
#   B  legacy      / true   -- isolates arbitration
#   C  propagation / false  -- isolates the NLOS fix
#   D  propagation / true   -- candidate baseline_v6
#
# Everything else held at defaults: seed 7, 3600 s, --pcsForm=legacy, --trace=true.
set -u
set -o pipefail
cd "$(dirname "$0")/../.." || exit 2

fail=0
for cfg in "legacy false A_legacy_noarb" \
           "legacy true  B_legacy_arb" \
           "propagation false C_prop_noarb" \
           "propagation true  D_prop_arb"; do
  set -- $cfg
  src=$1; arb=$2; name=$3
  d="results/v6_ladder/$name"
  printf '\n======== %s : nlosSource=%s arbitrateDutyBudget=%s ========\n' "$name" "$src" "$arb"
  rm -rf "$d"; mkdir -p "$d"
  ./ns3 run "ca_tbl_causal_sim --seed=7 --duration=3600 --nlosSource=$src \
             --arbitrateDutyBudget=$arb --pcsForm=legacy --trace=true --outdir=$d" 2>&1 | tail -5
  if [ "${PIPESTATUS[0]}" -ne 0 ]; then
    printf 'RUN FAILED: %s\n' "$name"; fail=1; continue
  fi
  python3 scratch/python_handoff/verify_causal_sim.py "$d" > "$d/verify.log" 2>&1
  vrc=$?
  tail -4 "$d/verify.log"
  [ $vrc -eq 0 ] || { printf 'VERIFIER FAILED: %s\n' "$name"; fail=1; }
done

printf '\n======== LADDER DONE (fail=%d) ========\n' "$fail"

# ---- GATE A: run A must reproduce baseline_v5 ------------------------------
#
# NOT a whole-file diff. The arbitration commit inserts handshakePreemptions at
# column 30, so a new arm_summary.csv has 63 columns against v5's 62 and byte
# identity is impossible by construction. The mapping is:
#
#     new 1-29  <-> v5 1-29        new 30 = handshakePreemptions (new)
#     new 31-63 <-> v5 30-62
#
# so the comparison is `cut -f1-29,31-63` on the NEW file against the UNMODIFIED
# v5 file. `cut -f1-62` is the trap: it keeps the new column and drops
# mac_deliveries, shifting every field from 30 onward by one and reporting a
# behavioural divergence that did not happen.
#
# DO NOT RE-RUN THIS SCRIPT WITHOUT CONVERTING TO NAME-BASED COLUMN MATCHING.
# The hand-worked offsets above are correct for exactly the 62/63-column pair
# they were written against, and silently wrong for any other. The identical
# hazard in G3 -- `cut -f1,3-15,17-37` against a trace that had grown to 42
# columns -- left five columns uncompared through three separate column
# additions, and G3 reported PASS the whole time.
#
# Note what that says about this comment. The hazard WAS noticed here, and
# noticing produced a comment and a one-off hand correction rather than a
# mechanism, so the same defect went on living everywhere else. See
# scripts/catbl_g3_ablation.py for the structural fix: match columns by name,
# and a column added later is compared automatically instead of dropped.
A=results/v6_ladder/A_legacy_noarb/arm_summary.csv
printf '\n======== GATE A: reproduce baseline_v5 ========\n'
if [ ! -s "$A" ]; then
  printf 'GATE A: no run A output\n'; exit 1
fi
if diff <(cut -d, -f1-29,31-63 "$A") results/baseline_v5/arm_summary.csv; then
  printf 'A REPRODUCES v5 (62 shared columns)\n'
else
  printf 'A DIVERGED -- STOP\n'; fail=1
fi
# The new column must be identically zero when arbitration is off; otherwise the
# flag does not actually gate the mechanism.
nz=$(tail -n +2 "$A" | cut -d, -f30 | grep -cv '^0$' || true)
if [ "$nz" -eq 0 ]; then
  printf 'handshakePreemptions == 0 on every arm under --arbitrateDutyBudget=false\n'
else
  printf 'GATE A: %s arm(s) preempted with arbitration OFF -- the flag does not gate\n' "$nz"
  fail=1
fi

exit $fail
