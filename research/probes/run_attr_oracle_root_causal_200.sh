#!/usr/bin/env bash
set -euo pipefail

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"
"$repo_root/build/udonshield_multi_patrol_oracle" \
    --manifest "$repo_root/research/holdouts/ATTR-ORACLE-ROOT-CAUSAL-200.csv" \
    --split consumed \
    --only-seed 1721200 \
    --inspect-plan '3.2.2.2.0.0.5.5.-2|-17|-17' \
    --inspect-parent-plan '3.2.2.2.0.1.5.5.5|-17|-17'
