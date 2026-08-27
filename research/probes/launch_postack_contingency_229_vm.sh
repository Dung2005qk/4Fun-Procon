#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon229-v2c-0826
binary="$root/source/build229v2c/udonshield_historical_tournament"
runner="$root/source/research/probes/run_postack_contingency_229_resume.sh"
manifest="$root/source/research/holdouts/SCORE-POSTACK-CONTINGENCY-DIVERSIFICATION-229.csv"
logdir="$root/logs229"
export BINARY_SHA256=5F1C818C22BECC3C29C1A9F8F264D8532F5F0688F648463215DC73D36829C654
runner_sha256=96E822D153211BE2A552A4305092AED156E5617537CC74FACB0CCD44060395BA
manifest_sha256=9D28F7FE1088AC6D46CB11C075D26663A7387C4497E408A6E816946C7C65CE33

actual_runner_sha256="$(sha256sum "$runner" | awk '{print toupper($1)}')"
actual_manifest_sha256="$(sha256sum "$manifest" | awk '{print toupper($1)}')"
[[ "$actual_runner_sha256" == "$runner_sha256" ]]
[[ "$actual_manifest_sha256" == "$manifest_sha256" ]]

if [[ -f "$root/runner.pid" ]] &&
   kill -0 "$(cat "$root/runner.pid")" 2>/dev/null; then
    echo "runner already active: $(cat "$root/runner.pid")"
    exit 0
fi

nohup bash -c '
    set -euo pipefail
    root=/home/LMC/udon229-v2c-0826
    export BINARY_SHA256=5F1C818C22BECC3C29C1A9F8F264D8532F5F0688F648463215DC73D36829C654
    runner="$root/source/research/probes/run_postack_contingency_229_resume.sh"
    binary="$root/source/build229v2c/udonshield_historical_tournament"
    logdir="$root/logs229"
    bash "$runner" "$binary" development off "$logdir"
    bash "$runner" "$binary" development on "$logdir"
' > "$root/runner.stdout" 2> "$root/runner.stderr" < /dev/null &
printf '%s\n' "$!" > "$root/runner.pid"
echo "started runner $(cat "$root/runner.pid")"
