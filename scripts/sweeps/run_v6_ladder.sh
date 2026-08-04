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
exit $fail
