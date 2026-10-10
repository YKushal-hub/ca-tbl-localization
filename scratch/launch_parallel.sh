#!/bin/bash
cd "$(dirname "$0")/.." || exit 1   # scratch/ -> repository root
python3 scratch/train_stage2_parallel.py > /tmp/stage2_parallel.log 2>&1 &
PID=$!
echo "Started PID=$PID"
sleep 6
echo "--- Initial log ---"
cat /tmp/stage2_parallel.log
echo "--- CPU/MEM of children ---"
ps -eo pid,ppid,pcpu,pmem,etime,args | grep "python3" | grep -v grep
