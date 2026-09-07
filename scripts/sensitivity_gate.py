#!/usr/bin/env python3
"""A7 / G14 -- the sensitivity-table fidelity gate.

WHY THIS EXISTS. The six-decibel SF-index defect -- `sf - 7` applied to tables whose
index 0 is SF5 -- sat on the tier carrying about 95% of active cycles and PASSED EVERY
GATE, VERIFIER CHECK AND VALIDATION IN THE PROJECT. Asked directly, the technical
report answers that no check in the project would catch it today. What found it was an
independent closed-form model refusing to reconcile, not a gate.

The existing fidelity checks -- CRLB ratio, UWB floor, energy to 10 significant figures
-- verify ranging precision and energy accounting, both of which are DOWNSTREAM of
whether a packet was received at all. None of them reads the sensitivity table.

WHAT IT CHECKS. For each tier, three things that must agree with each other:

  1. The table's declared size N and its own comment "index 0=SF<base>".
  2. The index expression the engine uses, min(N-1, max(0, sf - B)).
  3. That B equals the base the comment declares, and that the upper clamp is N-1.

That triple is precisely what D1 broke: the 2.4 GHz and UWB tables declare base SF5 and
carry eight entries, while the expression subtracted 7 -- so SF7 read the SF5 cell and
every margin on those tiers was wrong by two SF steps.

  4. Monotonicity: sensitivity must improve (become more negative) as SF rises. A
     table that is not monotone is either mis-transcribed or mis-ordered, and either
     way the index arithmetic above cannot save it.

WHAT IT DOES NOT CHECK, stated so the gap is visible rather than assumed closed: the
absolute values are not cross-checked against kTB + NF + required SNR. That needs the
per-SF required-SNR table, which is not carried in these headers. Item 3 of the A7
specification is therefore NOT implemented here, and the gate is narrower than the work
package asks for.
"""
import os
import re
import sys

ROOT = os.environ.get("CATBL_ROOT", ".")
MODEL = os.path.join(ROOT, "src/lora-physics-engine/model")

TIERS = [
    ("868MHz", "lora-physics-parameters.h", "lora-physics-engine.cc"),
    ("2.4GHz", "lora-2p4-physics-parameters.h", "lora-2p4-physics-engine.cc"),
    ("UWB", "lora-uwb-physics-parameters.h", "lora-uwb-physics-engine.cc"),
]

DECL = re.compile(r"double\s+sensitivityDbm\[(\d+)\]\s*;\s*//\s*index\s*0\s*=\s*SF(\d+)")
EXPR = re.compile(r"sfIndex\s*=\s*std::min\(\s*(\d+)\s*,\s*std::max\(\s*0\s*,\s*"
                  r"static_cast<int>\(sf\)\s*-\s*(\d+)\s*\)\s*\)")

fail = 0
print("G14 -- sensitivity table SF-index fidelity")

for name, hdr, eng in TIERS:
    hp = os.path.join(MODEL, hdr)
    ep = os.path.join(MODEL, eng)
    if not os.path.isfile(hp) or not os.path.isfile(ep):
        print("  %-8s MISSING SOURCE (%s / %s)" % (name, hdr, eng))
        fail = 1
        continue

    d = DECL.search(open(hp).read())
    if not d:
        print("  %-8s no 'double sensitivityDbm[N]; // index 0=SFb' declaration found."
              % name)
        print("           The gate reads the table's OWN comment as the authority on its"
              " base;")
        print("           without it there is nothing to check the expression against.")
        fail = 1
        continue
    size, base = int(d.group(1)), int(d.group(2))

    e = EXPR.search(open(ep).read())
    if not e:
        print("  %-8s no recognisable sfIndex expression in %s" % (name, eng))
        fail = 1
        continue
    clamp, sub = int(e.group(1)), int(e.group(2))

    ok = True
    if sub != base:
        print("  %-8s *** SF-INDEX DEFECT: table declares index 0 = SF%d, expression "
              "subtracts %d." % (name, base, sub))
        print("           SF%d would read the SF%d cell -- an error of %d SF steps on "
              "every margin." % (base, base + (sub - base), abs(sub - base)))
        ok = False
    if clamp != size - 1:
        print("  %-8s *** CLAMP DEFECT: table has %d entries so the upper clamp must be "
              "%d, found %d." % (name, size, size - 1, clamp))
        ok = False

    vals = re.findall(r"sensitivityDbm\[\s*(\d+)\s*\]\s*=\s*(-?\d+(?:\.\d+)?)", open(hp).read())
    vals += re.findall(r"sensitivityDbm\[\s*(\d+)\s*\]\s*=\s*(-?\d+(?:\.\d+)?)",
                       open(hp.replace(".h", ".cc")).read()) if os.path.isfile(
        hp.replace(".h", ".cc")) else []
    if vals:
        seq = [None] * size
        for i, v in vals:
            i = int(i)
            if 0 <= i < size:
                seq[i] = float(v)
        got = [v for v in seq if v is not None]
        if len(got) >= 2 and any(got[i + 1] > got[i] for i in range(len(got) - 1)):
            print("  %-8s *** NOT MONOTONE: sensitivity must improve as SF rises; "
                  "found %s" % (name, got))
            ok = False

    if ok:
        print("  %-8s ok    table[%d] base SF%d, expression sf-%d clamp %d -- consistent"
              % (name, size, base, sub, clamp))
    else:
        fail = 1

print()
print("  NOT CHECKED: absolute values against kTB + NF + required SNR. That needs the")
print("  per-SF required-SNR table, which these headers do not carry, so A7 item 3 is")
print("  not implemented and this gate is narrower than the work package asks for.")

sys.exit(1 if fail else 0)
