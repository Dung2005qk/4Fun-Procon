#!/usr/bin/env bash
# SCORE-MIDDAY-CHAIN-ADOPTION-210 development resume wrapper. Same-binary
# causal A/B: both sides run the accepted production configuration
# (terminal pair exchange ON); the sides differ only in --midday-chain 0|1.
# Replays the exact case order, skipping cases that already have a result
# line in the side log. Safe to re-invoke after spot preemption.
# Usage: run_midday_chain_210_resume.sh <binary> <side:off|on> <logdir>
set -euo pipefail
binary="$1"
side="$2"
logdir="$3"
middayFlag=0
if [ "$side" = "on" ]; then middayFlag=1; fi
log="$logdir/SCORE-MIDDAY-CHAIN-ADOPTION-210-development-causal-$side.log"

if [ ! -f "$log" ]; then
    {
        echo "experiment=SCORE-MIDDAY-CHAIN-ADOPTION-210"
        echo "split=development-causal"
        echo "side=$side"
        echo "manifest_sha256=33E1591047103CE10BFF4A35A7A488826B8CB9B321BDF596AF0DDECFE110D4FB"
    } > "$log"
fi
# Drop a dangling case_begin left by an interrupted case (no result after it).
lastLine="$(tail -n 1 "$log")"
case "$lastLine" in
    case_begin,*) sed -i '$ d' "$log"; echo "resume: dropped dangling case_begin ($lastLine)";;
esac

done_count="$(grep -c '^result' "$log" || true)"
echo "resume: side=$side completed=$done_count"
global_index=0

run_tier() {
    local suite="$1" first="$2" count="$3" agents="$4"; shift 4
    local spots=("$@")
    local fuels=(low default high)
    local roles=(fixed native)
    local nspots=${#spots[@]}
    for ((offset=0; offset<count; ++offset)); do
        if [ "$global_index" -lt "$done_count" ]; then
            global_index=$((global_index + 1))
            continue
        fi
        global_index=$((global_index + 1))
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
            --version "210-causal-$side" \
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
            --terminal-pair 1 \
            --midday-chain "$middayFlag" >> "$log" 2>&1
    done
}

run_tier stratified-easy 4920000 8 4 12 14
run_tier stratified-medium 4921000 8 4 12 18
run_tier stratified-hard 4922000 20 6 18 24
run_tier stratified-very-hard 4923000 24 8 18 24 30
if ! grep -q '^run_complete' "$log"; then
    echo "run_complete,side=$side,split=development-causal" >> "$log"
fi
