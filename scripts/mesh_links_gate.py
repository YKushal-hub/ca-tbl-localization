#!/usr/bin/env python3
"""G45: real mesh links (docs/SPEC_CONFORMANCE_PLAN.md NW-0, --meshLinks=all).

The spec's network is a "sub-GHz LoRa mesh": QUERY floods it, proxies answer from any node,
RESPONSE travels hop by hop, anchors gossip to neighbouring anchors. The PHY provider resolved
a link only between a field node and an anchor, so the flood was star-plus-anchor-reflection.
Under --meshLinks=all every registered pair on a tier both carry is a link; DISCOVERY is
one-hop; the ranging poll addresses anchors only; the measured node's sleeping radios neither
receive nor relay; its relays are billed.

Reads a directory written by scripts/mesh_links_gate.sh (def, star, all; r_bad). The events
file has one row per reception; a frame's originator is its packet id >> 20 (ids are
node_id << 20 plus a count). Every check runs and is reported; exit 1 if any failed, 2 if an
input is missing.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9):
  NULL      --meshLinks=star IS the default: every file byte-identical, no NW-0 line.
  PAIRS     under all, frames are delivered finder -> gateway, gateway -> gateway and
            tag -> tag (> 0 each); under star none of the three is.
  DISCOVERY under all no DISCOVERY is rebroadcast (transmitter = originator on every
            DISCOVERY reception) and every one left with hops=0 -- a limit of 1 is still
            relayed once; under star some are relayed.
  ASLEEP    under all the measured node (id 1) receives nothing on a radio that was asleep
            in that slice -- the slice after cycle i belongs to cycle i's nextTier, plus
            868 MHz whenever the hybrid arm is off 868 -- and relays nothing there; and the guard fired (> 0
            receptions refused asleep). The channel skips a sleeping radio before evaluating
            or logging, so an events row at node 1 on a sleeping tier is a reception that
            happened.
  DEDUP     no frame is relayed by more distinct nodes than exist (12: 5 gateways, 4
            finders, 3 tags) -- the flood terminates.
  BILLED    the node's relays are billed: frames billed >= its distinct relayed frames heard
            anywhere, and > 0; and the poll guard fired (peers kept out of poll sets > 0).
  R         an unknown value is refused (exit 1).
"""
import bisect
import csv
import os
import re
import sys

LINE = re.compile(r"NW-0 mesh: link evaluations (\d+) \(mesh pairs (\d+)\), peers kept out of poll "
                  r"sets (\d+), receptions refused asleep (\d+), node rebroadcasts billed (\d+) "
                  r"frames ([0-9.eE+-]+) s")
NODES = 12
TIERS = {"868MHz": 0, "2.4GHz": 1, "UWB": 2}


def rc(d, n):
    with open(os.path.join(d, n + ".rc")) as f:
        return int(f.read().strip())


def log(d, n):
    with open(os.path.join(d, n + ".log")) as f:
        return f.read()


def events(d, n):
    out = []
    for r in csv.DictReader(open(os.path.join(d, n, "events_ca_tbl_full_seed7.csv"))):
        m = re.search(r"pkt=(\d+)", r["payload"])
        r["pkt"] = int(m.group(1)) if m else -1
        r["orig"] = r["pkt"] >> 20
        r["tx"] = int(r["txId"])
        r["rx"] = int(r["rxId"])
        r["t"] = float(r["t"])
        out.append(r)
    return out


def cls(tx, rx):
    if tx >= 501 and 101 <= rx <= 199:
        return "finder->gw"
    if 101 <= tx <= 199 and 101 <= rx <= 199:
        return "gw->gw"
    if tx < 100 and rx < 100:
        return "tag->tag"
    return None


def main():
    if len(sys.argv) != 2:
        print("usage: mesh_links_gate.py <dir>", file=sys.stderr)
        return 2
    d = sys.argv[1]
    res = []

    def check(name, ok, why):
        res.append((name, bool(ok), why))

    def safe(name, fn):
        try:
            fn()
        except (OSError, ValueError, KeyError, IndexError, ZeroDivisionError, TypeError,
                AttributeError) as e:
            res.append((name, None, f"input missing or malformed: {e}"))

    def c_runs():
        bad = [n for n in ("def", "star", "all") if rc(d, n) != 0]
        check("RUNS", not bad, "every run exited 0" if not bad else f"non-zero exit: {bad}")
    safe("RUNS", c_runs)

    def c_null():
        fd = sorted(f for f in os.listdir(os.path.join(d, "def")) if f != "run_id.txt")
        fs = sorted(f for f in os.listdir(os.path.join(d, "star")) if f != "run_id.txt")
        same = fd == fs and all(open(os.path.join(d, "def", f), "rb").read() ==
                                open(os.path.join(d, "star", f), "rb").read() for f in fd)
        lines = log(d, "def").count("NW-0")
        check("NULL", same and fd and lines == 0,
              f"--meshLinks=star vs default: {len(fd)} files, {'identical' if same else 'DIFFER'}; "
              f"NW-0 lines {lines} (need 0)")
    safe("NULL", c_null)

    def c_pairs():
        got = {}
        for n in ("star", "all"):
            c = {"finder->gw": 0, "gw->gw": 0, "tag->tag": 0}
            for r in events(d, n):
                k = cls(r["tx"], r["rx"])
                if k and r["outcome"] in ("delivered", "capture"):
                    c[k] += 1
            got[n] = c
        ok = all(v > 0 for v in got["all"].values()) and all(v == 0 for v in got["star"].values())
        check("PAIRS", ok, f"delivered frames by class: star {got['star']} (need all 0), all "
                           f"{got['all']} (need all > 0)")
    safe("PAIRS", c_pairs)

    def c_disc():
        relayed = {}
        hops_bad = 0
        for n in ("star", "all"):
            ds = [r for r in events(d, n) if r["msgType"] == "DISCOVERY"]
            relayed[n] = sum(1 for r in ds if r["tx"] != r["orig"])
            if n == "all":
                hops_bad = sum(1 for r in ds if ";hops=0" not in r["payload"] + ";")
        check("DISCOVERY", relayed["all"] == 0 and hops_bad == 0 and relayed["star"] > 0,
              f"DISCOVERY receptions relayed: all {relayed['all']} (need 0), star "
              f"{relayed['star']} (need > 0); under all, not hops=0 {hops_bad} (need 0)")
    safe("DISCOVERY", c_disc)

    def c_asleep():
        tr = list(csv.DictReader(open(os.path.join(d, "all", "trace_ca_tbl_full_seed7.csv"))))
        ts = [float(r["t"]) for r in tr]
        # The slice after cycle i belongs to the tier cycle i chose for the next cycle
        # (nextTier), plus 868 MHz (ca_tbl_full is hybrid: the floor stays awake off 868).
        # Before the first cycle the node is on 868 MHz alone.
        awake = [{TIERS[r["nextTier"]], 0} for r in tr]
        bad_rx, bad_tx = [], []
        for r in events(d, "all"):
            i = bisect.bisect_right(ts, r["t"] - 1e-9) - 1
            aw = awake[i] if i >= 0 else {0}
            tier = int(r["tier"])
            if r["rx"] == 1 and tier not in aw:
                bad_rx.append((r["t"], tier))
            if r["tx"] == 1 and r["orig"] != 1 and tier not in aw:
                bad_tx.append((r["t"], tier))
        m = LINE.search(log(d, "all"))
        refused = int(m.group(4)) if m else 0
        check("ASLEEP", not bad_rx and not bad_tx and refused > 0,
              f"all: node-1 receptions on a sleeping radio {len(bad_rx)}, relays from one "
              f"{len(bad_tx)} (need 0, 0); receptions refused asleep {refused} (need > 0)"
              f"{': ' + str((bad_rx + bad_tx)[:3]) if bad_rx or bad_tx else ''}")
    safe("ASLEEP", c_asleep)

    def c_dedup():
        relayers = {}
        for r in events(d, "all"):
            if r["tx"] != r["orig"]:
                relayers.setdefault((r["tier"], r["pkt"]), set()).add(r["tx"])
        worst = max((len(v) for v in relayers.values()), default=0)
        check("DEDUP", worst <= NODES,
              f"all: most distinct relays of one frame {worst} (need <= {NODES}) over "
              f"{len(relayers)} relayed frames")
    safe("DEDUP", c_dedup)

    def c_billed():
        m = LINE.search(log(d, "all"))
        billed = int(m.group(5)) if m else -1
        skipped = int(m.group(3)) if m else 0
        heard = len({(r["tier"], r["pkt"]) for r in events(d, "all") if r["tx"] == 1 and r["orig"] != 1})
        check("BILLED", m and heard > 0 and billed >= heard and skipped > 0,
              f"all: node-1 relayed frames heard {heard} (need > 0), billed {billed} (need >= "
              f"heard); peers kept out of poll sets {skipped} (need > 0)")
    safe("BILLED", c_billed)

    def c_ref():
        got = rc(d, "r_bad")
        has = "--meshLinks must be star or all" in log(d, "r_bad")
        check("R", got == 1 and has, f"r_bad: exit {got} (need 1), reason {'present' if has else 'ABSENT'}")
    safe("R", c_ref)

    bad = sum(ok is False for _, ok, _ in res)
    missing = sum(ok is None for _, ok, _ in res)
    for name, ok, why in res:
        tag = "ok  " if ok else ("MISSING" if ok is None else "FAIL")
        print(f"  G45 {tag:7s} {name:9s} {why}")
    print(f"  G45: {len(res) - bad - missing} ok, {bad} failed, {missing} missing input")
    if missing:
        return 2
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
