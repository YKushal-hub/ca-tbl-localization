#!/usr/bin/env bash
# G14 negative fixture. Rule 6: no gate is trusted without forced-failure evidence.
#
# A gate that has only ever passed proves nothing. This reintroduces, in a throwaway
# copy of the source, each defect G14 claims to catch, and asserts that it fires.
#
# The first case is D1 itself: `sf - 7` applied to the 2.4 GHz table, whose index 0 is
# SF5. That is the defect that sat on the tier carrying ~95% of active cycles and passed
# every gate in the project.
set -u
cd "$(dirname "$0")/.." || exit 2
ROOT=$(pwd)
GATE="$ROOT/scripts/sensitivity_gate.py"
MODEL_REL="src/lora-physics-engine/model"

pass=0
fail=0

check() {
    local name="$1" expect="$2" root="$3"
    CATBL_ROOT="$root" python3 "$GATE" > "$root/out.txt" 2>&1
    local rc=$?
    if [ "$rc" -eq "$expect" ]; then
        printf '  G14 ok    %-46s exit=%d\n' "$name" "$rc"
        pass=$((pass + 1))
    else
        printf '  G14 FAIL  %-46s exit=%d expected %d\n' "$name" "$rc" "$expect"
        sed 's/^/            /' "$root/out.txt" | head -6
        fail=$((fail + 1))
    fi
}

mkfixture() {
    local d
    d=$(mktemp -d "${TMPDIR:-/tmp}/g14fix.XXXXXX")
    mkdir -p "$d/$MODEL_REL"
    cp "$ROOT/$MODEL_REL"/lora-physics-parameters.h \
       "$ROOT/$MODEL_REL"/lora-2p4-physics-parameters.h \
       "$ROOT/$MODEL_REL"/lora-uwb-physics-parameters.h \
       "$ROOT/$MODEL_REL"/lora-physics-engine.cc \
       "$ROOT/$MODEL_REL"/lora-2p4-physics-engine.cc \
       "$ROOT/$MODEL_REL"/lora-uwb-physics-engine.cc \
       "$d/$MODEL_REL/" 2>/dev/null
    echo "$d"
}

echo "--- G14 sensitivity gate: negative fixtures ---"

# 0. the unmodified tree must PASS, or every case below is meaningless
u=$(mkfixture)
check "unmodified tree passes" 0 "$u"

# 1. D1 reintroduced: sf-7 against a base-SF5 table
f1=$(mkfixture)
sed -i 's/static_cast<int>(sf) - 5/static_cast<int>(sf) - 7/' \
    "$f1/$MODEL_REL/lora-2p4-physics-engine.cc"
check "D1 reintroduced (2.4GHz sf-7 on base SF5)" 1 "$f1"

# 2. the same defect on UWB
f2=$(mkfixture)
sed -i 's/static_cast<int>(sf) - 5/static_cast<int>(sf) - 7/' \
    "$f2/$MODEL_REL/lora-uwb-physics-engine.cc"
check "D1 reintroduced on UWB" 1 "$f2"

# 3. clamp inconsistent with table size
f3=$(mkfixture)
sed -i 's/std::min(7, std::max(0, static_cast<int>(sf) - 5))/std::min(5, std::max(0, static_cast<int>(sf) - 5))/' \
    "$f3/$MODEL_REL/lora-2p4-physics-engine.cc"
check "clamp 5 on an 8-entry table" 1 "$f3"

# 4. the 868 tier broken the other way
f4=$(mkfixture)
sed -i 's/static_cast<int>(sf) - 7/static_cast<int>(sf) - 5/' \
    "$f4/$MODEL_REL/lora-physics-engine.cc"
check "868 subtracting 5 on a base-SF7 table" 1 "$f4"

# 5. the declaration comment removed -- the gate must refuse, not assume
f5=$(mkfixture)
sed -i 's|double sensitivityDbm\[8\];.*|double sensitivityDbm[8];|' \
    "$f5/$MODEL_REL/lora-2p4-physics-parameters.h"
check "declaration comment stripped" 1 "$f5"

rm -rf "$u" "$f1" "$f2" "$f3" "$f4" "$f5"

echo "  G14 selftest: $pass passed, $fail failed"
[ "$fail" -eq 0 ] || exit 1
