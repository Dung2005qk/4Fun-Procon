#!/usr/bin/env bash
set -euo pipefail
root="$(cd "$(dirname "$0")/../.." && pwd)"
binary="$root/build-release/udonshield_historical_tournament"
logdir="$root/logs218-causal"
for tier in easy medium hard very-hard; do
    "$root/research/probes/run_public_window_causal_218.sh" \
        "$binary" "$tier" off "$logdir"
    "$root/research/probes/run_public_window_causal_218.sh" \
        "$binary" "$tier" on "$logdir"
done
