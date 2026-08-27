#!/usr/bin/env bash
set -euo pipefail

root=${1:-/home/LMC/udon227-0826}
for side in candidate e8bf766 baebad8; do
    smoke="/tmp/udon227-smoke-${side}.log"
    "$root/$side/historical_tournament" \
        --version "smoke-$side" \
        --track role-diagnostic-isolation \
        --suite stratified-easy \
        --first-seed 8600001 \
        --seeds 1 \
        --budget-ms 100 \
        --role-ms 200 \
        --role-mode deadline \
        --fuel-profile generated \
        --spot-count 12 \
        --short-role-fallback 1 >"$smoke"
    [[ $(grep -c '^result,' "$smoke") -eq 1 ]]
    echo "$side smoke=ok"
done

[[ ! -e "$root/logs227/all.run_complete" ]]
if [[ -f "$root/runner.pid" ]] && kill -0 "$(cat "$root/runner.pid")" 2>/dev/null; then
    echo "runner already active: $(cat "$root/runner.pid")" >&2
    exit 7
fi

nohup bash "$root/run_role_diagnostic_isolation_227.sh" "$root" \
    >"$root/runner.stdout" 2>"$root/runner.stderr" </dev/null &
echo $! >"$root/runner.pid"
echo "runner_pid=$(cat "$root/runner.pid")"
