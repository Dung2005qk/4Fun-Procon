#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon232-0827
cd "$root"
if pgrep -f "[l]aunch.sh" >/dev/null ||
   pgrep -f "[u]donshield_historical_tournament.*232-" >/dev/null; then
    echo "experiment 232 is already running" >&2
    exit 2
fi
if [[ -s runner.stderr ]]; then
    mv runner.stderr runner.invalid-permission-launch.stderr
fi
nohup bash launch.sh > launcher.stdout 2> runner.stderr < /dev/null &
echo "$!" > runner.pid
