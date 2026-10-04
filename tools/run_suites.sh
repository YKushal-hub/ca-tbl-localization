#!/bin/bash
# Runs every unit suite binary; prints rc and the last summary line of each.
# usage: bash tools/run_suites.sh   (from anywhere; uses CC/CXX, default gcc-11/g++-11)
cd "$(dirname "$0")/.." || exit 2
# Rebuild first: a suite binary left over from before a header change runs against the new
# library layout and can segfault (seen 2026-10-03 with ca_tbl_sparse_anchor_test).
CC=${CC:-gcc-11} CXX=${CXX:-g++-11} ./ns3 build $(ls scratch/ca_tbl_*_test.cc | xargs -n1 basename | sed 's/\.cc$//') ca_tbl_packet_validator > /dev/null 2>&1 || { echo "BUILD FAILED"; exit 2; }
O=$(mktemp -d "${TMPDIR:-/tmp}/suites.XXXXXX")
bad=0
for b in build/scratch/ns3.47-ca_tbl_*_test-optimized build/scratch/ns3.47-ca_tbl_packet_validator-optimized; do
    n=$(basename "$b" | sed 's/ns3.47-//; s/-optimized//')
    if [ "$n" = ca_tbl_packet_validator ]; then "$b" --outdir="$O" > "$O/$n.out" 2>&1; else "$b" > "$O/$n.out" 2>&1; fi
    rc=$?; [ $rc -ne 0 ] && bad=1
    printf '%-28s rc=%d  %s\n' "$n" $rc "$(grep -iE '[0-9]+ (passed|pass|ok|checks)|passed|failed' "$O/$n.out" | tail -1 | cut -c1-90)"
done
rm -rf "$O"
echo "SUITES_BAD=$bad"
