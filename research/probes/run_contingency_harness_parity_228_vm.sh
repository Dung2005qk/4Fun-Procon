#!/usr/bin/env bash
set -euo pipefail

root=/home/LMC/udon228-0826
replay="$root/m-4195.jsonl"
logs="$root/logs228"
parent="$root/buggy226/build228/udonshield_btc"
candidate="$root/cache-faithful/build228/udonshield_btc"

mkdir -p "$logs"
sha256sum "$replay" "$parent" "$candidate" > "$logs/frozen-sha256.txt"

run_lane() {
    local lane="$1"
    local repetition="$2"
    local binary="$3"
    local post_ack_ms="$4"
    local output="$logs/${lane}-$(printf '%02d' "$repetition").log"
    local stderr="$logs/${lane}-$(printf '%02d' "$repetition").stderr"
    if [[ -s "$output" ]] && grep -q '^summary ' "$output"; then
        return
    fi
    local -a command=(
        "$binary" replay-counterfactual
        --replay "$replay"
        --role-mask 4
        --response-ms 5000
        --short-role-fallback 1
    )
    if (( post_ack_ms > 0 )); then
        command+=(--post-ack-ms "$post_ack_ms")
    fi
    "${command[@]}" \
        > "$output.partial" 2> "$stderr.partial"
    mv "$output.partial" "$output"
    mv "$stderr.partial" "$stderr"
}

for repetition in $(seq 1 24); do
    if (( repetition <= 12 )); then
        run_lane no-ack-control "$repetition" "$candidate" 0
        run_lane buggy-226 "$repetition" "$parent" 2000
    fi
    run_lane cache-faithful "$repetition" "$candidate" 2000
done

touch "$logs/run_complete"
