#!/usr/bin/env bash
set -euo pipefail

split="${1:-development}"
log_path="${2:-research/evidence/PERF-PROTECTED-HASH-MEMBERSHIP-201-${split}.log}"
core="${UDON201_CORE:-3}"
parent_binary="${UDON201_PARENT_BINARY:-build/udonshield_historical_tournament_parent}"
candidate_binary="${UDON201_CANDIDATE_BINARY:-build/udonshield_historical_tournament_candidate}"

case "$split" in
  development)
    tiers=(
      "stratified-easy 4880000 12 4 12,14"
      "stratified-medium 4881000 12 4 12,18"
      "stratified-hard 4882000 18 6 14,18,24"
      "stratified-very-hard 4883000 18 8 18,24,30"
    )
    ;;
  holdout)
    tiers=(
      "stratified-easy 4890000 24 4 12,14"
      "stratified-medium 4891000 24 4 12,18"
      "stratified-hard 4892000 30 6 14,18,24"
      "stratified-very-hard 4893000 30 8 18,24,30"
    )
    ;;
  *)
    echo "split must be development or holdout" >&2
    exit 2
    ;;
esac

mkdir -p "$(dirname "$log_path")"
exec > >(tee "$log_path") 2>&1
printf '%s\n' \
  "experiment=PERF-PROTECTED-HASH-MEMBERSHIP-201" \
  "split=$split" \
  "parent=690728a" \
  "manifest_sha256=6A073DE108F32E616C56E1A35F32B77E11F0378C45BCBD9F7ADD9B82D5FAD086"

fuels=(low default high)
roles=(fixed native)

for tier in "${tiers[@]}"; do
  read -r suite first count agents spot_csv <<<"$tier"
  IFS=',' read -r -a spots <<<"$spot_csv"
  spot_count=${#spots[@]}
  for ((offset = 0; offset < count; ++offset)); do
    seed=$((first + offset))
    spot="${spots[$((offset % spot_count))]}"
    fuel="${fuels[$(((offset / spot_count) % ${#fuels[@]}))]}"
    role="${roles[$(((offset / (spot_count * ${#fuels[@]})) % ${#roles[@]}))]}"
    if [[ "$fuel" == low ]]; then
      role_mask=$((1 | (1 << (agents - 1))))
    elif [[ "$fuel" == high ]]; then
      role_mask=2
    else
      role_mask=1
    fi
    if ((offset % 2 == 0)); then
      modes=(parent candidate)
      wait_budget=500
      terminal_budget=3875
      window=short
    else
      modes=(candidate parent)
      wait_budget=1600
      terminal_budget=5000
      window=long
    fi
    echo "case_begin,suite=$suite,seed=$seed,spots=$spot,fuel=$fuel,role=$role,window=$window,order=${modes[0]}|${modes[1]}"
    for mode in "${modes[@]}"; do
      if [[ "$mode" == parent ]]; then
        binary="$parent_binary"
      else
        binary="$candidate_binary"
      fi
      echo "mode_begin,seed=$seed,mode=$mode"
      taskset -c "$core" "$binary" \
        --version "201-$split-$mode" \
        --track protected-hash-membership \
        --suite "$suite" \
        --first-seed "$seed" \
        --seeds 1 \
        --budget-ms 3375 \
        --role-ms 3375 \
        --role-mode "$role" \
        --role-mask "$role_mask" \
        --spot-count "$spot" \
        --fuel-profile "$fuel" \
        --protected-wait-closed-loop \
        --protected-wait-ms "$wait_budget" \
        --terminal-sparse-ms "$terminal_budget"
    done
  done
done
