#!/usr/bin/env python3
import collections
import json
import pathlib
import sys


def parse_result(line):
    fields = {}
    for item in line.strip().split(",")[1:]:
        if "=" in item:
            key, value = item.split("=", 1)
            fields[key] = value
    return fields


def load(path):
    return [
        parse_result(line)
        for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines()
        if line.startswith("result,")
    ]


def score(row):
    return tuple(int(row[key]) for key in ("lifetime", "daily", "servings"))


def first_tier(left, right):
    for index, (a, b) in enumerate(zip(left, right), 1):
        if a != b:
            return index
    return 0


def summarize(off_path, on_path):
    off_rows = load(off_path)
    on_rows = load(on_path)
    if len(off_rows) != len(on_rows):
        raise SystemExit(f"unpaired result counts: off={len(off_rows)} on={len(on_rows)}")

    wins = ties = losses = 0
    score_delta = [0, 0, 0]
    first_tiers = collections.Counter()
    strata = collections.defaultdict(lambda: [0, 0, 0, 0])
    differences = []
    activation_cases = 0
    differences_with_activation = 0

    for index, (off, on) in enumerate(zip(off_rows, on_rows)):
        identity = (
            off["suite"], off["seed"], off["spot_count"],
            off["fuel_profile"], off["role_mode"],
        )
        other = (
            on["suite"], on["seed"], on["spot_count"],
            on["fuel_profile"], on["role_mode"],
        )
        if identity != other:
            raise SystemExit(f"pair identity mismatch at {index}: {identity} != {other}")
        before, after = score(off), score(on)
        tier = first_tier(before, after)
        first_tiers[tier] += 1
        if after > before:
            outcome = 0
            wins += 1
        elif after < before:
            outcome = 2
            losses += 1
        else:
            outcome = 1
            ties += 1
        delta = tuple(after[i] - before[i] for i in range(3))
        score_delta = [score_delta[i] + delta[i] for i in range(3)]
        activated = int(on.get("proof_protected_acceptances", "0")) > 0
        activation_cases += int(activated)
        differences_with_activation += int(tier != 0 and activated)
        if tier:
            differences.append({
                "seed": int(off["seed"]),
                "suite": off["suite"],
                "fuel": off["fuel_profile"],
                "role": off["role_mode"],
                "tier": tier,
                "delta": delta,
                "activated": activated,
                "eligible": int(on.get("proof_protected_eligible", "0")),
                "valid": int(on.get("proof_protected_valid", "0")),
                "accepted": int(on.get("proof_protected_acceptances", "0")),
            })
        for label in (
            "suite:" + off["suite"],
            "fuel:" + off["fuel_profile"],
            "role:" + off["role_mode"],
            "family:" + off["family"],
        ):
            bucket = strata[label]
            bucket[outcome] += 1
            bucket[3] += delta[tier - 1] if tier else 0

    safety_fields = (
        "invalid", "emergency", "checkpoint_closed_loop_failures",
        "midday_failure_days", "terminal_sparse_failure",
        "proof_protected_failures",
    )
    telemetry_fields = (
        "proof_protected_eligible", "proof_protected_valid",
        "proof_protected_acceptances", "proof_protected_failures",
        "post_ack_calls", "post_ack_contingencies", "post_ack_proof_calls",
        "post_ack_completed_proofs", "post_ack_strong_proof_records",
        "post_ack_proof_paths",
    )
    result = {
        "pairs": len(off_rows),
        "wtl": [wins, ties, losses],
        "score_delta": score_delta,
        "first_difference_tiers": dict(sorted(first_tiers.items())),
        "safety": {
            side: {
                field: sum(int(row.get(field, "0")) for row in rows)
                for field in safety_fields
            }
            for side, rows in (("off", off_rows), ("on", on_rows))
        },
        "telemetry": {
            side: {
                field: sum(int(row.get(field, "0")) for row in rows)
                for field in telemetry_fields
            }
            for side, rows in (("off", off_rows), ("on", on_rows))
        },
        "activation": {
            "cases": activation_cases,
            "score_differences_with_activation": differences_with_activation,
            "score_differences_without_activation": len(differences) - differences_with_activation,
        },
        "strata": {key: value for key, value in sorted(strata.items())},
        "differences": differences,
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: summarize_proof_protected_continuation_232.py OFF_LOG ON_LOG")
    summarize(sys.argv[1], sys.argv[2])
