#!/usr/bin/env python3
"""G192 (--finderDigestBothLora, DD 78). Checks, each printed as "G192 PASS|FAIL <name> ...":
  RUNS     every run rc 0
  OFFSAME  the explicit default equals the default, every csv byte for byte
  LINES    a DD78 line per arm in "on" only; "on" for ca_tbl_full and fixed_threshold, "unused" for static_2p4 and static_868
  SINGLE   static_2p4 and static_868 identical in off and on, every csv
  POLLS    on: ca_tbl_full's searchers polled on 2.4 GHz and read entries from the 2.4 GHz table (both > 0)
  EFFECT   on: ca_tbl_full's output differs from off, and its searchers' casualty entry is younger at the end of the run
usage: finder_both_lora_gate.py <dir>"""
import filecmp, glob, os, re, sys


def files(d, pat="*.csv"):
    return sorted(os.path.basename(f) for f in glob.glob(os.path.join(d, pat)))


def same(d, a, b, pat="*.csv"):
    fa = files(os.path.join(d, a), pat)
    return len(fa) > 0 and fa == files(os.path.join(d, b), pat) and all(
        filecmp.cmp(os.path.join(d, a, f), os.path.join(d, b, f), shallow=False) for f in fa), len(fa)


def age(log, arm):
    # the TABLE line follows its arm's stale-entry line
    m = re.search(r"stale-entry search \(%s\):.*?TABLE: \d+/\d+ finders hold the casualty entry, mean age ([\d.]+) s" % arm, log, re.S)
    return float(m[1]) if m else None


def main():
    d = sys.argv[1]
    bad = []

    def rep(name, ok, msg=""):
        print(f"G192 {'PASS' if ok else 'FAIL'} {name} {msg}")
        if not ok:
            bad.append(name)

    names = ("off", "false", "on")
    rcs = {n: (open(os.path.join(d, n + ".rc")).read().strip() if os.path.exists(os.path.join(d, n + ".rc")) else "missing") for n in names}
    rep("RUNS", all(v == "0" for v in rcs.values()), str(rcs))
    ok, n = same(d, "off", "false")
    rep("OFFSAME", ok, f"{n} csv")
    lg = {k: open(os.path.join(d, k + ".log"), errors="replace").read() if os.path.exists(os.path.join(d, k + ".log")) else "" for k in names}
    lines = dict(re.findall(r"DD78 finderDigestBothLora \((\w+)\): (on(?=,)|one LoRa radio, unused)", lg["on"]))
    rep("LINES", lines.get("ca_tbl_full") == "on" and lines.get("fixed_threshold") == "on" and lines.get("static_2p4", "").endswith("unused")
        and lines.get("static_868", "").endswith("unused") and "DD78" not in lg["off"] and "DD78" not in lg["false"], str(lines))
    s2, n2 = same(d, "off", "on", "*static_2p4*.csv"); s8, n8 = same(d, "off", "on", "*static_868*.csv")
    rep("SINGLE", s2 and s8, f"{n2} + {n8} csv")
    m = re.search(r"DD78 finderDigestBothLora \(ca_tbl_full\): on, searcher polls on 2.4 GHz (\d+), entries read from the 2.4 GHz table (\d+)", lg["on"])
    rep("POLLS", bool(m) and int(m[1]) > 0 and int(m[2]) > 0, m[0] if m else "no line")
    diff = not same(d, "off", "on", "*ca_tbl_full*.csv")[0]
    a0, a1 = age(lg["off"], "ca_tbl_full"), age(lg["on"], "ca_tbl_full")
    rep("EFFECT", diff and a0 is not None and a1 is not None and a1 < a0, f"ours differs {diff}, casualty entry age {a0} -> {a1} s")
    print(f"G192 {'PASS' if not bad else 'FAIL'} overall {' '.join(bad)}")
    return 0 if not bad else 1


if __name__ == "__main__":
    sys.exit(main())
