#!/usr/bin/env python3
"""The analysis registered by report/PREREG_UWB_NLOS.md, applied to its 40 cells.

Reads $HOME/catbl_uwbnlos/registered/<cell>/seed_<N>/ (or the directory given) and prints
a markdown read-out: completeness and provenance first (sect 7 abort conditions, rule 12:
rows are counted), then P1-P5 exactly as sect 5 words them, each with its pooled numbers.
Nothing here is tuned after the fact; the thresholds are the registration's.

Denominators (sect 3): the matrix is over DECODED UWB links, the rows of
nlos_links_static_uwb_seed<N>.csv with tier UWB; scored rows fill tp/fp/fn/tn, unscored
rows are counted apart; polled = decoded + lost, and lost-by-truth is the summary's
lostLos / lostNlos. Window 0-3600 s, 1440 cycles per seed. Pooling: sums over the 10 seeds.

Usage: uwb_nlos_analyse.py [REGISTERED_DIR] [--commit SHORTSHA]
Exit 0 if every cell is complete and provenance-clean (whatever the verdicts), 1 otherwise.
"""
import csv
import hashlib
import math
import os
import sys

CELLS = ["A_802", "A_leg", "B_802", "B_leg"]
SEEDS = list(range(7, 17))
PAIRS = {"outdoor": (45.6, 73.0, 1.76, 2.5, 0.12, 0.13)}  # CM5/CM6; no cell is office
F_HZ = 7.99e9


def dpl(d):
    pl0l, pl0n, nl, nn, kl, kn = PAIRS["outdoor"]
    return (pl0n - pl0l) + 10.0 * (nn - nl) * math.log10(max(d, 1.0)) + \
        20.0 * (kn - kl) * math.log10(F_HZ / 5e9)


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for b in iter(lambda: f.read(1 << 20), b""):
            h.update(b)
    return h.hexdigest()


def main(argv):
    root = os.path.expanduser("~/catbl_uwbnlos/registered")
    commit = None
    args = list(argv)
    if "--commit" in args:
        i = args.index("--commit")
        commit = args[i + 1]
        del args[i:i + 2]
    if args:
        root = args[0]
    problems = []
    pool = {}
    for cell in CELLS:
        p = dict(tp=0, fp=0, fn=0, tn=0, unscoredLos=0, unscoredNlos=0, lostLos=0,
                 lostNlos=0, rows=0, nlosRows=[], p4bad=0, scoreLos=[], scoreNlos=[], steps=[])
        for seed in SEEDS:
            d = os.path.join(root, cell, f"seed_{seed}")
            rid = os.path.join(d, "run_id.txt")
            if not os.path.isfile(rid):
                problems.append(f"{cell}/seed_{seed}: no run_id.txt")
                continue
            lines = open(rid).read().splitlines()
            src = [l for l in lines if l.startswith("source ")]
            if not src or " dirty 0 " not in src[0] + " ":
                problems.append(f"{cell}/seed_{seed}: sidecar not dirty 0 ({src})")
            elif commit and not src[0].startswith(f"source {commit} "):
                problems.append(f"{cell}/seed_{seed}: sidecar source {src[0]} != {commit}")
            dig = lines[lines.index("digest sha256") + 1:] if "digest sha256" in lines else []
            for l in dig:
                h, _, name = l.partition("  ")
                fp = os.path.join(d, name)
                if not os.path.isfile(fp) or sha256(fp) != h:
                    problems.append(f"{cell}/seed_{seed}: digest mismatch or missing {name}")
            arm = list(csv.DictReader(open(os.path.join(d, "arm_summary.csv"))))
            if len(arm) != 1 or arm[0]["arm"] != "static_uwb" or arm[0]["seed"] != str(seed):
                problems.append(f"{cell}/seed_{seed}: arm_summary has {len(arm)} rows, expected 1")
                continue
            p["steps"].append(int(arm[0]["steps"]))
            summ = [r for r in csv.DictReader(open(os.path.join(d, f"nlos_summary_static_uwb_seed{seed}.csv")))
                    if r["kind"] == "confusion" and r["tier"] == "UWB"]
            if len(summ) != 1:
                problems.append(f"{cell}/seed_{seed}: {len(summ)} UWB confusion rows")
                continue
            for k in ("tp", "fp", "fn", "tn", "unscoredLos", "unscoredNlos", "lostLos", "lostNlos"):
                p[k] += int(summ[0][k])
            for r in csv.DictReader(open(os.path.join(d, f"nlos_links_static_uwb_seed{seed}.csv"))):
                if r["tier"] != "UWB":
                    continue
                p["rows"] += 1
                sc = float(r["score"])
                if r["truthNlos"] == "1":
                    p["nlosRows"].append((seed, float(r["distM"]), float(r["snrDb"]), float(r["nlosDb"]), sc))
                    if abs(float(r["nlosDb"]) - dpl(float(r["distM"]))) > 1e-6:
                        p["p4bad"] += 1
                    if sc >= 0:
                        p["scoreNlos"].append(sc)
                else:
                    if float(r["nlosDb"]) != 0.0:
                        p["p4bad"] += 1
                    if sc >= 0:
                        p["scoreLos"].append(sc)
        pool[cell] = p

    out = []
    out.append("# UWB-NLOS registered cells: analysis\n")
    out.append(f"Directory `{root}`; registration `report/PREREG_UWB_NLOS.md`.\n")
    out.append("## Completeness and provenance (sect 7)\n")
    for cell in CELLS:
        st = pool[cell]["steps"]
        out.append(f"- {cell}: {len(st)} of 10 seeds read; steps per seed "
                   f"{sorted(set(st)) if st else '-'} (1440 expected)")
    if problems:
        out.append("\n**PROBLEMS (the campaign is not reportable while any stands):**\n")
        out.extend(f"- {m}" for m in problems)
    else:
        out.append("\nEvery cell complete; every sidecar `dirty 0` with verified digests.")
    out.append("\n## Pooled UWB matrix per cell (10 seeds)\n")
    out.append("| cell | tp | fp | fn | tn | unscored LOS | unscored NLOS | lost LOS | lost NLOS | decoded rows | decoded NLOS |")
    out.append("|---|---|---|---|---|---|---|---|---|---|---|")
    for cell in CELLS:
        p = pool[cell]
        out.append(f"| {cell} | {p['tp']} | {p['fp']} | {p['fn']} | {p['tn']} | {p['unscoredLos']} | "
                   f"{p['unscoredNlos']} | {p['lostLos']} | {p['lostNlos']} | {p['rows']} | {len(p['nlosRows'])} |")

    def verdict(ok):
        return "HOLDS" if ok else "FALSIFIED"

    a8, al, b8, bl = (pool[c] for c in CELLS)
    out.append("\n## Predictions (sect 5)\n")
    p1 = a8["tp"] >= 1 and a8["fn"] >= 1
    out.append(f"- **P1** (A_802: tp >= 1 and fn >= 1): tp {a8['tp']}, fn {a8['fn']} -> **{verdict(p1)}**")
    al_n = len(al["nlosRows"]) + al["lostNlos"]
    bl_n = len(bl["nlosRows"]) + bl["lostNlos"]
    p2 = al["tp"] == 0 and al["fn"] == 0 and bl["tp"] == 0 and bl["fn"] == 0 and al_n <= 3 and bl_n <= 15
    out.append(f"- **P2** (A_leg, B_leg: tp = fn = 0; NLOS polls <= 3 and <= 15): A_leg tp {al['tp']} "
               f"fn {al['fn']} NLOS polls {al_n}; B_leg tp {bl['tp']} fn {bl['fn']} NLOS polls {bl_n} "
               f"-> **{verdict(p2)}**")
    b8_dec = b8["nlosRows"]
    p3 = b8["lostNlos"] >= 50 and len(b8_dec) <= 2 and all(r[1] <= 3.0 for r in b8_dec)
    out.append(f"- **P3** (B_802: lostNlos >= 50; <= 2 decoded NLOS, each within 3 m): lostNlos "
               f"{b8['lostNlos']}, decoded NLOS {len(b8_dec)} at d_3D "
               f"{[round(r[1], 2) for r in b8_dec]} m -> **{verdict(p3)}**")
    p4 = a8["p4bad"] == 0 and b8["p4bad"] == 0
    out.append(f"- **P4** (both _802: NLOS rows carry dPL(distM) to 1e-6 dB, LOS rows 0): "
               f"violations A_802 {a8['p4bad']}, B_802 {b8['p4bad']} over {len(a8['nlosRows'])} + "
               f"{len(b8_dec)} NLOS rows -> **{verdict(p4)}**"
               + ("" if (a8["nlosRows"] or b8_dec) else " (vacuous: no decoded NLOS row)"))
    if a8["scoreNlos"] and a8["scoreLos"]:
        mn = sum(a8["scoreNlos"]) / len(a8["scoreNlos"])
        ml = sum(a8["scoreLos"]) / len(a8["scoreLos"])
        out.append(f"- **P5** (descriptive; A_802 mean score NLOS > LOS): NLOS {mn:.4f} over "
                   f"{len(a8['scoreNlos'])} scored rows, LOS {ml:.4f} over {len(a8['scoreLos'])} -> "
                   f"**{verdict(mn > ml)}**")
    else:
        out.append(f"- **P5**: UNDEFINED -- scored NLOS rows {len(a8['scoreNlos'])}, scored LOS rows "
                   f"{len(a8['scoreLos'])}; a mean over an empty set is not a comparison")
    out.append("\n## Decoded NLOS rows (seed, d_3D m, SNR dB, nlosDb, score)\n")
    for cell in ("A_802", "B_802"):
        rows = pool[cell]["nlosRows"]
        out.append(f"- {cell}: {len(rows)} rows" + ("" if not rows else ""))
        for r in rows[:40]:
            out.append(f"  - seed {r[0]}: {r[1]:.3f} m, {r[2]:.2f} dB, {r[3]:.4f} dB, score {r[4]:.4f}")
        if len(rows) > 40:
            out.append(f"  - ... {len(rows) - 40} more")
    print("\n".join(out))
    return 1 if problems else 0


if __name__ == "__main__":
    sys.exit(main(sys.argv[1:]))
