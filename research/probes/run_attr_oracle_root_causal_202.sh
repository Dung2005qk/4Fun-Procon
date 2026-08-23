#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
"$repo_root/build/udonshield_multi_patrol_oracle" \
    --manifest "$repo_root/research/holdouts/ATTR-ORACLE-ROOT-CAUSAL-202.csv" \
    --split consumed \
    --only-seed 1721100 \
    --inspect-plan '3.2.2.2.0.1.5.-3|-16|-16' \
    --inspect-parent-plan '3.2.2.2.0.0.2.-3|-16|-16'
