#!/usr/bin/env python3
"""L.1.2 / G17 and G17b -- relay depth 2 (Network sect 5): the chain is wired, obeys A2,
and every depth it reports was earned.

WHY THIS EXISTS. CaTblComposeRelayChain, --maxRelayDepth and --relayVarBudget existed and
the scenario hard-set hopDepth = 1, so depth 2 could not arise and neither half of A2 ever
bound on the thing it exists for. A unit suite proves the admission rule; only a run can
prove anything reaches it. This gate reads what a run with --relayChain wrote.

WHAT A FIRING LOOKS LIKE ON DISK (rule 9). relay_chain_<arm>_seed<N>.csv, one row per
relayed-range decision on either leg (leg `relay`: a short relay ranging a fix-holder;
leg `tag`: the measured tag ranging a relay) plus one row per FUSE (leg `fuse`: a short
relay whose step fused a relayed range, carrying the depth it took):

    t,consumerId,relayId,leg,tier,hopDepth,maxRelayDepth,distM,carriedVarM2,
    inheritedVarM2,budgetM2,a2,verdict,sigmaRawM,sigmaEffM

The defects it guards against have two forms. ABSENCE -- the chain unwired, the flag
inert -- is caught by counting rows in a file a fresh run must produce. And DEPTH FROM
NOWHERE -- the first draft of L.1.2 marked a relay deeper when it merely ADMITTED a
relayed range, though below three measurements the filter coasts and fuses none of them;
it reported depth 2 hundreds of times in runs where no relay ever leaned on another. That
form is present data that looks right, so it is caught by provenance: a depth-d candidate
through relay R needs an earlier fuse row for R at depth d-1.

WHAT IT CHECKS (G17)
  1. The file exists, has the expected header, and at least one row (rows COUNTED).
  2. A2 on every decision row agrees with the row's own numbers: `ceiling` iff hopDepth is
     outside [1, maxRelayDepth]; else `budget` iff a budget is set and carriedVarM2 exceeds
     it; else `pass`. REJECT_DEPTH(A2) iff a2 is ceiling or budget; ADMIT only on pass.
     An A1 refusal or a refused composition never reaches A2.
  3. On every ADMIT, sigma_eff^2 - sigma_raw^2 lies in [0, carriedVarM2]: the projection
     u^T P u of a covariance is non-negative and at most its trace.
  4. Provenance, as above, in FILE ORDER -- the fuse must precede the use. A fuse row at
     depth d >= 2 is held to the same rule through the holder it leaned on.
  5. The BUDGET half of A2 observed both ways: at least one row passing A2 with a budget
     set, and at least one refused on the budget.

WITH --require-depth2 (G17b, the L.1.2 done-when)
  6. At least one fuse row, and at hopDepth == 2 at least one row passing A2 under a set
     budget and at least one refused on the budget.

WITH --stdout <log> (C5_INSTANT, --relaySlots: the run's own counters, recounted)
  Depth 2 became reachable only when --relaySlots gave the relays' round instants of its
  own; before it every relay exchange shared the cycle instant and none delivered. So a
  green G17b must be traced to the round, not merely co-occur with it (rule 4). The log
  must carry exactly one `C5_INSTANT relay round:` line and one `L.1.2 relay chain:` line,
  and:
  7. slot x slots = the round, and the round is SHORTER than the cycle -- a longer one would
     reach into the cycle before;
  8. anchor polls were moved (> 0, in 1..polls bursts): a round that moved nothing is the
     flag inert;
  9. relay-to-relay exchanges moved = the chain's relay-to-relay exchanges, every one, and
     the most used in a cycle does not exceed the slots reserved for them;
  10. the CSV's fuse rows = the chain's "fused it and held a fix" count.

usage: relay_chain_gate.py [--require-depth2] [--stdout <log>] <relay_chain csv>
exit 0 PASS, 1 FAIL, 2 usage.
"""
import csv
import re
import sys

ROUND_RE = re.compile(
    r"C5_INSTANT relay round: slot ([0-9.]+) ms x (\d+) = ([0-9.]+) ms before the instant, "
    r"of a ([0-9.]+) ms cycle; anchor polls moved (\d+) in (\d+) bursts; relay-to-relay "
    r"moved (\d+), at most (\d+) of (\d+) slots in a cycle")
CHAIN_RE = re.compile(
    r"L\.1\.2 relay chain: relay-to-relay exchanges (\d+) \(ranged (\d+)\), short relays "
    r"stepped (\d+) \(no exchange possible (\d+), admitted a relayed range (\d+), fused it "
    r"and held a fix (\d+)\)")


def check_stdout(g, log_path, n_fuse):
    """Checks 7-10. Returns True when the log's counters agree with the round and the CSV."""
    try:
        with open(log_path, errors="replace") as fh:
            text = fh.read()
    except OSError as e:
        print(f"{g} FAIL: cannot read stdout log {log_path}: {e}")
        return False
    rounds = ROUND_RE.findall(text)
    chains = CHAIN_RE.findall(text)
    if len(rounds) != 1 or len(chains) != 1:
        print(f"{g} FAIL: stdout has {len(rounds)} C5_INSTANT round line(s) and {len(chains)} "
              f"L.1.2 chain line(s), expected one of each -- was --relaySlots on, one arm?")
        return False
    slot, k, rnd, cyc, polls, bursts, moved, peak, reserved = rounds[0]
    slot, rnd, cyc = float(slot), float(rnd), float(cyc)
    k, polls, bursts, moved, peak, reserved = (int(k), int(polls), int(bursts), int(moved),
                                               int(peak), int(reserved))
    exchanges, fused = int(chains[0][0]), int(chains[0][5])
    ok = True
    if k < 1 or abs(slot * k - rnd) > 0.1 or not rnd < cyc:
        print(f"{g} FAIL: round {slot} ms x {k} = {rnd} ms against a {cyc} ms cycle -- it must "
              f"add up and end inside the cycle")
        ok = False
    if polls == 0 or bursts == 0 or bursts > polls:
        print(f"{g} FAIL: the round moved {polls} anchor polls in {bursts} bursts -- nothing "
              f"moved means the flag was inert")
        ok = False
    if moved != exchanges or peak > reserved:
        print(f"{g} FAIL: the round moved {moved} relay-to-relay exchanges of the chain's "
              f"{exchanges}, at most {peak} a cycle against {reserved} reserved")
        ok = False
    if n_fuse != fused:
        print(f"{g} FAIL: the CSV holds {n_fuse} fuse rows, the run reports {fused} fused")
        ok = False
    if ok:
        print(f"{g} stdout: round {k} x {slot} ms = {rnd} ms of a {cyc} ms cycle; anchor polls "
              f"moved {polls} in {bursts} bursts; relay-to-relay moved {moved} of {exchanges}; "
              f"fuse rows {n_fuse} = fused {fused}")
    return ok

COLS = ["t", "consumerId", "relayId", "leg", "tier", "hopDepth", "maxRelayDepth", "distM",
        "carriedVarM2", "inheritedVarM2", "budgetM2", "a2", "verdict", "sigmaRawM",
        "sigmaEffM"]

# The CSV prints six decimals. A carried variance within this of the budget cannot be
# placed on either side of it from the file, so the label is not second-guessed there.
TIE = 1e-5


def expected_a2(depth, ceiling, carried, budget):
    if depth < 1 or depth > ceiling:
        return "ceiling"
    if budget > 0.0:
        if abs(carried - budget) <= TIE:
            return None
        if carried > budget:
            return "budget"
    return "pass"


def main(argv):
    args = argv[1:]
    need2 = False
    log_path = None
    while args and args[0].startswith("--"):
        if args[0] == "--require-depth2":
            need2 = True
            args = args[1:]
        elif args[0] == "--stdout" and len(args) >= 2:
            log_path = args[1]
            args = args[2:]
        else:
            break
    if len(args) != 1:
        print("usage: relay_chain_gate.py [--require-depth2] [--stdout <log>] "
              "<relay_chain csv>")
        return 2
    g = "G17b" if need2 else "G17"
    path = args[0]
    try:
        with open(path, newline="") as fh:
            rd = csv.reader(fh)
            header = next(rd, None)
            rows = [r for r in rd if r]
    except OSError as e:
        print(f"{g} FAIL: cannot read {path}: {e} -- the flag-on run wrote no chain log")
        return 1
    if header != COLS:
        print(f"{g} FAIL: header {header} is not {COLS}")
        return 1
    print(f"{g} rows: {len(rows)} in {path}")
    if not rows:
        print(f"{g} FAIL: zero rows -- no relayed range was ever decided")
        return 1

    bad = 0
    fused = {}          # (relayId, depth) -> first line number of a fuse row
    n_fuse = 0
    budget_pass = budget_ref = 0
    d2_pass = d2_ref = 0
    by_depth = {}
    for n, r in enumerate(rows, start=2):
        if len(r) != len(COLS):
            print(f"{g} FAIL: line {n} has {len(r)} fields: {r}")
            return 1
        try:
            t = float(r[0])
            consumer, relay = int(r[1]), int(r[2])
            leg = r[3]
            depth, ceiling = int(r[5]), int(r[6])
            carried, budget = float(r[8]), float(r[10])
            a2, verdict = r[11], r[12]
            s_raw, s_eff = float(r[13]), float(r[14])
        except ValueError as e:
            print(f"{g} FAIL: line {n} does not parse ({e}): {r}")
            return 1
        del t
        if leg == "fuse":
            if verdict != "FUSED" or a2 != "-" or depth < 1:
                print(f"{g} FAIL: line {n}: malformed fuse row {r}")
                bad += 1
            # A fuse at depth d >= 2 leaned on a holder that must itself have fused at d-1.
            if depth >= 2 and fused.get((relay, depth - 1)) is None:
                print(f"{g} FAIL: line {n}: relay {consumer} fused at depth {depth} through "
                      f"{relay}, which has no earlier fuse at depth {depth - 1}")
                bad += 1
            fused.setdefault((consumer, depth), n)
            n_fuse += 1
            continue
        if leg not in ("relay", "tag"):
            print(f"{g} FAIL: line {n}: unknown leg {leg!r}")
            bad += 1
            continue
        by_depth[(leg, depth)] = by_depth.get((leg, depth), 0) + 1

        if verdict == "COMPOSE_REFUSED" or verdict.startswith("REJECT_TIER"):
            if a2 != "not_reached":
                print(f"{g} FAIL: line {n}: {verdict} never reaches A2, yet a2={a2}")
                bad += 1
        else:
            want = expected_a2(depth, ceiling, carried, budget)
            if want is not None and a2 != want:
                print(f"{g} FAIL: line {n}: a2={a2} but depth {depth}/ceiling {ceiling}, "
                      f"carried {carried} vs budget {budget} says {want}")
                bad += 1
            is_depth_rej = verdict.startswith("REJECT_DEPTH")
            if is_depth_rej != (a2 in ("ceiling", "budget")):
                print(f"{g} FAIL: line {n}: verdict {verdict} disagrees with a2={a2}")
                bad += 1
            if verdict == "ADMIT" and a2 != "pass":
                print(f"{g} FAIL: line {n}: ADMIT with a2={a2}")
                bad += 1
            if budget > 0.0 and a2 == "pass":
                budget_pass += 1
                d2_pass += 1 if depth == 2 else 0
            if a2 == "budget":
                budget_ref += 1
                d2_ref += 1 if depth == 2 else 0
        if verdict == "ADMIT":
            proj = s_eff * s_eff - s_raw * s_raw
            if not (s_raw > 0.0) or proj < -1e-6 or proj > carried + 1e-3 * max(1.0, carried):
                print(f"{g} FAIL: line {n}: ADMIT with sigma_raw {s_raw}, sigma_eff {s_eff}, "
                      f"projection {proj} outside [0, tr P = {carried}]")
                bad += 1
        if depth >= 2:
            at = fused.get((relay, depth - 1))
            if at is None or at > n:
                print(f"{g} FAIL: line {n}: depth {depth} through relay {relay}, which has no "
                      f"earlier fuse at depth {depth - 1} -- depth from nowhere")
                bad += 1
    if bad:
        return 1

    shown = ", ".join(f"{leg} d{d}: {c}" for (leg, d), c in sorted(by_depth.items()))
    print(f"{g} decisions by leg and depth: {shown}")
    print(f"{g} fuse rows: {n_fuse}; A2 under a set budget: pass {budget_pass}, budget "
          f"{budget_ref}; at depth 2: pass {d2_pass}, budget {d2_ref}")
    if budget_pass == 0 or budget_ref == 0:
        print(f"{g} FAIL: the budget half of A2 was not observed both admitting and refusing")
        return 1
    if log_path is not None and not check_stdout(g, log_path, n_fuse):
        return 1
    if need2:
        if n_fuse == 0:
            print(f"{g} FAIL: no short relay ever FUSED a relayed range, so no depth >= 2 "
                  f"can exist")
            return 1
        if d2_pass == 0 or d2_ref == 0:
            print(f"{g} FAIL: at depth 2, A2 was not observed both admitting and refusing on "
                  f"the budget")
            return 1
    print(f"{g} PASS")
    return 0


if __name__ == "__main__":
    sys.exit(main(sys.argv))
