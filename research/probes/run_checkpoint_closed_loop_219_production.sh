#!/usr/bin/env bash
# Frozen post-integration protected gate for DEADLINE-CHECKPOINT-CLOSED-LOOP-219.
# Usage: run_checkpoint_closed_loop_219_production.sh <binary> <log>
set -euo pipefail

binary="$1"
log="$2"
manifest_sha="F5222CFCF850DD1858F4F3973CE4951C268DC71CFC7A882F7C82FBFE32E1BBC3"
expected_binary_sha="479EE6C9DFD25BB91D32E656ACFBcd4FB04032D2AA25A88D4ED337CEBFF2B3CC"
binary_sha="$(sha256sum "$binary" | awk '{print toupper($1)}')"
if [[ "$binary_sha" != "${expected_binary_sha^^}" ]]; then
    echo "frozen binary hash mismatch: $binary_sha" >&2
    exit 3
fi

mkdir -p "$(dirname "$log")"
{
    echo "experiment=DEADLINE-CHECKPOINT-CLOSED-LOOP-219"
    echo "stage=production-protected"
    echo "manifest_sha256=$manifest_sha"
    echo "binary_sha256=$binary_sha"
} > "$log"

fuels=(low default high)
roles=(fixed native)
run_tier() {
    local suite="$1"
    local first="$2"
    local agents="$3"
    local spots="$4"
    for ((offset=0; offset<6; ++offset)); do
        local seed=$((first + offset))
        local fuel="${fuels[$((offset % 3))]}"
        local role="${roles[$((offset % 2))]}"
        local role_mask=1
        if [[ "$fuel" == low ]]; then
            role_mask=$(( (1 << 0) | (1 << (agents - 1)) ))
        elif [[ "$fuel" == high ]]; then
            role_mask=2
        fi
        echo "case_begin,suite=$suite,seed=$seed,spots=$spots,fuel=$fuel,role=$role" >> "$log"
        "$binary" \
            --version 219-production-protected \
            --track checkpoint-closed-loop \
            --suite "$suite" \
            --first-seed "$seed" \
            --seeds 1 \
            --budget-ms 3375 \
            --role-ms 3375 \
            --role-mode "$role" \
            --role-mask "$role_mask" \
            --spot-count "$spots" \
            --fuel-profile "$fuel" \
            --protected-wait-closed-loop \
            --protected-wait-ms 1600 \
            --terminal-sparse-ms 5000 \
            --terminal-pair 1 \
            --midday-chain 1 \
            --midday-pair 0 \
            --midday-target-followup 1 \
            --public-window-probe-ms 15000 \
            --checkpoint-closed-loop 1 \
            --day-details >> "$log" 2>&1
        echo "case_complete,suite=$suite,seed=$seed" >> "$log"
    done
}

run_tier stratified-hard 5070000 6 24
run_tier stratified-very-hard 5071000 8 30
echo "run_complete,count=12" >> "$log"
