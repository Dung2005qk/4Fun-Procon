#!/usr/bin/env bash
set -euo pipefail

log_path="${1:-research/evidence/PERF-PROTECTED-HASH-MEMBERSHIP-201-equivalence.log}"
core="${UDON201_CORE:-3}"
parent_binary="${UDON201_PARENT_BINARY:-build/udonshield_historical_tournament_parent}"
candidate_binary="${UDON201_CANDIDATE_BINARY:-build/udonshield_historical_tournament_candidate}"

mkdir -p "$(dirname "$log_path")"
exec > >(tee "$log_path") 2>&1
printf '%s\n' \
  "experiment=PERF-PROTECTED-HASH-MEMBERSHIP-201" \
  "split=equivalence" \
  "parent=690728a" \
  "manifest_sha256=6A073DE108F32E616C56E1A35F32B77E11F0378C45BCBD9F7ADD9B82D5FAD086"

run_case() {
  local label="$1" seed="$2" suite="$3" spots="$4" fuel="$5" agents="$6" role_mask="$7" wait_ms="$8" terminal_ms="$9"
  for mode in parent candidate; do
    local binary="$parent_binary"
    if [[ "$mode" == candidate ]]; then binary="$candidate_binary"; fi
    echo "mode_begin,label=$label,seed=$seed,mode=$mode"
    taskset -c "$core" "$binary" \
      --version "201-equivalence-$label-$mode" \
      --track protected-hash-membership-equivalence \
      --suite "$suite" \
      --first-seed "$seed" \
      --seeds 1 \
      --budget-ms 3375 \
      --role-ms 3375 \
      --role-mode fixed \
      --role-mask "$role_mask" \
      --spot-count "$spots" \
      --fuel-profile "$fuel" \
      --direct-protected-refiner "$label" \
      --protected-wait-ms "$wait_ms" \
      --terminal-sparse-ms "$terminal_ms"
  done
}

# Dense-feasible terminal control: all wait-detour enumeration receives a
# generous research-only deadline and terminal sparse refinement is unsupported.
run_case wait 4880000 stratified-easy 12 low 4 9 120000 0

# Sparse terminal case: nonterminal wait work is disabled and terminal route
# enumeration receives enough research-only time to reach its bounded frontier.
run_case terminal 4883008 stratified-very-hard 30 high 8 2 0 120000
