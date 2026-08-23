#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
binary="$repo_root/build/udonshield_multi_patrol_oracle"
manifest="$repo_root/research/holdouts/ATTR-ORACLE-LATEST-RESERVE-199.csv"
log="$repo_root/research/evidence/ATTR-ORACLE-LATEST-RESERVE-199.log"

: > "$log"
for reserve_ms in 1100 0; do
    for seed in 1721100 1721200; do
        echo "run_begin,seed=$seed,protected_reserve_ms=$reserve_ms" | tee -a "$log"
        "$binary" \
            --manifest "$manifest" \
            --split consumed \
            --only-seed "$seed" \
            --protected-head \
            --latest-terminal \
            --protected-refinement-reserve-ms "$reserve_ms" \
            2>&1 | tee -a "$log"
    done
done
sha256sum "$log"
