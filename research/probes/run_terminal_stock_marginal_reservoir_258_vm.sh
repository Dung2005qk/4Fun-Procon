#!/usr/bin/env bash
set -euo pipefail

root="${1:-/home/LMC/udon258-0831}"
manifest="$root/SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258.csv"
binary="$root/udonshield_historical_tournament"
output_dir="$root/logs258-holdout"
expected_manifest_sha="0408971890500e70d4abd2e909345c6f212a4db946eb9555c35d11fbec7d372a"
expected_binary_sha="${BINARY_SHA256:?BINARY_SHA256 is required}"

actual_manifest_sha="$(sha256sum "$manifest" | awk '{print $1}')"
actual_binary_sha="$(sha256sum "$binary" | awk '{print $1}')"
[[ "$actual_manifest_sha" == "${expected_manifest_sha,,}" ]] || {
  echo "manifest hash mismatch: $actual_manifest_sha" >&2
  exit 2
}
[[ "$actual_binary_sha" == "${expected_binary_sha,,}" ]] || {
  echo "binary hash mismatch: $actual_binary_sha" >&2
  exit 2
}

mkdir -p "$output_dir"

expected=0
while IFS=, read -r experiment split suite first_seed count budget_ms role_ms role_mode fuel_profile spot_count players public_window_ms scope authority; do
  [[ "$experiment" == "SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258" ]] || {
    echo "bad experiment: $experiment" >&2
    exit 2
  }
  [[ "$split" == "development" || "$split" == "holdout" ]] || {
    echo "bad split: $split" >&2
    exit 2
  }
  [[ "$budget_ms" == "3375" && "$role_ms" == "5000" ]] || {
    echo "bad budget: $suite/$first_seed" >&2
    exit 2
  }
  [[ "$players" == "8" || "$players" == "9" || "$players" == "10" ]] || {
    echo "bad player count: $players" >&2
    exit 2
  }
  [[ "$public_window_ms" == "5000" || "$public_window_ms" == "10000" || "$public_window_ms" == "15000" ]] || {
    echo "bad window: $public_window_ms" >&2
    exit 2
  }
  if [[ "$split" == "holdout" ]]; then
    expected=$((expected + count))
  fi
done < <(tail -n +2 "$manifest")
[[ "$expected" == "54" ]] || {
  echo "expected 54 holdout cases, got $expected" >&2
  exit 2
}

while IFS=, read -r experiment split suite first_seed count budget_ms role_ms role_mode fuel_profile spot_count players public_window_ms scope authority; do
  [[ "$split" == "holdout" ]] || continue
  for ((offset=0; offset<count; ++offset)); do
    seed=$((first_seed + offset))
    result_file="$output_dir/${suite}-${seed}.result"
    [[ -f "$result_file" ]] && continue
    partial="$output_dir/${suite}-${seed}.partial.$$"
    raw="$output_dir/${suite}-${seed}.raw.$$"
    if ! "$binary" \
      --version causal-258 \
      --track terminal-stock-marginal-reservoir-258 \
      --suite "$suite" \
      --first-seed "$seed" \
      --seeds 1 \
      --budget-ms "$budget_ms" \
      --role-ms "$role_ms" \
      --role-mode "$role_mode" \
      --role-mask 1 \
      --fuel-profile "$fuel_profile" \
      --spot-count "$spot_count" \
      --players "$players" \
      --short-role-fallback 1 \
      --protected-wait-closed-loop \
      --protected-wait-ms 1600 \
      --terminal-sparse-ms 5000 \
      --terminal-pair 1 \
      --terminal-marginal-reservoir 1 \
      --midday-chain 1 \
      --midday-pair 0 \
      --midday-target-followup 1 \
      --public-window-probe-ms "$public_window_ms" \
      --checkpoint-closed-loop 1 \
      --day-details >"$raw" 2>&1; then
      cat "$raw" >&2
      exit 3
    fi
    [[ "$(grep -c '^result,' "$raw")" == "1" ]] || {
      echo "$suite/$seed did not emit exactly one result" >&2
      cat "$raw" >&2
      exit 3
    }
    {
      echo "case_begin,suite=$suite,seed=$seed,role=$role_mode,players=$players,public_window_ms=$public_window_ms"
      cat "$raw"
      echo "case_complete,suite=$suite,seed=$seed"
    } >"$partial"
    mv "$partial" "$result_file"
    rm -f "$raw"
  done
done < <(tail -n +2 "$manifest")

actual="$(find "$output_dir" -maxdepth 1 -type f -name '*.result' | wc -l)"
[[ "$actual" == "$expected" ]] || {
  echo "completed $actual, expected $expected" >&2
  exit 4
}

combined_partial="$output_dir/SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258-holdout.log.partial.$$"
combined="$output_dir/SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258-holdout.log"
{
  echo "run_begin,experiment=SCORE-TERMINAL-STOCK-MARGINAL-RESERVOIR-258,phase=holdout,expected_results=54"
  echo "frozen,manifest_sha256=${expected_manifest_sha^^},binary_sha256=${expected_binary_sha^^}"
  find "$output_dir" -maxdepth 1 -type f -name '*.result' -print0 | sort -z | xargs -0 cat
  echo "run_complete,results=54"
} >"$combined_partial"
mv "$combined_partial" "$combined"
touch "$output_dir/run_complete"
echo "completed 54 results in $output_dir"
