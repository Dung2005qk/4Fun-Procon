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
        raise SystemExit(
            f"unpaired result counts: off={len(off_rows)} on={len(on_rows)}"
        )

    wins = ties = losses = 0
    score_delta = [0, 0, 0]
    first_tiers = collections.Counter()
    strata = collections.defaultdict(lambda: [0, 0, 0, 0])
    differences = []
    activation_cases = reuse_cases = 0
    differing_with_activation = differing_without_activation = 0
    differing_with_reuse = differing_without_reuse = 0
    proof_fields = (
        "post_ack_completed_proofs",
        "post_ack_strong_proof_records",
    )
    proof_coverage = {
        field: {"gtl": [0, 0, 0], "delta": 0, "losses": []}
        for field in proof_fields
    }

    for index, (off, on) in enumerate(zip(off_rows, on_rows)):
        identity = (
            off["suite"],
            off["seed"],
            off["spot_count"],
            off["fuel_profile"],
            off["role_mode"],
        )
        other = (
            on["suite"],
            on["seed"],
            on["spot_count"],
            on["fuel_profile"],
            on["role_mode"],
        )
        if identity != other:
            raise SystemExit(
                f"pair identity mismatch at {index}: {identity} != {other}"
            )

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

        activated = int(on.get("post_ack_proof_witness_contingencies", "0")) > 0
        reused = int(on.get("proof_witness_reused", "0")) > 0
        activation_cases += int(activated)
        reuse_cases += int(reused)
        if tier:
            differing_with_activation += int(activated)
            differing_without_activation += int(not activated)
            differing_with_reuse += int(reused)
            differing_without_reuse += int(not reused)
            differences.append(
                {
                    "seed": int(off["seed"]),
                    "suite": off["suite"],
                    "fuel": off["fuel_profile"],
                    "role": off["role_mode"],
                    "tier": tier,
                    "delta": delta,
                    "activated": activated,
                    "reused": reused,
                    "proof_witnesses": int(
                        on.get("post_ack_proof_witness_contingencies", "0")
                    ),
                    "proof_witness_eligible": int(
                        on.get("proof_witness_eligible", "0")
                    ),
                    "proof_witness_reused": int(
                        on.get("proof_witness_reused", "0")
                    ),
                    "proof_witness_rejected": int(
                        on.get("proof_witness_rejected", "0")
                    ),
                }
            )

        for field in proof_fields:
            before_value = int(off.get(field, "0"))
            after_value = int(on.get(field, "0"))
            proof_delta = after_value - before_value
            proof_coverage[field]["delta"] += proof_delta
            proof_outcome = 0 if proof_delta > 0 else (2 if proof_delta < 0 else 1)
            proof_coverage[field]["gtl"][proof_outcome] += 1
            if proof_delta < 0:
                proof_coverage[field]["losses"].append(
                    {
                        "seed": int(off["seed"]),
                        "suite": off["suite"],
                        "delta": proof_delta,
                    }
                )

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
        "invalid",
        "emergency",
        "checkpoint_closed_loop_failures",
        "midday_failure_days",
        "terminal_sparse_failure",
    )
    telemetry_fields = (
        "post_ack_calls",
        "post_ack_contingencies",
        "post_ack_proof_calls",
        "post_ack_completed_proofs",
        "post_ack_strong_proof_records",
        "post_ack_proof_witness_contingencies",
        "proof_witness_eligible",
        "proof_witness_reused",
        "proof_witness_rejected",
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
            "witness_cases": activation_cases,
            "reuse_cases": reuse_cases,
            "score_differences_with_witness": differing_with_activation,
            "score_differences_without_witness": differing_without_activation,
            "score_differences_with_reuse": differing_with_reuse,
            "score_differences_without_reuse": differing_without_reuse,
        },
        "proof_coverage": proof_coverage,
        "strata": {key: value for key, value in sorted(strata.items())},
        "differences": differences,
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit(
            "usage: summarize_postack_proof_witness_230.py OFF_LOG ON_LOG"
        )
    summarize(sys.argv[1], sys.argv[2])
