#!/usr/bin/env bash
# SCORE-ACTUAL-ACK-TRANSITION-ANCHOR-231 resumable paired runner.
# Usage: run_actual_ack_transition_anchor_231_resume.sh <binary> <development|holdout> <off|on> <logdir>
set -euo pipefail

binary="$1"
split="$2"
side="$3"
logdir="$4"
manifest_sha256=F7E6DB5791C2FBEB7D9F60B29525004712CB77F9C6E64D5CB8A42D975A644B04

if [[ "$split" != development && "$split" != holdout ]]; then
    echo "split must be development or holdout" >&2
    exit 2
fi
if [[ "$side" != off && "$side" != on ]]; then
    echo "side must be off or on" >&2
    exit 2
fi
if [[ -z "${BINARY_SHA256:-}" ]]; then
    echo "BINARY_SHA256 must freeze the tested binary" >&2
    exit 2
fi
actual_binary_sha256="$(sha256sum "$binary" | awk '{print toupper($1)}')"
if [[ "$actual_binary_sha256" != "$BINARY_SHA256" ]]; then
    echo "binary hash mismatch: $actual_binary_sha256 != $BINARY_SHA256" >&2
    exit 2
fi

mkdir -p "$logdir"
flag=0
if [[ "$side" == on ]]; then flag=1; fi
log="$logdir/SCORE-ACTUAL-ACK-TRANSITION-ANCHOR-231-$split-causal-$side.log"
if [[ ! -f "$log" ]]; then
    {
        echo "experiment=SCORE-ACTUAL-ACK-TRANSITION-ANCHOR-231"
        echo "split=$split-causal"
        echo "side=$side"
        echo "manifest_sha256=$manifest_sha256"
        echo "binary_sha256=$BINARY_SHA256"
    } > "$log"
fi

last_line="$(tail -n 1 "$log")"
case "$last_line" in
    case_begin,*)
        sed -i '$ d' "$log"
        echo "resume: dropped dangling case_begin ($last_line)"
        ;;
esac

done_count="$(grep -c '^result' "$log" || true)"
echo "resume: split=$split side=$side completed=$done_count"
global_index=0

run_tier() {
    local suite="$1" first="$2" count="$3" agents="$4"; shift 4
    local spots=("$@")
    local fuels=(low default high)
    local roles=(fixed native)
    local nspots=${#spots[@]}
    for ((offset=0; offset<count; ++offset)); do
        if (( global_index < done_count )); then
            global_index=$((global_index + 1))
            continue
        fi
        global_index=$((global_index + 1))
        local seed=$((first + offset))
        local spot_count=${spots[$((offset % nspots))]}
        local fuel=${fuels[$(((offset / nspots) % 3))]}
        local role=${roles[$(((offset / (nspots * 3)) % 2))]}
        local role_mask=1
        if [[ "$fuel" == low ]]; then
            role_mask=$(( (1 << 0) | (1 << (agents - 1)) ))
        elif [[ "$fuel" == high ]]; then
            role_mask=2
        fi
        echo "case_begin,suite=$suite,seed=$seed,spots=$spot_count,fuel=$fuel,role=$role" >> "$log"
        "$binary" \
            --version "231-$split-$side" \
            --track protected-reserve \
            --suite "$suite" \
            --first-seed "$seed" \
            --seeds 1 \
            --budget-ms 3375 \
            --role-ms 3375 \
            --role-mode "$role" \
            --role-mask "$role_mask" \
            --spot-count "$spot_count" \
            --fuel-profile "$fuel" \
            --protected-wait-closed-loop \
            --protected-wait-ms 1600 \
            --terminal-sparse-ms 5000 \
            --terminal-pair 1 \
            --midday-chain 1 \
            --midday-pair 0 \
            --midday-target-followup 1 \
            --short-role-fallback 1 \
            --post-ack-ms 1600 \
            --post-ack-slice-ms 100 \
            --actual-ack-transition-anchor "$flag" >> "$log" 2>&1
    done
}

if [[ "$split" == development ]]; then
    run_tier stratified-easy 5340000 8 4 12 14
    run_tier stratified-medium 5341000 8 4 12 18
    run_tier stratified-hard 5342000 20 6 18 24
    run_tier stratified-very-hard 5343000 24 8 18 24 30
else
    run_tier stratified-easy 5350000 16 4 12 14
    run_tier stratified-medium 5351000 16 4 12 18
    run_tier stratified-hard 5352000 36 6 18 24
    run_tier stratified-very-hard 5353000 40 8 18 24 30
fi

if ! grep -q '^run_complete' "$log"; then
    echo "run_complete,split=$split-causal,side=$side,count=$global_index" >> "$log"
fi
