#!/usr/bin/env bash
# SCORE-MIDDAY-TARGET-FOLLOWUP-215 development resume wrapper. Both sides run
# the accepted 210 global-pool prefix; only the on side enables its protected
# target-terminal suffix. Safe to resume after Spot preemption.
# Usage: run_midday_target_215_resume.sh <binary> <side:off|on> <logdir>
set -euo pipefail
binary="$1"
side="$2"
logdir="$3"
target_flag=0
if [ "$side" = "on" ]; then target_flag=1; fi
log="$logdir/SCORE-MIDDAY-TARGET-FOLLOWUP-215-development-causal-$side.log"

if [ ! -f "$log" ]; then
    {
        echo "experiment=SCORE-MIDDAY-TARGET-FOLLOWUP-215"
        echo "split=development-causal"
        echo "side=$side"
        echo "manifest_sha256=908C08A5E13AEE08C764F48DD444C98D1568FB5FF8145D471DE1262EF0D1EC14"
    } > "$log"
fi
last_line="$(tail -n 1 "$log")"
case "$last_line" in
    case_begin,*) sed -i '$ d' "$log"; echo "resume: dropped dangling case_begin ($last_line)";;
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
        local spot_count=${spots[$((offset % nspots))]}
        local fuel=${fuels[$(((offset / nspots) % 3))]}
        local role=${roles[$(((offset / (nspots * 3)) % 2))]}
        local role_mask=1
        if [ "$fuel" = "low" ]; then role_mask=$(( (1 << 0) | (1 << (agents - 1)) ));
        elif [ "$fuel" = "high" ]; then role_mask=2; fi
        local wait_budget=500 terminal_budget=3875 window=short
        if [ $((offset % 2)) -eq 1 ]; then
            wait_budget=1600
            terminal_budget=5000
            window=long
        fi
        echo "case_begin,suite=$suite,seed=$seed,spots=$spot_count,fuel=$fuel,role=$role,window=$window" >> "$log"
        "$binary" \
            --version "215-causal-$side" \
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
            --protected-wait-ms "$wait_budget" \
            --terminal-sparse-ms "$terminal_budget" \
            --terminal-pair 1 \
            --midday-chain 1 \
            --midday-pair 0 \
            --midday-target-followup "$target_flag" >> "$log" 2>&1
    done
}

run_tier stratified-easy 4980000 8 4 12 14
run_tier stratified-medium 4981000 8 4 12 18
run_tier stratified-hard 4982000 20 6 18 24
run_tier stratified-very-hard 4983000 24 8 18 24 30
if ! grep -q '^run_complete' "$log"; then
    echo "run_complete,side=$side,split=development-causal" >> "$log"
fi
