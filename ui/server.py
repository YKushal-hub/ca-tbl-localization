#!/usr/bin/env python3
"""CA-TBL layout UI server.

This file only WIRES the existing simulator to a browser page. It does not model,
re-derive or alter anything: every position, packet and flag shown in the UI is read
from the files the unmodified simulator (scratch/ca_tbl_causal_sim.cc) writes.

What it does:
  POST /api/run      writes the user's gateway layout to an x,y CSV and launches
                     the built simulator with --gwLayout=file --gwLayoutFile=<csv>
  GET  /api/poll     tails the run's output CSVs while the simulator is still
                     writing them, and returns any newly completed cycles
  POST /api/stop     stops a running simulation
  GET  /             serves ui/index.html

Standard library only. Run from anywhere:
    python3 ui/server.py [--port 8765]
"""
import argparse
import csv
import io
import json
import os
import shutil
import subprocess
import threading
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from urllib.parse import parse_qs, urlparse

UI_DIR = os.path.dirname(os.path.abspath(__file__))
REPO = os.path.dirname(UI_DIR)
RUNS_DIR = os.path.join(UI_DIR, "runs")  # NOT results/: that tree is tracked evidence
SIM = os.path.join(REPO, "build", "scratch", "ns3.47-ca_tbl_causal_sim-optimized")
ARM = "ca_tbl_full"  # the system arm; one arm instead of all five (see demo/run_demo.sh)
MIN_GATEWAYS = 3
SIM_DEFAULT_SPEED_MPS = 1.2  # the scenario's --speed default (walkSpeedMps), docs/SCENARIO_FLAGS.md

# The id ranges the scenario assigns (README "The deployment"): the only place role is expressed.
def role_of(nid):
    if 101 <= nid <= 199:
        return "gateway"
    if nid >= 501:
        return "finder"
    if nid == 1:
        return "missing"
    return "peer"


TIERS = {"868MHz": 0, "2p4GHz": 1, "2.4GHz": 1, "UWB": 2, "0": 0, "1": 1, "2": 2}
OUTCOME_OK = {"delivered"}  # capture and collision are failures


def tier_index(v):
    return TIERS.get((v or "").strip(), -1)


class Tail:
    """Incrementally read complete CSV rows from a file another process is writing."""

    def __init__(self, path):
        self.path = path
        self.pos = 0
        self.buf = ""
        self.header = None

    def read(self):
        if not os.path.exists(self.path):
            return []
        with open(self.path, "r", encoding="utf-8", errors="replace") as f:
            f.seek(self.pos)
            chunk = f.read()
            self.pos = f.tell()
        if not chunk:
            return []
        self.buf += chunk
        lines = self.buf.split("\n")
        self.buf = lines.pop()  # the last piece may be a half-written line
        rows = []
        for ln in lines:
            if not ln.strip():
                continue
            if self.header is None:
                self.header = next(csv.reader([ln]))
                continue
            vals = next(csv.reader(io.StringIO(ln)))
            rows.append(dict(zip(self.header, vals)))
        return rows


class Run:
    def __init__(self, rid, outdir, cmd, dt):
        self.rid = rid
        self.outdir = outdir
        self.cmd = cmd
        self.dt = dt
        self.lock = threading.Lock()
        self.proc = None
        self.log = open(os.path.join(outdir, "run.log"), "w")
        sfx = "%s_seed%s.csv" % (ARM, cmd_seed(cmd))
        self.tails = {k: Tail(os.path.join(outdir, "%s_%s" % (k, sfx)))
                      for k in ("trace", "nodes", "events", "loctable")}
        self.cycles = {}  # ci -> {"c": ci, "t":, "trace":, "pos": {}, "pk": [], "loc": {}}
        # the highest cycle each file has reached; a cycle is complete only when ALL files are past it
        self.max_ci = {k: -1 for k in self.tails}
        self.deployment = None
        self.error = None
        self.found_ = None
        # --vizFixSet rows (only on cycles with a fix), sent as they arrive rather than per cycle,
        # so a stretch without fixes never holds the cycles back
        self.fix_tail = Tail(os.path.join(outdir, "fixset_%s" % sfx))
        self.fix_new = {}

    def start(self):
        self.proc = subprocess.Popen(self.cmd, cwd=REPO, stdout=self.log, stderr=subprocess.STDOUT)

    def cyc(self, ci):
        c = self.cycles.get(ci)
        if c is None:
            c = {"c": ci, "trace": None, "pos": {}, "pk": [], "loc": {}}
            self.cycles[ci] = c
        return c

    def ingest(self):
        with self.lock:
            if self.deployment is None:
                p = os.path.join(self.outdir, "deployment.json")
                if os.path.exists(p):
                    try:
                        self.deployment = json.load(open(p))
                    except ValueError:
                        pass  # still being written
            dt = self.dt
            for r in self.tails["nodes"].read():
                ci = int(round(float(r["t"]) / dt))
                self.cyc(ci)["pos"][r["nodeId"]] = [round(float(r["x"]), 2), round(float(r["y"]), 2)]
                self.max_ci["nodes"] = max(self.max_ci["nodes"], ci)
            for r in self.tails["trace"].read():
                ci = int(round(float(r["t"]) / dt))
                self.cyc(ci)["trace"] = {
                    "t": float(r["t"]),
                    "activeTier": tier_index(r.get("activeTier")),
                    "fixTier": tier_index(r.get("fixTier")),
                    "trueX": float(r["trueX"]), "trueY": float(r["trueY"]),
                    "estX": float(r["estX"]), "estY": float(r["estY"]),
                    "errM": float(r["errM"]),
                    "sigM": round(max(0.0, float(r.get("traceSigma") or 0)) ** 0.5, 1),
                    "hadFix": int(r["hadFix"]),
                    "escalated": int(r["escalated"]),
                    "deescalated": int(r["deescalated"]),
                }
                self.max_ci["trace"] = max(self.max_ci["trace"], ci)
            for r in self.tails["events"].read():
                ci = int(float(r["t"]) / dt)
                self.cyc(ci)["pk"].append({
                    "t": float(r["t"]), "tx": int(r["txId"]), "rx": int(r["rxId"]),
                    "tier": int(r["tier"]), "type": r["msgType"], "outcome": r["outcome"],
                    "ok": r["outcome"] in OUTCOME_OK,
                    "rxDbm": round(float(r["rxPowerDbm"]), 1), "payload": r.get("payload", ""),
                })
                self.max_ci["events"] = max(self.max_ci["events"], ci)
            for r in self.fix_tail.read():
                ci = int(round(float(r["t"]) / dt))
                self.fix_new[ci] = [[int(a), int(b)] for a, b in
                                    (x.split(":") for x in r["nodes"].split(";") if x)]
            for r in self.tails["loctable"].read():
                ci = int(round(float(r["t"]) / dt))
                self.cyc(ci)["loc"].setdefault(r["nodeId"], []).append({
                    "entry": int(r["entryId"]), "x": float(r["x"]), "y": float(r["y"]),
                    "ageS": float(r["ageS"]), "fresh": r["freshness"],
                })
                self.max_ci["loctable"] = max(self.max_ci["loctable"], ci)

    def done(self):
        return self.proc is not None and self.proc.poll() is not None

    def poll(self, since):
        # Exit status FIRST, then read: a process that ends between the two would otherwise have
        # its last rows unread while every cycle is declared complete.
        finished = self.done()
        self.ingest()
        with self.lock:
            # Each output file is written in its own buffered chunks, so the trace can be cycles
            # ahead of the positions file. A cycle is sent once EVERY file has moved past it (or
            # the run ended); sending on the trace alone lost search-node positions mid-run.
            limit = max(self.cycles) if (finished and self.cycles) else min(self.max_ci.values()) - 1
            out = [self.cycles[ci] for ci in sorted(self.cycles) if since < ci <= limit]
            dep = self.deployment
            fixes, self.fix_new = self.fix_new, {}
        tail = ""
        if finished:
            try:
                tail = open(os.path.join(self.outdir, "run.log")).read()[-3000:]
            except OSError:
                pass
        return {
            "cycles": out, "deployment": dep, "finished": finished,
            "exit": self.proc.returncode if finished else None, "logTail": tail,
            "found": self.found(), "fixes": fixes, "anchors": self.anchors(),
        }

    def anchors(self):
        """Anchor assignments and releases (--finderAnchors), from the run log: [[t, finder, on]]."""
        out = []
        try:
            with open(os.path.join(self.outdir, "run.log"), encoding="utf-8", errors="replace") as f:
                for ln in f:
                    if ln.startswith("UI ANCHOR"):
                        kv = dict(p.split("=", 1) for p in ln.split()[2:] if "=" in p)
                        out.append([float(kv["t"]), int(kv["finder"]), ln.startswith("UI ANCHOR ")])
        except (OSError, KeyError, ValueError):
            pass
        return out

    def found(self):
        """The simulator's one 'UI FOUND t=.. finder=.. uwbRangeM=..' line (--finderContactM), if any."""
        if self.found_ is not None:
            return self.found_
        try:
            with open(os.path.join(self.outdir, "run.log"), encoding="utf-8", errors="replace") as f:
                for ln in f:
                    if ln.startswith("UI FOUND "):
                        kv = dict(p.split("=", 1) for p in ln.split()[2:] if "=" in p)
                        self.found_ = {"t": float(kv["t"]), "finder": int(kv["finder"]),
                                       "uwbRangeM": float(kv["uwbRangeM"])}
                        break
        except (OSError, KeyError, ValueError):
            pass
        return self.found_


def cmd_seed(cmd):
    for a in cmd:
        if a.startswith("--seed="):
            return a.split("=", 1)[1]
    return "7"


RUNS = {}

# GATEWAY RANGE, for drawing: the simulator's own range test (--probe) sends thousands of packets
# per tier at set distances in the scenario's default environment and reports where 80 % and 50 %
# of them still arrive ("usable range: r80=... r50=..."). Run once per simulator build, cached.
RANGES = {"state": "pending"}


def probe_ranges():
    global RANGES
    if not os.path.exists(SIM):
        RANGES = {"state": "error", "error": "simulator not built"}
        return
    out = os.path.join(RUNS_DIR, "probe")
    cache = os.path.join(out, "ranges.json")
    stamp = os.path.getmtime(SIM)
    try:
        c = json.load(open(cache))
        if c.get("simMtime") == stamp:
            RANGES = c
            return
    except (OSError, ValueError):
        pass
    os.makedirs(out, exist_ok=True)
    try:
        res = subprocess.run([SIM, "--probe=true", "--outdir=%s" % out], cwd=REPO,
                             capture_output=True, text=True, timeout=600)
        rows = []
        for ln in res.stdout.splitlines():
            ln = ln.strip()
            if ln.startswith("usable range:"):
                kv = dict(p.split("=", 1) for p in ln.split()[2:] if "=" in p)
                rows.append({"r80": float(kv["r80"].rstrip("m")), "r50": float(kv["r50"].rstrip("m"))})
        if len(rows) < 3:
            raise ValueError("expected 3 'usable range' lines, found %d" % len(rows))
        RANGES = {"state": "ok", "simMtime": stamp, "source": "--probe",
                  "tiers": {"868": rows[0], "2p4": rows[1], "uwb": rows[2]}}
        json.dump(RANGES, open(cache, "w"))
    except (OSError, ValueError, KeyError, subprocess.TimeoutExpired) as e:
        RANGES = {"state": "error", "error": str(e)}


def start_run(body):
    gws = body.get("gateways") or []
    # The simulator's own rule (ca_tbl_causal_sim.cc: "at least 3 anchors per site are needed
    # for a 2D fix"; a fix steps on >= 3 ranges). --gwLayout=file itself accepts fewer.
    if len(gws) < MIN_GATEWAYS:
        raise ValueError("at least %d gateways are needed for a 2D fix; %d placed" % (MIN_GATEWAYS, len(gws)))
    missing = body.get("missing")
    if not os.path.exists(SIM):
        raise ValueError("simulator binary not built: %s" % SIM)
    box = float(body.get("boxHalf", 340))
    dur = float(body.get("duration", 600))
    seed = int(body.get("seed", 7))
    dt = 2.5  # the scenario's --dt default; not exposed, so it cannot drift from the binary
    rid = time.strftime("%Y%m%d-%H%M%S")
    outdir = os.path.join(RUNS_DIR, rid)
    os.makedirs(outdir, exist_ok=True)
    layout = os.path.join(outdir, "gateways.csv")
    with open(layout, "w") as f:
        f.write("x,y\n")
        for g in gws:
            f.write("%.3f,%.3f\n" % (float(g["x"]), float(g["y"])))
    cmd = [SIM,
           "--arms=%s" % ARM, "--seed=%d" % seed, "--duration=%g" % dur,
           "--boxHalf=%g" % box,
           "--gwLayout=file", "--gwLayoutFile=%s" % layout,
           "--trace=true", "--vizEvents=true", "--vizTablePeriodS=%g" % dt,
           "--outdir=%s" % outdir]
    # BASE CAMP (--baseCampFrac, existing): the camp is at (f*boxHalf, f*boxHalf); search nodes
    # start there and mode 2 sweeps out from it as the last known position.
    camp_frac = float(body.get("baseCampFrac", 0) or 0)
    if not (0.0 <= camp_frac < 1.0):
        raise ValueError("base camp fraction must be in [0, 1)")
    if camp_frac > 0:
        cmd += ["--baseCampFrac=%g" % camp_frac]
    # A DRAGGED CAMP (--baseCampX/--baseCampY, added for the UI; --baseCampFrac still switches
    # the camp on): the camp, and so the searchers' start, at this point instead.
    camp_pos = body.get("campPos")
    camp_xy = [camp_frac * box, camp_frac * box] if camp_frac > 0 else None
    if camp_pos:
        if camp_frac <= 0:
            raise ValueError("a dragged camp needs the base camp switched on")
        cx, cy = float(camp_pos["x"]), float(camp_pos["y"])
        if abs(cx) > box or abs(cy) > box:
            raise ValueError("the base camp must lie inside the map")
        cmd += ["--baseCampX=%.3f" % cx, "--baseCampY=%.3f" % cy]
        camp_xy = [cx, cy]
    # WHERE THE ESTIMATE STARTS. "camp": the simulator's base-camp default, the last known
    # position (its starting sigma is sized from the true distance, the simulator's own rule).
    # "unknown": --unknownStart, the box centre with a box-sized sigma.
    prior = body.get("prior", "unknown")
    if prior == "camp" and camp_frac <= 0:
        raise ValueError("'last known = camp' needs a base camp")
    if prior != "camp":
        cmd += ["--unknownStart=true"]
    n_finders = int(body.get("nFinders", 0))
    if n_finders < 0 or n_finders > 388:  # ids 501+, and 101-199 / 1-5 are taken by other roles
        raise ValueError("search nodes must be between 0 and 388")
    relays = int(body.get("finderRelays", 0) or 0)
    # LAST KNOWN POSITION (--lkpX/--lkpY): where the missing node was last seen. The estimate's
    # prior is centred on it (with the base-camp prior), and until the party holds a location
    # over the air the searchers head for it (the briefing).
    lkp = body.get("lkp")
    if lkp:
        if camp_frac <= 0:
            raise ValueError("a last known position needs a base camp (the search starts there)")
        cmd += ["--lkpX=%.3f" % float(lkp["x"]), "--lkpY=%.3f" % float(lkp["y"])]
    party_share = n_finders > 0 and body.get("partyShare", True) is not False
    if n_finders > 0:
        # THE SEARCH: whenever the party holds a location for the missing node, every searcher
        # moves toward it and re-targets the moment a newer one arrives. All of it is the
        # simulator's own except --finderPartyShare:
        #   --finderMode=1           converge on the location held (no sweep first);
        #   --finderDispersal=1.0    each searcher heads for its own point on the location's
        #                            1-sigma circle, at its own bearing: toward it, not in a cluster;
        #   --finderStaleSteerBound=0  an older location keeps steering until a newer one arrives;
        #   --finderPartyShare       a searcher steers on the freshest location ANY searcher holds,
        #                            so every searcher moves (without it some never receive one);
        #   and the search settings of demo/run_demo.sh (--sendHandshakeFrames in particular:
        #   without it no search node ever polls for entries).
        # PARTY SHARE is the page's switch (on by default): off, each searcher steers on its own
        # table entry, received over the radio -- the simulator's original behaviour.
        if party_share:
            cmd += ["--finderPartyShare=true"]
        cmd += ["--nFinders=%d" % n_finders, "--finderPseudoAnchor=true", "--finderMode=1",
                "--finderStaleSteerBound=0", "--finderDispersal=1.0",
                "--sendHandshakeFrames=true", "--finderDigestPeriodS=20",
                # FOUND -> HOLD (measured on Scenarios 1 and 2, seeds 7-11; ui/README.md):
                #   --rangeReplyPosition   a searcher's ranging reply carries where it is NOW, so
                #                          the missing node's fix is not anchored to positions
                #                          minutes old (the simulator's own option, off by default);
                #   --uwbEvidenceFromNeighbours  UWB may switch on when a searcher is ranged within
                #                          20 m, instead of needing 3 gateways there (the
                #                          simulator's own option, off by default);
                #   --finderHoldM=15       a searcher with a direct 2.4 GHz / UWB range of at most
                #                          15 m to the missing node stops following broadcast
                #                          locations and stays, until no such range for 60 s.
                "--rangeReplyPosition=true", "--uwbEvidenceFromNeighbours=true", "--finderHoldM=15",
                # CONTACT STEP: while the party's location is within 2 m (1-sigma), the nearest
                # holder walks straight to it; FOUND when its own direct UWB range is <= 2 m.
                "--finderContactM=2",
                # STOPPING CONDITION: the search ends at FOUND (a searcher's own direct UWB range to
                # the missing node within 2 m); otherwise the run goes to its full duration
                "--stopOnFound=true",
                # ANCHORS AND VERIFIERS (measured: found in 8 of 10 seeds against 5 of 10, and
                # sooner; ui/README.md): the first 3 searchers to range the missing node within 15 m
                # stay put as anchors; every other close searcher walks straight in until FOUND
                "--finderAnchors=3"]
    # OUTPUT ONLY: the nodes each position fix was computed from (the page highlights them)
    cmd += ["--vizFixSet=true"]
    # RADIO LINKS (--meshLinks, the simulator's own option): "star" (default) links each field node
    # to the anchors only; "all" links every pair. Measured (ui/README.md): "all" triples the traffic
    # and does worse, and a full-mesh run needs ~1-3 GB of memory and ~20 min.
    mesh = body.get("meshLinks", "star")
    if mesh not in ("star", "all"):
        raise ValueError("radio links must be 'star' or 'all'")
    if mesh == "all":
        cmd += ["--meshLinks=all"]
    # MULTI-HOP (--digestSummary, the simulator's own option, off by default): a DIGEST carries
    # {node id, sequence number} for every entry the asker holds, and the answer pushes the entry
    # the asker lacks or holds older -- so a location can be passed on, node to node. Off: the
    # answer is always the responder's own entry, so a location only ever travels one hop.
    multi_hop = bool(body.get("multiHop"))
    if multi_hop:
        cmd += ["--digestSummary=true", "--finderDigestSummary=true"]
    # GATEWAYS ASK (--gatewayDigestPeriodS, added for the UI, 0 = off): each gateway sends a
    # DIGEST every 20 s (the searchers' period), so nodes holding a newer location push it to
    # the gateway. Without it gateways only answer, and nothing is ever pushed to them.
    gw_ask = bool(body.get("gatewayAsk"))
    if gw_ask:
        cmd += ["--gatewayDigestPeriodS=20"]
    if relays:
        if camp_frac <= 0 or relays < 0 or relays >= n_finders:
            raise ValueError("stay-behind relays need a base camp and fewer relays than search nodes")
        cmd += ["--finderRelays=%d" % relays]
    if missing:
        # placed by the user and held still there (--speed is the scenario's walking speed)
        cmd += ["--subjectStartX=%.3f" % float(missing["x"]), "--subjectStartY=%.3f" % float(missing["y"]),
                "--speed=0"]
        if n_finders > 0:
            # --speed also sets the searchers' speed unless --finderSpeed is given (the
            # simulator's own note: --speed=0 "pinned all four searchers to the spot").
            # Keep them at the simulator's default walking speed.
            cmd += ["--finderSpeed=%g" % SIM_DEFAULT_SPEED_MPS]
    run = Run(rid, outdir, cmd, dt)
    run.config = {"boxHalf": box, "duration": dur, "nFinders": n_finders, "finderRelays": relays,
                  "baseCampFrac": camp_frac, "camp": camp_xy,
                  "prior": prior, "partyShare": party_share, "meshLinks": mesh, "multiHop": multi_hop, "gatewayAsk": gw_ask,
                  "lkp": [float(lkp["x"]), float(lkp["y"])] if lkp else None}
    run.start()
    RUNS[rid] = run
    return {"run": rid, "cmd": " ".join(cmd), "config": run.config}


class H(BaseHTTPRequestHandler):
    def log_message(self, *a):
        pass

    def send_json(self, obj, code=200):
        b = json.dumps(obj, separators=(",", ":")).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json")
        self.send_header("Content-Length", str(len(b)))
        self.end_headers()
        self.wfile.write(b)

    def do_GET(self):
        u = urlparse(self.path)
        if u.path in ("/", "/index.html"):
            b = open(os.path.join(UI_DIR, "index.html"), "rb").read()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(b)))
            self.end_headers()
            self.wfile.write(b)
            return
        if u.path == "/api/ranges":
            self.send_json(RANGES)
            return
        if u.path == "/api/status":
            self.send_json({"simBuilt": os.path.exists(SIM), "sim": SIM, "minGateways": MIN_GATEWAYS})
            return
        if u.path == "/api/poll":
            q = parse_qs(u.query)
            run = RUNS.get(q.get("run", [""])[0])
            if not run:
                self.send_json({"error": "unknown run"}, 404)
                return
            self.send_json(run.poll(int(q.get("since", ["-1"])[0])))
            return
        self.send_json({"error": "not found"}, 404)

    def do_POST(self):
        u = urlparse(self.path)
        n = int(self.headers.get("Content-Length") or 0)
        body = json.loads(self.rfile.read(n) or b"{}")
        try:
            if u.path == "/api/run":
                self.send_json(start_run(body))
            elif u.path == "/api/stop":
                run = RUNS.get(body.get("run", ""))
                if run and run.proc and run.proc.poll() is None:
                    run.proc.terminate()
                self.send_json({"ok": True})
            else:
                self.send_json({"error": "not found"}, 404)
        except ValueError as e:
            self.send_json({"error": str(e)}, 400)


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--port", type=int, default=8765)
    a = ap.parse_args()
    os.makedirs(RUNS_DIR, exist_ok=True)
    print("CA-TBL UI on http://localhost:%d  (simulator %s)" % (a.port, "found" if os.path.exists(SIM) else "NOT BUILT"))
    threading.Thread(target=probe_ranges, daemon=True).start()
    ThreadingHTTPServer(("127.0.0.1", a.port), H).serve_forever()


if __name__ == "__main__":
    main()
