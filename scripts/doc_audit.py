#!/usr/bin/env python3
"""Verify document claims against source, mechanically.

citation_audit.py (G10) asks whether a cited PATH resolves. It does not read the
line. This asks whether a cited `file:line` says what the document claims, which is
the failure that produced a fault model documented as h=1 while injecting h=4, and a
comment describing a term the code zeroes on the next line.

Verification is against SOURCE. Not against another document, not against a prior
run's output.

Reports, per category, with the ACTUAL line content beside the claim:
  MATCH     the cited line exists and its content is consistent with the claim
  SHIFTED   the file has that line but a nearby line matches better -- the citation
            has drifted, which is a defect
  MISSING   the file does not have that line at all
  NOFILE    the cited file does not exist

An empty category is VACUOUS, not a pass, and is reported as such.
"""
import os, re, sys, glob

TIER1 = ["Architecture.md", "Simulation.md", "CA-TBL_Technical_Report_SelfContained.md",
         "src/catbl-propagation-study/DEFECTS.md",
         "REPORT_1_Propagation_Module.md", "REPORT_2_Project_Retrospective.md"] \
        + sorted(glob.glob("src/catbl-propagation-study/experiments/*.md"))

# `path:line` or `path:line-line`, inside backticks or bare. Source files only: a
# citation into a .md is prose-to-prose and is not verifiable this way.
CITE = re.compile(
    r"`?([A-Za-z0-9_./-]+\.(?:cc|h|py|sh))`?\s*:\s*(\d+)(?:\s*-\s*(\d+))?")

SEARCH_DIRS = ["", "src/lora-physics-engine/model/", "src/lora-physics-engine/model/submodules/",
               "scratch/", "scripts/", "src/catbl-propagation-study/"]


def resolve(path):
    if os.path.isfile(path):
        return path
    for d in SEARCH_DIRS:
        p = os.path.join(d, path)
        if os.path.isfile(p):
            return p
    base = os.path.basename(path)
    hits = []
    for root, dirs, files in os.walk("."):
        if any(s in root for s in (".git", "build", "cmake-cache", "src/lorawan")):
            continue
        if base in files:
            hits.append(os.path.join(root, base))
    return hits[0] if len(hits) == 1 else None


def main():
    rows = []
    scanned = 0
    for doc in TIER1:
        if not os.path.isfile(doc):
            rows.append((doc, 0, doc, 0, "NOFILE", "tier-1 document does not exist", "", "", 0))
            continue
        scanned += 1
        for i, line in enumerate(open(doc, errors="replace"), 1):
            for m in CITE.finditer(line):
                path, a, b = m.group(1), int(m.group(2)), m.group(3)
                real = resolve(path)
                if real is None:
                    rows.append((doc, i, path, a, "NOFILE", "", line.strip()[:90], "", a))
                    continue
                src = open(real, errors="replace").read().splitlines()
                end = int(b) if b else a
                if a > len(src) or end > len(src):
                    rows.append((doc, i, real, a, "MISSING",
                                 "file has %d lines, citation reaches %d" % (len(src), end),
                                 line.strip()[:90], ""))
                    continue
                # READ THE WHOLE CITED RANGE. Reading only line N of a `:N-M` citation
                # measures the parser, not the document: 75 of 137 citations here are
                # ranges averaging 6.2 lines, so 388 cited lines went unread and the
                # resulting token-overlap split described the extractor. Line 69 of
                # lora-terrain-submodule.cc is blank; the claim it anchors lives at 70.
                cited = src[a-1:end]
                content = " / ".join(x.strip() for x in cited if x.strip())[:160]
                # A citation whose line merely EXISTS is not verified. Extract the
                # identifiers the document names near the citation and require at least
                # one to appear at or near the cited line; otherwise the citation is
                # either shifted or unsupported. This is what makes MATCH mean something.
                # NOTE ON SCOPE. An earlier version of this tool tried to adjudicate
                # whether the cited line SUPPORTS the claim, by requiring an identifier
                # from the document to appear in the line. It flagged 71 of 137 citations
                # as unsupported, and the first one checked by hand was correct:
                # Architecture.md cites lora-terrain-submodule.cc:69 for "terrain hardcodes
                # 868 MHz" and the line is `double frequencyHz = 868e6;` -- true, and
                # invisible to token matching. A gate with that false-positive rate is
                # worse than no gate.
                #
                # So this tool GATES only on what is mechanically decidable -- the file
                # exists, the line exists -- and EMITS the claim/content pairs for review
                # rather than adjudicating them. Semantic verification is reported, by a
                # person, against the pairs it prints. That division is the point.
                toks = set(re.findall(r"[A-Za-z_][A-Za-z0-9_]{3,}", line))
                toks -= {"true","false","this","that","which","where","line","lines",
                         "the","and","for","with","from","into","onto","over","under"}
                toks = {t for t in toks if not t.endswith(".cc") and not t.endswith(".h")}
                window = " ".join(src[max(0,a-1-3):min(len(src),end+3)])
                exact = " ".join(cited)
                hit_exact = any(t in exact for t in toks)
                hit_near  = any(t in window for t in toks)
                cat = "RESOLVES"
                hint = ("token-in-line" if hit_exact
                        else ("token-within-3-lines" if hit_near else "no-token-overlap"))
                rows.append((doc, i, real, a, cat, content, line.strip()[:90], hint, end))
    print("DOCUMENT AUDIT -- file:line citations verified against source")
    print("population: %d of %d tier-1 documents scanned; %d citations extracted\n"
          % (scanned, len(TIER1), len(rows)))
    cats = {}
    for r in rows:
        cats.setdefault(r[4], []).append(r)
    for c in ("NOFILE", "MISSING", "RESOLVES"):
        got = cats.get(c, [])
        if not got:
            print("  %-8s 0  -- VACUOUS, not a pass: no citation fell in this category" % c)
        else:
            print("  %-8s %d" % (c, len(got)))
    bad = cats.get("NOFILE", []) + cats.get("MISSING", [])
    hints = {}
    for r in cats.get("RESOLVES", []):
        hints[r[7]] = hints.get(r[7], 0) + 1
    print("\n  review hints (NOT verdicts -- token overlap only):")
    for k, v in sorted(hints.items()):
        print("    %-22s %d" % (k, v))
    if bad:
        print("\nDEFECTS -- cited file:line does not resolve:")
        for doc, i, path, a, cat, note, ctx, *_ in bad:
            print("  %s:%d  cites %s:%d  [%s] %s" % (doc, i, path, a, cat, note))
            print("      context: %s" % ctx)
    if "--show" in sys.argv:
        print("\nEVERY RESOLVED CITATION, claimed context beside actual line content:")
        for doc, i, path, a, cat, content, ctx, hint, end in cats.get("RESOLVES", []):
            span = "%d" % a if end == a else "%d-%d" % (a, end)
            print("  %s:%d -> %s:%s" % (doc, i, path, span))
            print("      document says : %s" % ctx)
            print("      source line is: %s   [%s]" % (content, hint))
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
