#!/usr/bin/env bash
# SCORE-TERMINAL-PAIR-EXCHANGE-207 same-binary causal development run.
# Usage: run_terminal_pair_207_causal.sh <binary> <side:off|on> <logdir>
set -euo pipefail
binary="$1"
side="$2"
logdir="$3"
pairFlag=0
if [ "$side" = "on" ]; then pairFlag=1; fi
log="$logdir/SCORE-TERMINAL-PAIR-EXCHANGE-207-development-causal-$side.log"
{
    echo "experiment=SCORE-TERMINAL-PAIR-EXCHANGE-207"
    echo "split=development-causal"
    echo "side=$side"
    echo "manifest_sha256=D9506BFC73120CB30679271992EDAA71D333E94879AF1A78E47A3CF04B740D79"
} > "$log"

run_tier() {
    local suite="$1" first="$2" count="$3" agents="$4"; shift 4
    local spots=("$@")
    local fuels=(low default high)
    local roles=(fixed native)
    local nspots=${#spots[@]}
    for ((offset=0; offset<count; ++offset)); do
        local seed=$((first + offset))
        local spotCount=${spots[$((offset % nspots))]}
        local fuel=${fuels[$(((offset / nspots) % 3))]}
        local role=${roles[$(((offset / (nspots * 3)) % 2))]}
        local roleMask=1
        if [ "$fuel" = "low" ]; then roleMask=$(( (1 << 0) | (1 << (agents - 1)) ));
        elif [ "$fuel" = "high" ]; then roleMask=2; fi
        local waitBudget=500 terminalBudget=3875 window=short
        if [ $((offset % 2)) -eq 1 ]; then waitBudget=1600; terminalBudget=5000; window=long; fi
        echo "case_begin,suite=$suite,seed=$seed,spots=$spotCount,fuel=$fuel,role=$role,window=$window" >> "$log"
        "$binary" \
            --version "207-causal-$side" \
            --track protected-reserve \
            --suite "$suite" \
            --first-seed "$seed" \
            --seeds 1 \
            --budget-ms 3375 \
            --role-ms 3375 \
            --role-mode "$role" \
            --role-mask "$roleMask" \
            --spot-count "$spotCount" \
            --fuel-profile "$fuel" \
            --protected-wait-closed-loop \
            --protected-wait-ms "$waitBudget" \
            --terminal-sparse-ms "$terminalBudget" \
            --terminal-pair "$pairFlag" >> "$log" 2>&1
    done
}

run_tier stratified-easy 4900000 8 4 12 14
run_tier stratified-medium 4901000 8 4 12 18
run_tier stratified-hard 4902000 20 6 18 24
run_tier stratified-very-hard 4903000 24 8 18 24 30
echo "run_complete,side=$side,split=development-causal" >> "$log"
