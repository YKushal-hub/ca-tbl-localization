#!/usr/bin/env bash
# T9 (TECH_DEBT section 6): every published number was produced under --build-profile=optimized, and the default
# profile enables logging and runs ~10x slower; nothing enforced it. Exit 0 only if <cache> records build_profile
# optimized. usage: build_profile_check.sh [<CMakeCache.txt>]   (default: cmake-cache/CMakeCache.txt)
C=${1:-cmake-cache/CMakeCache.txt}
[ -f "$C" ] || { echo "  build profile: $C missing -- configure first"; exit 1; }
p=$(sed -n 's/^build_profile:INTERNAL=//p' "$C")
if [ "$p" = optimized ]; then echo "  build profile: optimized"; exit 0; fi
echo "  build profile: '${p:-unset}', not optimized -- reconfigure with --build-profile=optimized"; exit 1
