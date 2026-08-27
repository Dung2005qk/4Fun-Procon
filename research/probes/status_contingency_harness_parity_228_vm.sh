#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon228-0826
runner_pid=$(cat "$root/runner.pid")
ps -o pid,ppid,etime,rss,cmd -p "$runner_pid" --ppid "$runner_pid" || true
for lane in no-ack-control buggy-226 cache-faithful; do
    count=$(find "$root/logs228" -maxdepth 1 -type f \
        -name "$lane-*.log" -exec grep -l '^summary ' {} + 2>/dev/null \
        | wc -l)
    printf '%s %s\n' "$lane" "$count"
done
wc -c "$root/runner.stderr"
free -h
