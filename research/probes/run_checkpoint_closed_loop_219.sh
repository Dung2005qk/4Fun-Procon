#!/usr/bin/env bash
# DEADLINE-CHECKPOINT-CLOSED-LOOP-219 development runner.
# Usage: run_checkpoint_closed_loop_219.sh <binary> <phase> <tier> <logdir>
set -euo pipefail

binary="$1"
phase="$2"
tier="$3"
logdir="$4"
mkdir -p "$logdir"

manifest_sha="48200C4B086EBE73DFC0E476A83ECDB72118B3BF1D24871318B70A29DC0D4794"
binary_sha="$(sha256sum "$binary" | awk '{print toupper($1)}')"

case "$phase:$tier" in
    control:easy) suite=stratified-easy; first=5050000; count=4; agents=4; spots=(12 14); public_ms=5000 ;;
    control:medium) suite=stratified-medium; first=5050100; count=4; agents=4; spots=(12 18); public_ms=5000 ;;
    control:hard) suite=stratified-hard; first=5050200; count=8; agents=6; spots=(18 24); public_ms=5000 ;;
    control:very-hard) suite=stratified-very-hard; first=5050300; count=8; agents=8; spots=(18 24 30); public_ms=5000 ;;
    public15:easy) suite=stratified-easy; first=5051000; count=8; agents=4; spots=(12 14); public_ms=15000 ;;
    public15:medium) suite=stratified-medium; first=5051100; count=8; agents=4; spots=(12 18); public_ms=15000 ;;
    public15:hard) suite=stratified-hard; first=5051200; count=20; agents=6; spots=(18 24); public_ms=15000 ;;
    public15:very-hard) suite=stratified-very-hard; first=5051300; count=24; agents=8; spots=(18 24 30); public_ms=15000 ;;
    *) echo "unknown phase/tier: $phase/$tier" >&2; exit 2 ;;
esac

log="$logdir/DEADLINE-CHECKPOINT-CLOSED-LOOP-219-development-$phase-$tier.log"
if [ ! -f "$log" ]; then
    {
        echo "experiment=DEADLINE-CHECKPOINT-CLOSED-LOOP-219"
        echo "stage=development"
        echo "phase=$phase"
        echo "tier=$tier"
        echo "manifest_sha256=$manifest_sha"
        echo "binary_sha256=$binary_sha"
    } > "$log"
fi

if ! grep -q "^binary_sha256=$binary_sha$" "$log"; then
    echo "binary hash mismatch for existing log: $log" >&2
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

    run_case() {
        local enabled="$1"
        local label="$2"
        echo "case_begin,offset=$offset,suite=$suite,seed=$seed,spots=$spot_count,fuel=$fuel,role=$role,side=$label,public_ms=$public_ms" >> "$log"
        "$binary" \
            --version "219-$phase-$label" \
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
            --checkpoint-closed-loop "$enabled" \
            --day-details >> "$log" 2>&1
    }

    # The <=5000 ms control is an in-process identity check.  Independent
    # timed off/on processes can end at different search cutoffs under host
    # load even though the 219 branch is inactive, so they are not a valid
    # byte-equivalence oracle.  The enabled run reports both its authoritative
    # score and checkpoint_closed_loop_parent from the same execution.
    run_case 1 on
    echo "case_complete,offset=$offset,seed=$seed" >> "$log"
done

if ! grep -q '^run_complete' "$log"; then
    echo "run_complete,phase=$phase,tier=$tier,count=$count" >> "$log"
fi
