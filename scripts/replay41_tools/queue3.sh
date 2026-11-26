#!/usr/bin/env bash
# memory-budgeted finish of realistic2 and realistic1 (frozen binaries and commands); live prune; disk guard; refills missing cells
R=$HOME/catbl_work/campaigns/replay41; W=$HOME/catbl_work/campaigns; Q=$R/QUEUE.md
log(){ echo "$(date -u +%FT%TZ) $*" >> $Q; }
cnt(){ find $R/$1 -name rc -size +0 | wc -l; }
log "queue3 start (queue2 stopped: memory)"
while true; do
  for c in "realistic2 240" "realistic1 380"; do set -- $c
    if [ $(cnt $1) -lt $2 ] && ! pgrep -f "^bash $W/${1}_budget.sh" >/dev/null; then setsid bash $W/${1}_budget.sh $R/$1 >> $R/$1_runner.log 2>&1 & fi
    bash $W/prune_runs.sh $R/$1
  done
  fw=$(df --output=avail -BG $R | tail -1 | tr -dc 0-9); fc=$(df --output=avail -BG /mnt/c | tail -1 | tr -dc 0-9)
  [ "$fw" -lt 20 ] || [ "$fc" -lt 4 ] && { pkill -f "^bash $W/realistic[12]_budget.sh"; log "STOP disk $fw GB / C: $fc GB"; exit 1; }
  [ $(cnt realistic2) -ge 240 ] && [ $(cnt realistic1) -ge 380 ] && { log "ALL DONE"; exit 0; }
  sleep 60
done
