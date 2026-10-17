#!/usr/bin/env bash
# G147: the scope-2026-10-17 flags. usage: bash scripts/scope_flags_gate.sh [<workdir>]
# DEFAULT  every flag given explicitly at its default reproduces the default run byte for byte (one run with all of them)
# FIRES    each flag is shown to act (rule 4: a reachable path, not an assumed one):
#   --probeMonotoneTime   probe_range_sweep.csv differs from the default probe
#   --fadingPerBand       tag 0's trace differs
#   --finderEnergyReport  finder_summary_*.csv written (4 finders), digested
#   --dualBase868OnNeed   C3 line reports > 0 skipped 868 MHz slots
#   --hold24AfterK=3      C3 line reports > 0 skipped slots and the trace differs
#   --tofRecal            C2 line reports > 0 pairs
#   --tofCalibrate        under wolf, the mean error of tag 0's 2.4 GHz ranges drops by > 40 m
#   --acquireNegInfo      C1 line reports > 0 factors (with --acquireParticles, the preset's)
#   --relayNisInflate     B1 line printed and the trace differs
#   --agcPerReceiver      unit test (scratch/b6_agc_unit.cc): a strong frame on receiver A no longer desensitises B
#                         (in the scenario no link exceeds the -40 dBm AGC knee, so the flag is inert there)
set -u
cd "$(dirname "$0")/.." || exit 2
BIN="$(pwd)/build/scratch/ns3.47-ca_tbl_causal_sim-optimized"
if [ $# -ge 1 ]; then D=$1; mkdir -p "$D" || exit 2; else D=$(mktemp -d "${TMPDIR:-/tmp}/g147.XXXXXX") || exit 2; trap 'rm -rf "$D"' EXIT; fi
run() { local n=$1; shift; mkdir -p "$D/$n"; "$BIN" "$@" --outdir="$D/$n" > "$D/$n.log" 2>&1; echo $? > "$D/$n.rc"; }
B="--specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --traceRanges=true --boxHalf=750 --finderMode=1"
OFF="--probeMonotoneTime=false --fadingPerBand=false --finderEnergyReport=false --dualBase868OnNeed=false --hold24AfterK=0 --tofRecal=false --tofCalibrate=false --acquireNegInfo=false --relayNisInflate=false --agcPerReceiver=false"
run base $B &
run off $B $OFF &
run fade $B --fadingPerBand=true &
run fin $B --finderEnergyReport=true &
run c3a $B --dualBase868OnNeed=true &
run c3b $B --hold24AfterK=3 &
run c2 --specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --tofRecal=true &
run c1 --specProfile=true --arms=ca_tbl_full --seed=7 --duration=600 --trace=true --unknownStart=true --acquireNegInfo=true &
wait
run b1 $B --relayNisInflate=true &
run wb --specProfile=true --arms=static_2p4 --seed=7 --duration=600 --trace=true --traceRanges=true --boxHalf=750 --finderMode=1 --tofErrorModel=wolf &
run wc --specProfile=true --arms=static_2p4 --seed=7 --duration=600 --trace=true --traceRanges=true --boxHalf=750 --finderMode=1 --tofErrorModel=wolf --tofCalibrate=true &
run p0 --probe=true --seed=3 --foliageDepthClamp=true &
run pm --probe=true --seed=3 --foliageDepthClamp=true --probeMonotoneTime=true &
wait
python3 - "$D" <<'PY'
import csv, filecmp, os, re, statistics as st, sys
d = sys.argv[1]; res = []
def rc(n): return open(f"{d}/{n}.rc").read().strip() == "0"
def log(n): return open(f"{d}/{n}.log", errors="replace").read()
def same(a, b, f): return filecmp.cmp(f"{d}/{a}/{f}", f"{d}/{b}/{f}", shallow=False)
runs = ["base", "off", "fade", "fin", "c3a", "c3b", "c2", "c1", "b1", "wb", "wc", "p0", "pm"]
bad = [r for r in runs if not rc(r)]
res.append(("RUNS", not bad, f"bad {bad}" if bad else f"{len(runs)} runs exit 0"))
if not bad:
    fa = sorted(f for f in os.listdir(f"{d}/base") if f != "run_id.txt")
    diff = [f for f in fa if not os.path.exists(f"{d}/off/{f}") or not same("base", "off", f)]
    res.append(("DEFAULT", not diff, f"{len(diff)} outputs differ {diff[:3]}"))
    T = "trace_ca_tbl_full_seed7.csv"
    res.append(("FADE", not same("base", "fade", T), "--fadingPerBand changes the trace"))
    fs = f"{d}/fin/finder_summary_ca_tbl_full_seed7.csv"
    ok = os.path.exists(fs) and len(list(csv.DictReader(open(fs)))) == 4 and "finder_summary_ca_tbl_full_seed7.csv" in open(f"{d}/fin/run_id.txt").read()
    res.append(("FINDER", ok, "4 finder rows, digested" if ok else "missing / wrong / undigested"))
    for n, lab in (("c3a", "C3A"), ("c3b", "C3B")):
        m = re.findall(r"868 MHz slots skipped (\d+)", log(n))
        res.append((lab, bool(m) and int(m[-1]) > 0, f"skipped {m[-1] if m else None}"))
    res.append(("HOLD", not same("base", "c3b", T), "--hold24AfterK changes the trace"))
    m = re.findall(r"from (\d+) pairs", log("c2")); res.append(("C2", bool(m) and int(m[-1]) > 0, f"pairs {m[-1] if m else None}"))
    m = re.findall(r"factors applied (\d+)", log("c1")); res.append(("C1", bool(m) and int(m[-1]) > 0, f"factors {m[-1] if m else None}"))
    res.append(("B1", "B1 relayNisInflate" in log("b1") and not same("base", "b1", T), "line printed, trace changes"))
    e = lambda n: st.mean(float(r["errM"]) for r in csv.DictReader(open(f"{d}/{n}/ranges_static_2p4_seed7.csv")) if r["tier"] == "2.4GHz")
    res.append(("CALIB", e("wb") - e("wc") > 40.0, f"wolf 2.4 GHz mean range error {e('wb'):.1f} -> {e('wc'):.1f} m"))
    res.append(("MONO", not same("p0", "pm", "probe_range_sweep.csv"), "--probeMonotoneTime changes the probe"))
    import subprocess
    u = subprocess.run(["./build/scratch/ns3.47-b6_agc_unit-optimized"], capture_output=True, text=True)
    res.append(("AGC", u.returncode == 0, u.stdout.strip().replace("\n", " | ")))
for l, ok, why in res:
    print(f"G147 {'PASS' if ok else 'FAIL'} {l:8s} {why}")
sys.exit(0 if all(ok for _, ok, _ in res) else 1)
PY
