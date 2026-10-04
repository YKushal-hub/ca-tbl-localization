#!/usr/bin/env python3
"""G50: QUERY and RESPONSE carry what the spec says (docs/SPEC_CONFORMANCE_PLAN.md NW-9,
--queryV2).

Network sect 4: QUERY = {query_id, target_manifest_id_list[], requester_id, ttl, priority};
a matching node "respond[s] directly with own position table entry", a proxy answers "on
behalf of that entry", and a node rebroadcasts only when neither applies. A QUERY carried one
manifest id, a RESPONSE carried no entry and no query id, nothing at the requester handled
it, and an answering target still rebroadcast. Under --queryV2 a node answers EVERY listed
manifest it can, each answer carrying the entry (with the producer's PCS, integrity verdict,
battery, degradation, UWB state and granting anchor), and relays the query with the answered
manifests removed -- not at all if nothing is left (the batch-safe rule).

Reads a directory written by scripts/query_v2_gate.sh (v2: G32's query regime with mesh
links, the reverse path, five measured subjects and a five-manifest batch; v1: the same
without --queryV2; r_noq, r_list, r_bad). Every check runs and is reported; exit 1 if any
failed, 2 if an input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  LEGACY  without the flag: no responses_/queryv2_ file, no NW-9 line, and no v2 field in
          any frame of the event log.
  CONTENT every answer the requester got names the subject its manifest belongs to, and its
          entry is that subject's own published estimate -- the trace row of the cycle before
          the entry's timestamp (publication precedes Step) -- with that row's PCS quantised
          (+-1, the trace prints 4 decimals) and, from tag 0's trace, its integrity verdict
          and UWB state. grantedBy is 0 (NW-7 off). Recomputed from the traces.
  LIST    at every node and tier that handled a query, the manifests it answered and the
          ones it relayed partition the list it received (the first QUERY copy delivered to
          it in events_*); it relayed iff something was left; a relayed copy on the air
          carries exactly what was left. Non-vacuous: some lists were reduced and some
          relays suppressed.
  COUNTS  the NW-9 line equals the logs: answers, proxy answers, suppressions, reductions,
          answers heard and answers carrying an entry.
  DIGEST  responses_ and queryv2_ are in run_id.txt with their sha256.
  R       --queryV2 without queries, a list without --queryV2, and a bad id are refused.
"""
import csv
import hashlib
import os
import re
import sys

LINE = re.compile(r"NW-9 queries v2 \((\w+)\): answered as target (\d+), by proxy (\d+), "
                  r"rebroadcasts suppressed (\d+), lists reduced (\d+), answers heard by "
                  r"requesters (\d+) \(with an entry (\d+)\)")
OK_OUT = ("delivered", "capture")
DT = 2.5


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def rows(p):
    with open(p) as f:
        return list(csv.DictReader(f))


def payload(p):
    return dict(kv.split("=", 1) for kv in p.split(";") if "=" in kv)


def quant(x):
    if not x > 0.0:
        return 0
    if x >= 1.0:
        return 255
    return int(x * 255.0 + 0.5)


def main():
    if len(sys.argv) != 2:
        print("usage: query_v2_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError, StopIteration) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    V = os.path.join(d, "v2")
    st = {}

    def load():
        if "q" not in st:
            st["resp"] = rows(os.path.join(V, "responses_ca_tbl_full_seed7.csv"))
            st["q"] = rows(os.path.join(V, "queryv2_ca_tbl_full_seed7.csv"))
            ev = rows(os.path.join(V, "events_ca_tbl_full_seed7.csv"))
            for r in ev:
                r["p"] = payload(r["payload"])
            st["ev"] = ev
        return st

    def c_runs():
        bad = [n for n in ("v2", "v1") if rc(d, n) != 0]
        check("RUNS", not bad, "every run the checks read exited 0" if not bad
              else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_legacy():
        fs = os.listdir(os.path.join(d, "v1"))
        files = [f for f in fs if f.startswith("responses_") or f.startswith("queryv2_")]
        lines = log(d, "v1").count("NW-9")
        ev = rows(os.path.join(d, "v1", "events_ca_tbl_full_seed7.csv"))
        v2f = sum(1 for r in ev if ";q2=" in r["payload"] or ";rm=" in r["payload"])
        check("LEGACY", not files and lines == 0 and v2f == 0 and ev,
              f"without the flag: v2 files {len(files)}, NW-9 lines {lines}, frames with v2 "
              f"fields {v2f} (need 0, 0, 0)")
    safe("LEGACY", c_legacy)

    def c_content():
        s = load()
        traces = {"1": {round(float(r["t"]) / DT): r
                        for r in rows(os.path.join(V, "trace_ca_tbl_full_seed7.csv"))}}
        for f in os.listdir(V):
            m = re.match(r"subjtrace_ca_tbl_full_seed7_subj(\d+)\.csv$", f)
            if m:
                traces[m.group(1)] = {round(float(r["t"]) / DT): r for r in rows(os.path.join(V, f))}
        bad, n, prior, pcs_sat = [], 0, 0, 0
        for r in s["resp"]:
            n += 1
            node = r["entryNode"]
            if node != r["manifest"]:
                bad.append((r["t"], "manifest", r["manifest"], "entry of", node))
                continue
            tr = traces.get(node)
            if tr is None:
                bad.append((r["t"], "no trace for", node))
                continue
            k = round(float(r["entryT"]) / DT) - 1
            if k not in tr:
                prior += 1
                if int(r["pcsQ"]) != 0 or r["integrity"] != "0" or r["uwbActive"] != "0":
                    bad.append((r["t"], node, "prior entry with a status"))
                continue
            row = tr[k]
            if abs(float(row["estX"]) - float(r["x"])) > 2e-4 or \
                    abs(float(row["estY"]) - float(r["y"])) > 2e-4:
                bad.append((r["t"], node, "position", r["x"], row["estX"]))
            q = quant(float(row["pcs"]))
            pcs_sat += int(q == 255)
            if abs(q - int(r["pcsQ"])) > 1:
                bad.append((r["t"], node, "pcsQ", r["pcsQ"], q))
            if r["grantedBy"] != "0":
                bad.append((r["t"], node, "grantedBy", r["grantedBy"]))
            if node == "1":
                if r["integrity"] != row["suspect"]:
                    bad.append((r["t"], node, "integrity", r["integrity"], row["suspect"]))
                if r["uwbActive"] != ("1" if row["nextTier"] == "UWB" else "0"):
                    bad.append((r["t"], node, "uwbActive", r["uwbActive"], row["nextTier"]))
        check("CONTENT", n > 0 and not bad,
              f"{n} answers ({prior} on the initial entry, {pcs_sat} with PCS saturated at 255); "
              f"not the named subject's published state: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("CONTENT", c_content)

    def c_list():
        s = load()
        first = {}
        relayed = {}
        for r in s["ev"]:
            if r["msgType"] != "QUERY" or "q2" not in r["p"]:
                continue
            lst = [x for x in r["p"]["list"].split("|") if x]
            if r["outcome"] in OK_OUT:
                key = (r["rxId"], r["tier"], r["p"]["q2"])
                first.setdefault(key, lst)
            relayed.setdefault((r["txId"], r["tier"], r["p"]["q2"]), set()).add(tuple(lst))
        dec = {}
        for r in s["q"]:
            dec.setdefault((r["node"], r["tier"], r["queryId"]), []).append(r)
        bad, reduced, suppressed, n = [], 0, 0, 0
        for key, ds in dec.items():
            got = first.get(key)
            if got is None:
                bad.append((key, "decisions without a delivered copy"))
                continue
            n += 1
            ans = [x["manifest"] for x in ds if x["what"] in ("answer", "proxy")]
            rel = [x for x in ds if x["what"] == "relay"]
            sup = [x for x in ds if x["what"] == "suppress"]
            left = [x for x in got if x not in ans]
            if sorted(ans + left) != sorted(got) or len(set(ans)) != len(ans):
                bad.append((key, "answered not a subset of the list", ans, got))
            if left:
                if sup or (rel and [x for x in rel[0]["left"].split("|") if x] != left):
                    bad.append((key, "left", left, [x["left"] for x in rel], len(sup)))
                if rel and len(left) < len(got):
                    reduced += 1
                    on_air = relayed.get(key)
                    if on_air is not None and tuple(left) not in on_air:
                        bad.append((key, "relayed copy carries", on_air, "not", left))
            else:
                if rel or len(sup) != 1:
                    bad.append((key, "everything answered but", len(rel), "relays", len(sup)))
                suppressed += 1
                if key in relayed:
                    bad.append((key, "a fully answered query was put on the air again"))
        st["reduced"], st["suppressed"] = reduced, suppressed
        check("LIST", n > 0 and reduced > 0 and suppressed > 0 and not bad,
              f"{n} (node, tier, query) decisions; {reduced} lists reduced, {suppressed} relays "
              f"suppressed (need > 0 each); against the batch-safe rule: {len(bad)} (need 0)"
              f"{': ' + str(bad[:2]) if bad else ''}")
    safe("LIST", c_list)

    def c_counts():
        s = load()
        m = LINE.search(log(d, "v2"))
        got = tuple(int(x) for x in m.groups()[1:]) if m else None
        want = (sum(r["what"] == "answer" for r in s["q"]),
                sum(r["what"] == "proxy" for r in s["q"]),
                sum(r["what"] == "suppress" for r in s["q"]),
                st.get("reduced", -1),
                len(s["resp"]),
                sum(r["entryNode"] != "0" for r in s["resp"]))
        check("COUNTS", got == want,
              f"NW-9 line {got}; recounted {want} (answered, proxy, suppressed, reduced, heard, "
              f"with an entry)")
    safe("COUNTS", c_counts)

    def c_digest():
        dg = open(os.path.join(V, "run_id.txt")).read()
        bad = []
        for f in ("responses_ca_tbl_full_seed7.csv", "queryv2_ca_tbl_full_seed7.csv"):
            h = hashlib.sha256(open(os.path.join(V, f), "rb").read()).hexdigest()
            if f"{h}  {f}" not in dg:
                bad.append(f)
        check("DIGEST", not bad, f"not digested as written: {bad or 'none'}")
    safe("DIGEST", c_digest)

    def c_ref():
        for n, need in (("r_noq", "FATAL: --queryV2 needs --identityQueryPeriodS > 0"),
                        ("r_list", "FATAL: --identityQueryManifests needs --queryV2"),
                        ("r_bad", "FATAL: --identityQueryManifests takes ids 1..255")):
            got = rc(d, n)
            has = need in log(d, n)
            check("R", got == 1 and has, f"{n}: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G50 {tag:7s} {name:7s} {why}")
    print(f"  G50: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
