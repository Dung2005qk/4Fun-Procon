#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
binary="$root/build-release/udonshield_historical_tournament"
logdir="$root/logs218"
for tier in easy medium hard very-hard; do
    "$root/research/probes/run_public_window_probe_218.sh" \
        "$binary" "$tier" "$logdir"
done
