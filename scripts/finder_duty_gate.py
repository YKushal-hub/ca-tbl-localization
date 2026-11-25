#!/usr/bin/env python3
"""G188: one 868 MHz ledger per searcher's device (DD 60). usage: finder_duty_gate.py <run set dir>.
RUNS exit 0; OFFSAME the explicit defaults change no output; REPORT counting changes no output and prints four searchers, each with
MAC airtime; OVER without enforcement at least one searcher is over 2.5 % (else nothing to remove); LEGAL with it every mean and
audited peak is at most 2.5 % and above zero; REFUSED replies are refused only when enforced; LINES announced only when on."""
import os
import re
import sys

PAT = re.compile(r"DD60 finder (\d+) ca_tbl_full: replies ([\d.]+) s, MAC ([\d.]+) s, of [\d.]+ s: ([\d.]+), audited peak "
                 r"([\d.]+), replies refused \(all finders\) (\d+)")
CAP = 0.025


def main():
    d = sys.argv[1]
    bad = 0

    def rep(ok, name, msg):
        nonlocal bad
        print(f"  G188 {'ok  ' if ok else 'FAIL'} {name} {msg}")
        bad += not ok

    def text(n):
        p = os.path.join(d, n)
        return open(p, errors="replace").read() if os.path.exists(p) else ""

    def same(a, b):
        x, y = os.path.join(d, a), os.path.join(d, b)
        fa = sorted(q for q in os.listdir(x) if q != "run_id.txt") if os.path.isdir(x) else []
        fb = sorted(q for q in os.listdir(y) if q != "run_id.txt") if os.path.isdir(y) else []
        diff = [q for q in fa if q in fb and open(os.path.join(x, q), "rb").read() != open(os.path.join(y, q), "rb").read()]
        return bool(fa) and fa == fb and not diff, f"{a} against {b}: {len(fa)} files, differing {diff[:3]}"

    rc = {n: text(n + ".rc").strip() or "none" for n in ("off", "false", "report", "duty")}
    rep(all(v == "0" for v in rc.values()), "RUNS", f"exit codes {rc}")
    rep(*(lambda t: (t[0], "OFFSAME", t[1]))(same("off", "false")))
    r = PAT.findall(text("report.log"))
    u = PAT.findall(text("duty.log"))
    ok, msg = same("off", "report")
    rep(ok and [x[0] for x in r] == ["501", "502", "503", "504"] and all(float(x[2]) > 0 for x in r), "REPORT", f"{msg}; {len(r)} searchers")
    rep(any(float(x[4]) > CAP for x in r), "OVER", f"unenforced audited peaks {[x[4] for x in r]}")
    rep(len(u) == 4 and all(0 < float(x[3]) <= CAP and 0 < float(x[4]) <= CAP for x in u), "LEGAL", f"enforced mean, peak {[(x[3], x[4]) for x in u]}")
    rep(all(x[5] == "0" for x in r) and bool(u) and all(int(x[5]) > 0 for x in u), "REFUSED", f"report {[x[5] for x in r]}, duty {[x[5] for x in u]}")
    on = {n: "DD60 sharedDuty868Finders" in text(n + ".log") for n in ("off", "report", "duty")}
    rep(on == {"off": False, "report": False, "duty": True}, "LINES", str(on))
    print(f"  G188: {bad} failed")
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
