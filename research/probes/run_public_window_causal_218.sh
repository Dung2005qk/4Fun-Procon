#!/usr/bin/env bash
# DEADLINE-PUBLIC-WINDOW-218 causal development: same frozen binary, off/on.
# Usage: run_public_window_causal_218.sh <binary> <tier> <side> <logdir>
set -euo pipefail

binary="$1"
tier="$2"
side="$3"
logdir="$4"
mkdir -p "$logdir"

case "$side" in
    off) apply=0 ;;
    on) apply=1 ;;
    *) echo "unknown side: $side" >&2; exit 2 ;;
esac

case "$tier" in
    easy) suite=stratified-easy; first=5021000; count=8; agents=4; spots=(12 14) ;;
    medium) suite=stratified-medium; first=5021100; count=8; agents=4; spots=(12 18) ;;
    hard) suite=stratified-hard; first=5021200; count=20; agents=6; spots=(18 24) ;;
    very-hard) suite=stratified-very-hard; first=5021300; count=24; agents=8; spots=(18 24 30) ;;
    *) echo "unknown tier: $tier" >&2; exit 2 ;;
esac

log="$logdir/DEADLINE-PUBLIC-WINDOW-218-development-causal-$tier-$side.log"
if [ ! -f "$log" ]; then
    {
        echo "experiment=DEADLINE-PUBLIC-WINDOW-218"
        echo "stage=development-causal"
        echo "tier=$tier"
        echo "side=$side"
        echo "manifest_sha256=6FF8E0776FE02497DF00448B56C321F1EAD367DA82ACB2BD0791AC16936C90A9"
    } > "$log"
fi
last_line="$(tail -n 1 "$log")"
case "$last_line" in
    case_begin,*) sed -i '$ d' "$log" ;;
esac
done_count="$(grep -c '^result' "$log" || true)"

fuels=(low default high)
roles=(fixed native)
nspots=${#spots[@]}
for ((offset=done_count; offset<count; ++offset)); do
    seed=$((first + offset))
    spot_count=${spots[$((offset % nspots))]}
    fuel=${fuels[$(((offset / nspots) % 3))]}
    role=${roles[$(((offset / (nspots * 3)) % 2))]}
    role_mask=1
    if [ "$fuel" = low ]; then
        role_mask=$(( (1 << 0) | (1 << (agents - 1)) ))
    elif [ "$fuel" = high ]; then
        role_mask=2
    fi
    echo "case_begin,suite=$suite,seed=$seed,spots=$spot_count,fuel=$fuel,role=$role,side=$side,public_ms=15000" >> "$log"
    "$binary" \
        --version 218-causal \
        --track protected-public-window \
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
        --public-window-probe-ms 15000 \
        --public-window-apply "$apply" >> "$log" 2>&1
done
if ! grep -q '^run_complete' "$log"; then
    echo "run_complete,tier=$tier,side=$side,count=$count" >> "$log"
fi
