#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon228-0826
if [[ -f "$root/runner.pid" ]] && kill -0 "$(cat "$root/runner.pid")" 2>/dev/null; then
    echo "runner already active: $(cat "$root/runner.pid")"
    exit 0
fi

nohup bash "$root/run.sh" \
    > "$root/runner.stdout" \
    2> "$root/runner.stderr" \
    < /dev/null &
printf '%s\n' "$!" > "$root/runner.pid"
echo "started runner $(cat "$root/runner.pid")"
