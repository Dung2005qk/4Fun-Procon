#!/usr/bin/env bash
# Usage: run_checkpoint_closed_loop_219_all.sh <binary> <logdir>
set -euo pipefail

binary="$1"
logdir="$2"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

for tier in easy medium hard very-hard; do
    "$script_dir/run_checkpoint_closed_loop_219.sh" \
        "$binary" control "$tier" "$logdir"
done
for tier in easy medium hard very-hard; do
    "$script_dir/run_checkpoint_closed_loop_219.sh" \
        "$binary" public15 "$tier" "$logdir"
done
