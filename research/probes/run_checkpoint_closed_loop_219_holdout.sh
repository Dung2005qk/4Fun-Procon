#!/usr/bin/env bash
# One-time sealed holdout runner for DEADLINE-CHECKPOINT-CLOSED-LOOP-219.
# Usage: run_checkpoint_closed_loop_219_holdout.sh <binary> <tier> <logdir>
set -euo pipefail

binary="$1"
tier="$2"
logdir="$3"
mkdir -p "$logdir"

manifest_sha="48200C4B086EBE73DFC0E476A83ECDB72118B3BF1D24871318B70A29DC0D4794"
binary_sha="$(sha256sum "$binary" | awk '{print toupper($1)}')"

case "$tier" in
    easy) suite=stratified-easy; first=5060000; count=16; agents=4; spots=(12 14) ;;
    medium) suite=stratified-medium; first=5061000; count=16; agents=4; spots=(12 18) ;;
    hard) suite=stratified-hard; first=5062000; count=36; agents=6; spots=(18 24) ;;
    very-hard) suite=stratified-very-hard; first=5063000; count=40; agents=8; spots=(18 24 30) ;;
    *) echo "unknown tier: $tier" >&2; exit 2 ;;
esac

log="$logdir/DEADLINE-CHECKPOINT-CLOSED-LOOP-219-holdout-public15-$tier.log"
if [ ! -f "$log" ]; then
    {
        echo "experiment=DEADLINE-CHECKPOINT-CLOSED-LOOP-219"
        echo "stage=sealed-holdout"
        echo "tier=$tier"
        echo "manifest_sha256=$manifest_sha"
        echo "binary_sha256=$binary_sha"
    } > "$log"
fi
if ! grep -q "^binary_sha256=$binary_sha$" "$log"; then
    echo "binary hash mismatch for existing holdout log: $log" >&2
    exit 3
fi

last_complete="$(
    (grep '^case_complete,' "$log" || true) |
        tail -n 1 |
        sed -n 's/.*offset=\([0-9][0-9]*\).*/\1/p'
)"
if [ -z "$last_complete" ]; then
    next_offset=0
else
    next_offset=$((last_complete + 1))
fi
if [ "$next_offset" -lt "$count" ]; then
    tmp="$log.resume"
    if [ "$next_offset" -eq 0 ]; then
        sed -n '1,/^binary_sha256=/p' "$log" > "$tmp"
    else
        previous=$((next_offset - 1))
        awk -v previous="$previous" '
            { print }
            $0 ~ ("^case_complete,offset=" previous "(,|$)") { exit }
        ' "$log" > "$tmp"
    fi
    mv "$tmp" "$log"
fi

fuels=(low default high)
roles=(fixed native)
nspots=${#spots[@]}
for ((offset=next_offset; offset<count; ++offset)); do
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
    echo "case_begin,offset=$offset,suite=$suite,seed=$seed,spots=$spot_count,fuel=$fuel,role=$role,side=on,public_ms=15000" >> "$log"
    "$binary" \
        --version 219-holdout-on \
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
        --public-window-probe-ms 15000 \
        --checkpoint-closed-loop 1 \
        --day-details >> "$log" 2>&1
    echo "case_complete,offset=$offset,seed=$seed" >> "$log"
done

if ! grep -q '^run_complete' "$log"; then
    echo "run_complete,stage=holdout,tier=$tier,count=$count" >> "$log"
fi
