#!/usr/bin/env python3
"""G11 — bring src/catbl-propagation-study/ under the verification regime.

The module reproduced the engines in closed form, refused to reconcile, and that is
how the six-decibel sensitivity indexing defect was found. It then sat outside the
regime it created the need for: no gate touched it, and nothing checked its corpus
against its own digest.

Three assertions:
  1. results/corpus.json verifies against results/corpus.json.sha256
  2. validate.py runs and its reconciliation meets the REGISTERED criterion
     (15% on r80, 3 dB on PDR), or fails carrying the measured discrepancy
  3. every module file is tracked, except an enumerated allowance, and the corpus
     digest matches the one recorded in every experiment sidecar claiming to use it

Exit 0 if all three hold, 1 otherwise.
"""
import hashlib, glob, json, os, re, subprocess, sys

MOD = "src/catbl-propagation-study"
CORPUS = os.path.join(MOD, "results", "corpus.json")
CORPUS_SHA = CORPUS + ".sha256"

# Enumerated, with a reason. Not a blanket exemption for anything untracked.
UNTRACKED_ALLOWED = {
    "results/corpus.json":
        "10.7 MB, deterministic from corpus.py at seed 20260819; its sha256 is "
        "tracked instead and assertion 1 verifies the file against it",
}


def a1_corpus_digest():
    print("--- 1. corpus verifies against its recorded digest")
    if not os.path.exists(CORPUS):
        print("    corpus.json ABSENT -- regenerate with corpus.py (seed 20260819)")
        return False
    if not os.path.exists(CORPUS_SHA):
        print("    corpus.json.sha256 ABSENT -- the digest anchor is missing")
        return False
    want = open(CORPUS_SHA).read().split()[0].strip()
    got = hashlib.sha256(open(CORPUS, "rb").read()).hexdigest()
    ok = want == got
    print("    recorded %s" % want[:24])
    print("    actual   %s   %s" % (got[:24], "MATCH" if ok else "*** MISMATCH ***"))
    return ok


def a2_validation():
    print("--- 2. analytic model reconciles with the engines at the registered criterion")
    r = subprocess.run([sys.executable, os.path.join(MOD, "validate.py")],
                       capture_output=True, text=True)
    out = (r.stdout or "") + (r.stderr or "")
    for line in out.splitlines():
        if any(k in line for k in ("r80 model", "ACCEPTANCE", "corpus draws", "FATAL", "tier")):
            print("    " + line.strip())
    if r.returncode != 0:
        print("    VALIDATION FAILED -- the discrepancy is in the lines above.")
        print("    Do NOT tune the analytic model to match. Report which term differs")
        print("    and which of the two is more likely wrong.")
    return r.returncode == 0


def a3_tracked_and_digest_consistent():
    print("--- 3. module files tracked; corpus digest consistent across sidecars")
    tracked = set(subprocess.run(["git", "ls-files", MOD], capture_output=True,
                                 text=True).stdout.split())
    ok = True
    for p in sorted(glob.glob(MOD + "/**/*", recursive=True)):
        if os.path.isdir(p) or "__pycache__" in p:
            continue
        rel = os.path.relpath(p, MOD)
        if p in tracked:
            continue
        if rel in UNTRACKED_ALLOWED:
            print("    allowed untracked: %s" % rel)
            print("        %s" % UNTRACKED_ALLOWED[rel])
            continue
        print("    UNTRACKED and not allowed: %s" % rel)
        ok = False
    want = open(CORPUS_SHA).read().split()[0].strip() if os.path.exists(CORPUS_SHA) else None
    # The glob IS the population claim, so it is printed, and every file that leaves
    # the population is accounted for by name. A sidecar that fails to parse used to be
    # skipped silently, which shrinks the population invisibly -- it now fails the gate.
    globs = [MOD + "/experiments/*.json", MOD + "/results/*_sidecar.json"]
    sidecars = sorted(sum((glob.glob(g) for g in globs), []))
    print("    population globs: %s" % ", ".join(os.path.relpath(g, MOD) for g in globs))
    print("    files matched: %d" % len(sidecars))
    n = 0
    no_digest = []
    for s in sidecars:
        try:
            d = json.load(open(s))
        except Exception as e:
            print("    UNPARSEABLE SIDECAR (population would silently shrink): %s: %s"
                  % (os.path.relpath(s, MOD), e))
            ok = False
            continue
        got = d.get("corpus_sha256") or d.get("corpus_digest")
        if not got:
            no_digest.append(os.path.relpath(s, MOD))
            continue
        n += 1
        if got != want:
            print("    SIDECAR DIGEST MISMATCH: %s claims %s" % (s, str(got)[:24]))
            ok = False
    if no_digest:
        print("    matched but carry no corpus digest (%d): %s"
              % (len(no_digest), ", ".join(no_digest)))
    if n == 0:
        # An empty population is not a pass. It is reported as what it is: this
        # assertion checked nothing, because the sweeps that write these sidecars
        # are blocked behind assertion 2.
        print("    experiment sidecars claiming a corpus digest: 0")
        print("    VACUOUS -- no sidecar carries a digest, so this assertion verified")
        print("    NOTHING. It is not evidence of consistency. It becomes a real check")
        print("    only once the sweeps run.")
    else:
        print("    experiment sidecars claiming a corpus digest: %d, all matching: %s"
              % (n, "yes" if ok else "no"))
    return ok


def main():
    print("G11 — propagation study under the gates")
    r = [a1_corpus_digest(), a2_validation(), a3_tracked_and_digest_consistent()]
    print("G11 VERDICT: %s" % ("PASS" if all(r) else "FAIL"))
    return 0 if all(r) else 1


if __name__ == "__main__":
    sys.exit(main())
