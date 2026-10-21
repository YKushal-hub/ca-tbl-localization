#!/usr/bin/env python3
"""G164: --uwbMode puts tier 3 on one DW3000 Table 9 row -- billing, ranging and sensitivity -- and legacy is absent.

Reads the run set scripts/uwb_mode_gate.sh writes. What each failure looks like on disk:
  RUNS    every run but the refusals exits 0                                   (a crash in a mode)
  SENS    UWB nominal margin moves by the row's sensitivity against legacy's   (the mode not setting sensitivity,
          -104 dBm: 6m8 by -10.8 dB, 850k by -4.7 dB (+/-0.5); 868/2.4 GHz      or leaking into other tiers)
          rows identical
  RANGE   the probe's UWB coherent gain: legacy and 850k 30.10 dB             (the ranging still integrating the
          (1024 symbols), 6m8 18.06 dB (64 symbols)                            wrong preamble)
  BILL    the 20-byte frame each mode prints it bills (CaTblEnergyModel::     (frames billed in another mode
          UwbFrameTimeS, the function the MAC and energy model call) equals     than the one decoded)
          the 802.15.4z closed form for its row: 6m8 64 symbols at 6.81
          Mb/s, 850k 1024 symbols at 850 kb/s
  OFF     def/ and leg/ byte-identical, run_id.txt aside                      (legacy not being the default)
  REFUSE  r_name and r_both exit 1                                             (an unknown mode, or a mode
                                                                               contradicting an explicit sensitivity)
"""
import csv
import os
import sys

nok = nfail = 0


def report(ok, label, msg):
    global nok, nfail
    nok += ok
    nfail += not ok
    print(f"  G164 {'ok     ' if ok else 'FAIL   '} {label:<10}{msg}")


def rc(d, n):
    p = os.path.join(d, n + ".rc")
    return int(open(p).read().strip()) if os.path.exists(p) else None


def probe(d, n):
    p = os.path.join(d, n, "probe_range_sweep.csv")
    return {(r["tier"], r["distance_m"]): r for r in csv.DictReader(open(p))} if os.path.exists(p) else {}


def row(d, n):
    p = os.path.join(d, n, "arm_summary.csv")
    rs = [r for r in csv.DictReader(open(p)) if r["arm"] == "ca_tbl_full"] if os.path.exists(p) else []
    return rs[0] if rs else None


def main():
    d = sys.argv[1]
    names = ("p_leg", "p_6m8", "p_850", "e_leg", "e_6m8", "e_850", "def", "leg")
    rcs = {n: rc(d, n) for n in names}
    report(all(v == 0 for v in rcs.values()), "RUNS", f"exit codes {rcs} (need all 0)")
    L, A, B = probe(d, "p_leg"), probe(d, "p_6m8"), probe(d, "p_850")
    ok = bool(L) and bool(A) and bool(B)
    msg = []
    for P, want, nm in ((A, -10.8, "6m8"), (B, -4.7, "850k")):
        sh = [float(P[k]["mean_nominal_margin_db"]) - float(L[k]["mean_nominal_margin_db"]) for k in L if k[0] == "UWB" and k in P]
        same = all(P.get(k, {}).get("mean_nominal_margin_db") == L[k]["mean_nominal_margin_db"] for k in L if k[0] != "UWB")
        good = bool(sh) and all(abs(x - want) <= 0.5 for x in sh) and same
        ok = ok and good
        msg.append(f"{nm} UWB shift {min(sh, default=0):+.2f}..{max(sh, default=0):+.2f} (need {want:+.1f}), other tiers same {same}")
    report(ok, "SENS", "; ".join(msg))
    g = {nm: {float(r["coherent_gain_db"]) for k, r in P.items() if k[0] == "UWB"} for nm, P in (("leg", L), ("6m8", A), ("850k", B))}
    okr = g["leg"] == {30.1} and g["850k"] == {30.1} and g["6m8"] == {18.06}
    report(okr, "RANGE", f"UWB coherent gain legacy {sorted(g['leg'])}, 6m8 {sorted(g['6m8'])}, 850k {sorted(g['850k'])} "
                         f"(need 30.10, 18.06, 30.10)")
    import re

    def billed(n):
        p = os.path.join(d, n + ".log")
        m = re.search(r"a 20-byte frame bills ([0-9.eE+-]+) s", open(p, errors="replace").read()) if os.path.exists(p) else None
        return float(m.group(1)) if m else None

    def closed_form(pre, rate):  # IEEE 802.15.4z HRP: (preamble + 8-symbol SFD) x 1.0176 us, PHR 19 bits at 850 kb/s,
        return (pre + 8) * 1.0176e-6 + 19 / 850e3 + (8 * 20 + 48) / rate  # payload + CRC/RS at the data rate
    b6, b8 = billed("e_6m8"), billed("e_850")
    want6, want8, leg = closed_form(64, 6.81e6), closed_form(1024, 850e3), closed_form(128, 6.81e6)
    okb = b6 is not None and b8 is not None and abs(b6 - want6) < 1e-9 and abs(b8 - want8) < 1e-9
    report(okb, "BILL", f"20-byte UWB frame billed: 6m8 {b6} s (closed form {want6:.9f}), 850k {b8} s "
                        f"(closed form {want8:.9f}); legacy closed form {leg:.9f}")
    x, y = os.path.join(d, "def"), os.path.join(d, "leg")
    fa = sorted(q for q in os.listdir(x) if q != "run_id.txt") if os.path.isdir(x) else []
    fb = sorted(q for q in os.listdir(y) if q != "run_id.txt") if os.path.isdir(y) else []
    diff = [q for q in fa if q not in fb or open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
    report(bool(fa) and not diff and fa == fb, "OFF", f"def/ against leg/: {len(fa)} files, differing {diff[:3]} (need none)")
    rr = {n: rc(d, n) for n in ("r_name", "r_both")}
    report(all(v == 1 for v in rr.values()), "REFUSE", f"exit codes {rr} (need 1, 1)")
    print(f"  G164: {nok} ok, {nfail} failed")
    return 0 if nfail == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
