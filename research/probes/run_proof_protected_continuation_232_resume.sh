#!/usr/bin/env bash
# SCORE-PROOF-PROTECTED-CONTINUATION-232 resumable paired runner.
# Usage: run_proof_protected_continuation_232_resume.sh <binary> <development-control|development|holdout> <off|on> <logdir>
set -euo pipefail

binary="$1"
split="$2"
side="$3"
logdir="$4"
manifest_sha256=42CC8EEEF031C27C85BA7168C2187AEA7A60AED3E60061213743D0497763F670

case "$split" in
    development-control|development|holdout) ;;
    *) echo "invalid split: $split" >&2; exit 2 ;;
esac
case "$side" in
    off) enabled=0 ;;
    on) enabled=1 ;;
    *) echo "invalid side: $side" >&2; exit 2 ;;
esac
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
log="$logdir/SCORE-PROOF-PROTECTED-CONTINUATION-232-$split-causal-$side.log"
if [[ ! -f "$log" ]]; then
    {
        echo "experiment=SCORE-PROOF-PROTECTED-CONTINUATION-232"
        echo "split=$split-causal"
        echo "side=$side"
        echo "manifest_sha256=$manifest_sha256"
        echo "binary_sha256=$BINARY_SHA256"
    } > "$log"
fi

last_line="$(tail -n 1 "$log")"
case "$last_line" in
    case_begin,*) sed -i '$ d' "$log" ;;
esac
done_count="$(grep -c '^result' "$log" || true)"
global_index=0

run_tier() {
    local suite="$1" first="$2" count="$3" agents="$4" public_ms="$5"; shift 5
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
        echo "case_begin,suite=$suite,seed=$seed,spots=$spot_count,fuel=$fuel,role=$role,public_ms=$public_ms" >> "$log"
        "$binary" \
            --version "232-$split-$side" \
            --track checkpoint-closed-loop \
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
            --public-window-probe-ms "$public_ms" \
            --checkpoint-closed-loop 1 \
            --post-ack-ms 1600 \
            --post-ack-slice-ms 100 \
            --proof-protected-continuation "$enabled" \
            --day-details >> "$log" 2>&1
    done
}

if [[ "$split" == development-control ]]; then
    run_tier stratified-easy 5360000 4 4 5000 12 14
    run_tier stratified-medium 5361000 4 4 5000 12 18
    run_tier stratified-hard 5362000 8 6 5000 18 24
    run_tier stratified-very-hard 5363000 8 8 5000 18 24 30
elif [[ "$split" == development ]]; then
    run_tier stratified-easy 5364000 8 4 15000 12 14
    run_tier stratified-medium 5365000 8 4 15000 12 18
    run_tier stratified-hard 5366000 20 6 15000 18 24
    run_tier stratified-very-hard 5367000 24 8 15000 18 24 30
else
    run_tier stratified-easy 5370000 16 4 15000 12 14
    run_tier stratified-medium 5371000 16 4 15000 12 18
    run_tier stratified-hard 5372000 36 6 15000 18 24
    run_tier stratified-very-hard 5373000 40 8 15000 18 24 30
fi

if ! grep -q '^run_complete' "$log"; then
    echo "run_complete,split=$split-causal,side=$side,count=$global_index" >> "$log"
fi
