#!/bin/bash
# launch_retrain.sh -- Full 12-variant retrain with Bug-1-fixed features
# Kills any leftover python training processes first, then starts fresh.
cd "$(dirname "$0")/.." || exit 1   # scratch/ -> repository root

echo "=== Killing any stale training processes ==="
pkill -f train_stage2 2>/dev/null; sleep 2

echo "=== Confirming stale results are gone ==="
ls results/stage2_M*.pkl results/stage2_result_M*.json 2>/dev/null && echo "WARNING: stale files found!" || echo "Clean — no stale results"

echo "=== Launching parallel retrain (Bug-1-fixed features) ==="
python3 scratch/train_stage2_parallel.py > /tmp/stage2_retrain.log 2>&1 &
PID=$!
echo "Started PID=$PID"
sleep 8
echo "=== Initial log ==="
cat /tmp/stage2_retrain.log
echo "=== Processes ==="
ps -eo pid,pcpu,pmem,etime,args | grep python3 | grep -v grep
