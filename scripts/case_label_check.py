#!/usr/bin/env python3
"""Targeted check: citations whose target line is a `case` label, `default:`, or blank.

WHY THIS EXISTS. Expanding the environment table in REPORT_1_Propagation_Module.md
found twelve off-by-one citations in fifteen. Every one had the same shape: the
citation pointed at the `case ENV_RURAL_OPEN:` label and the value it claimed to cite
was on the NEXT line. A citation that lands on a switch label, or on a blank line, is
a candidate for that drift and can be checked in one pass instead of one at a time.

A hit is NOT automatically a defect. `file:120-140` that opens on a case label is
citing the whole block, correctly. This flags candidates; the verdict is a person's.

POPULATION. Two disjoint sets, reported separately because they are not equally
knowable:

  QUALIFIED     `path.cc:N` or `path.cc:N-M`. The file is named in the citation.
                Fully mechanical.

  CONTINUATION  a bare `:N` in backticks, whose file is carried over from an earlier
                citation in prose. Commit 6f1b4ab established this form is NOT
                mechanically resolvable in general -- the carry-over is a reading
                convention, not a syntax. This tool resolves each one to the nearest
                preceding qualified citation and labels the result HEURISTIC. Any
                continuation it cannot resolve is counted and named, never dropped.
"""
import os, re, sys, glob

TIER1 = ["Architecture.md", "Simulation.md", "CA-TBL_Technical_Report_SelfContained.md",
         "src/catbl-propagation-study/DEFECTS.md",
         "REPORT_1_Propagation_Module.md", "REPORT_2_Project_Retrospective.md"] \
        + sorted(glob.glob("src/catbl-propagation-study/experiments/*.md"))

QUAL = re.compile(r"`?([A-Za-z0-9_./-]+\.(?:cc|h|py|sh))`?\s*:\s*(\d+)(?:\s*-\s*(\d+))?")
BARE = re.compile(r"`\s*:\s*(\d+)(?:\s*-\s*(\d+))?\s*`")

SEARCH_DIRS = ["", "src/lora-physics-engine/model/", "src/lora-physics-engine/model/submodules/",
               "scratch/", "scripts/", "src/catbl-propagation-study/"]

_cache = {}
def resolve(path):
    if path in _cache:
        return _cache[path]
    r = None
    if os.path.isfile(path):
        r = path
    else:
        for d in SEARCH_DIRS:
            p = os.path.join(d, path)
            if os.path.isfile(p):
                r = p
                break
    _cache[path] = r
    return r

_src = {}
def lines_of(real):
    if real not in _src:
        _src[real] = open(real, errors="replace").read().splitlines()
    return _src[real]

CASE = re.compile(r"^\s*(case\b|default\s*:)")

def classify(real, a):
    src = lines_of(real)
    if a < 1 or a > len(src):
        # NOT a blank line. The file does not have this line at all -- for a
        # continuation, that is the carry-over heuristic naming the wrong file.
        return "OUTOFRANGE", "(file has %d lines; line %d does not exist)" % (len(src), a), ""
    tgt = src[a-1]
    nxt = src[a].strip() if a < len(src) else ""
    if CASE.match(tgt):
        return "CASE-LABEL", tgt.strip(), nxt
    if not tgt.strip():
        return "BLANK", "", nxt
    return "OK", tgt.strip(), nxt

def main():
    qual_hits, cont_hits = [], []
    nq = nc = 0
    unresolved = []
    for doc in TIER1:
        if not os.path.isfile(doc):
            continue
        carry = None   # last qualified path seen in this document
        for i, line in enumerate(open(doc, errors="replace"), 1):
            spans = []
            events = []
            for m in QUAL.finditer(line):
                spans.append((m.start(), m.end()))
                events.append((m.start(), "Q", m.group(1), int(m.group(2)), m.group(3)))
            for m in BARE.finditer(line):
                if any(s <= m.start() < e for s, e in spans):
                    continue
                events.append((m.start(), "C", None, int(m.group(1)), m.group(2)))
            for _, kind, path, a, b in sorted(events):
                if kind == "Q":
                    nq += 1
                    carry = path
                    real = resolve(path)
                    if real is None:
                        continue
                    cat, tgt, nxt = classify(real, a)
                    if cat in ("CASE-LABEL", "BLANK"):
                        qual_hits.append((doc, i, real, a, b, cat, tgt, nxt, line.strip()[:88]))
                else:
                    nc += 1
                    if carry is None:
                        unresolved.append((doc, i, a, line.strip()[:88]))
                        continue
                    real = resolve(carry)
                    if real is None:
                        unresolved.append((doc, i, a, line.strip()[:88]))
                        continue
                    cat, tgt, nxt = classify(real, a)
                    if cat in ("CASE-LABEL", "BLANK", "OUTOFRANGE"):
                        cont_hits.append((doc, i, real, a, b, cat, tgt, nxt, line.strip()[:88]))

    tot = nq + nc
    print("CASE-LABEL / BLANK-TARGET CHECK")
    print("population: %d citations over %d tier-1 documents"
          "  (%d qualified, %d continuation)\n" % (tot, len(TIER1), nq, nc))

    def dump(title, hits, note):
        print("%s -- %d hit%s" % (title, len(hits), "" if len(hits) == 1 else "s"))
        print("  %s" % note)
        if not hits:
            print("  (none)\n")
            return
        for doc, i, real, a, b, cat, tgt, nxt, ctx in hits:
            span = "%d" % a if not b else "%d-%s" % (a, b)
            print("  [%s] %s:%d  ->  %s:%s%s" % (cat, doc, i, real, span,
                  "   RANGE (may legitimately open on a label)" if b else ""))
            print("       document : %s" % ctx)
            if cat == "OUTOFRANGE":
                print("       %s" % tgt)
            else:
                print("       line %-5d: %s" % (a, tgt if tgt else "(blank)"))
                print("       line %-5d: %s" % (a+1, nxt if nxt else "(blank)"))
        print("")

    dump("QUALIFIED", qual_hits, "file named in the citation; fully mechanical")
    dump("CONTINUATION (HEURISTIC)", cont_hits,
         "file inferred as the nearest preceding qualified citation -- not authoritative")

    print("UNRESOLVED CONTINUATIONS -- %d" % len(unresolved))
    if unresolved:
        for doc, i, a, ctx in unresolved:
            print("  %s:%d  bare :%d  -- no preceding qualified citation to carry from" % (doc, i, a))
            print("       %s" % ctx)
    else:
        print("  (none -- every continuation had a carry-over candidate)")
    single = [h for h in qual_hits + cont_hits if not h[4]]
    print("\nSINGLE-LINE hits (no range to justify a label target): %d" % len(single))
    return 0

if __name__ == "__main__":
    sys.exit(main())
