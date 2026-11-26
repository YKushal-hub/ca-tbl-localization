#!/usr/bin/env bash
# exit 0 when a new run fits: at most 11 running, and reserved memory (per run type, or actual if larger) within budget
w(){ case "$1" in *RING12_F2_ARMS*) echo 2000;; *RING12_F2_*) echo 1500;; *RING12_F0*) echo 1000;; *) echo 420;; esac; }
t=0; n=0
for p in $(pgrep -f 'optimized --specProfile=true'); do
  a=$(tr '\0' ' ' < /proc/$p/cmdline 2>/dev/null); case "$a" in /*ns3.47*) ;; *) continue;; esac
  r=$(awk '/VmRSS|VmSwap/{s+=$2}END{print int(s/1024)}' /proc/$p/status 2>/dev/null); x=$(w "$a"); [ "${r:-0}" -gt $x ] && x=$r
  t=$((t+x)); n=$((n+1)); done
[ $n -lt 11 ] && [ $((t+$(w "$1"))) -le 6900 ] && [ "$(awk '/MemAvailable/{print int($2/1024)}' /proc/meminfo)" -gt 900 ]
