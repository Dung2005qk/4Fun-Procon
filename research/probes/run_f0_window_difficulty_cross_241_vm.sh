#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 4 ]]; then
  echo "usage: $0 MANIFEST PARENT_BINARY CANDIDATE_BINARY OUTPUT_DIR" >&2
  exit 2
fi

manifest=$1
parent_binary=$2
candidate_binary=$3
output_dir=$4

expected_manifest=a03fbb63c6221a151f274e1811b25b5f3d0aa8ed56e8d84d0b7ea95206ca67a8
expected_parent=985c2bda7872214a252a6284e7db1b1256243d28110b0061f2f244faf5950711
expected_candidate=0874dbca60fa9bebad85076bc67af0622ac4fdca3f3d85ee28847867ed5d1f6e

actual_manifest=$(sha256sum "$manifest" | awk '{print $1}')
actual_parent=$(sha256sum "$parent_binary" | awk '{print $1}')
actual_candidate=$(sha256sum "$candidate_binary" | awk '{print $1}')
[[ "$actual_manifest" == "$expected_manifest" ]] || {
  echo "manifest hash mismatch: $actual_manifest" >&2
  exit 3
}
[[ "$actual_parent" == "$expected_parent" ]] || {
  echo "parent hash mismatch: $actual_parent" >&2
  exit 3
}
[[ "$actual_candidate" == "$expected_candidate" ]] || {
  echo "candidate hash mismatch: $actual_candidate" >&2
  exit 3
}

mkdir -p "$output_dir"
header="$output_dir/frozen_hashes.txt"
if [[ ! -e "$header" ]]; then
  {
    echo "experiment=ATTR-F0-WINDOW-DIFFICULTY-CROSS-241"
    echo "manifest_sha256=$actual_manifest"
    echo "parent_sha256=$actual_parent"
    echo "candidate_sha256=$actual_candidate"
  } >"$header"
fi

expected=0
while IFS=, read -r experiment_id split suite first_seed count budget_ms role_ms role_mode fuel_profile spot_count public_window_ms scope authority; do
  [[ "$experiment_id" == "experiment_id" ]] && continue
  [[ "$split" == "development" ]] || continue
  expected=$((expected + 2 * count))
  for ((offset = 0; offset < count; ++offset)); do
    seed=$((first_seed + offset))
    window_bit=0
    if ((public_window_ms > 5000)); then
      window_bit=1
    fi
    if (((seed + window_bit) % 2 == 0)); then
      labels=(parent candidate)
    else
      labels=(candidate parent)
    fi
    for label in "${labels[@]}"; do
      if [[ "$label" == "parent" ]]; then
        binary=$parent_binary
        version=parent
      else
        binary=$candidate_binary
        version=signature
      fi
      result="$output_dir/${label}-${suite}-w${public_window_ms}-${seed}.result"
      [[ -e "$result" ]] && continue
      partial="$result.partial.$$"
      "$binary" \
        --version "$version" --track f0-window-difficulty-cross \
        --suite "$suite" --first-seed "$seed" --seeds 1 \
        --budget-ms "$budget_ms" --role-ms "$role_ms" \
        --role-mode "$role_mode" --role-mask 1 \
        --fuel-profile "$fuel_profile" --spot-count "$spot_count" \
        --short-role-fallback 1 --protected-wait-closed-loop \
        --protected-wait-ms 1600 --terminal-sparse-ms 5000 \
        --terminal-pair 1 --midday-chain 1 --midday-pair 0 \
        --midday-target-followup 1 \
        --public-window-probe-ms "$public_window_ms" \
        --checkpoint-closed-loop 1 --day-details >"$partial" 2>&1
      [[ $(grep -c '^result,' "$partial") -eq 1 ]] || {
        echo "invalid result count for $label/$suite/$public_window_ms/$seed" >&2
        exit 4
      }
      mv "$partial" "$result"
    done
  done
done <"$manifest"

actual=$(find "$output_dir" -maxdepth 1 -type f -name '*.result' | wc -l)
[[ "$actual" -eq "$expected" ]] || {
  echo "completed $actual, expected $expected" >&2
  exit 5
}
printf 'run_complete,results=%s,pairs=%s\n' "$actual" "$((actual / 2))" >"$output_dir/run_complete"
echo "completed $actual results in $output_dir"
