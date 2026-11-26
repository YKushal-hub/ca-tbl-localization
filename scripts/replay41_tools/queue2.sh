#!/usr/bin/env bash
# takes over replay_dd41_queue.sh after realistic3: R2 and R1 in parallel; live prune; disk watchdog; resume with more jobs when alone
set -u
cd $HOME/catbl_multi || exit 2
R=$HOME/catbl_work/campaigns/replay41; Q=$R/QUEUE.md; PR=$HOME/catbl_work/campaigns/prune_runs.sh
log(){ echo "$(date -u +%FT%TZ) $*" >> $Q; }
start(){ # campaign jobs ; runner in its own session so it can be stopped as a group
  local U=$(echo $1 | tr a-z A-Z)
  setsid env "${U}_JOBS=$2" bash scripts/$1_run.sh $R/$1 >> $R/$1_runner.log 2>&1 &
  echo $! > $R/.$1.pid
  for i in $(seq 30); do [ -e $R/$1/build.sha256 ] && break; sleep 2; done
  pgrep -f "prune_runs.sh $R/$1 --loop" >/dev/null || bash $PR $R/$1 --loop &
  log "queue2 start $1 J=$2"; }
alive(){ kill -0 "$(cat $R/.$1.pid 2>/dev/null)" 2>/dev/null; }
isdone(){ grep -q " done" $R/$1/RUN.md 2>/dev/null; }
stopc(){ kill -TERM -- -"$(cat $R/.$1.pid)" 2>/dev/null; sleep 3; kill -KILL -- -"$(cat $R/.$1.pid)" 2>/dev/null; }
start realistic2 5
log "queue2 waiting for realistic3"
R1=0; boosted=
while true; do
  sleep 20
  fw=$(df --output=avail -BG $R | tail -1 | tr -dc 0-9); fc=$(df --output=avail -BG /mnt/c | tail -1 | tr -dc 0-9)
  if [ "$fw" -lt 20 ] || [ "$fc" -lt 5 ]; then for c in realistic1 realistic2; do alive $c && stopc $c; done; log "STOP disk $fw GB / C: $fc GB"; exit 1; fi
  if [ $R1 = 0 ] && isdone realistic3; then log "realistic3 rc=0"; bash $PR $R/realistic3; start realistic1 7; R1=1; fi
  for c in realistic2 realistic1; do
    [ -e $R/.$c.pid ] && ! alive $c && ! isdone $c && { log "STOP $c runner ended without done"; exit 1; }
    isdone $c && ! grep -q "$c finished" $Q && { bash $PR $R/$c; log "$c finished rc=0"; }
  done
  # when one campaign is alone, restart it once with 12 jobs (frozen runner's own resume path)
  if [ -z "$boosted" ] && [ $R1 = 1 ]; then
    for p in "realistic1 realistic2" "realistic2 realistic1"; do set -- $p
      if isdone $1 && ! isdone $2 && alive $2; then
        left=$(( $( [ $2 = realistic2 ] && echo 240 || echo 380 ) - $(find $R/$2 -name rc | wc -l) ))
        [ $left -gt 40 ] && { stopc $2; start $2 12; log "boost $2 ($left cells left)"; }
        boosted=1; fi
    done; fi
  isdone realistic1 && isdone realistic2 && isdone realistic3 && { log "ALL DONE"; exit 0; }
done
