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
    rows = []
    for line in pathlib.Path(path).read_text(encoding="utf-8").splitlines():
        if line.startswith("result,"):
            rows.append(parse_result(line))
    return rows


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
    deltas = [0, 0, 0]
    first_tiers = collections.Counter()
    strata = collections.defaultdict(lambda: [0, 0, 0, 0])
    tails = []
    activation_cases = 0
    differing_with_activation = 0
    differing_without_activation = 0
    proof_fields = ("post_ack_completed_proofs", "post_ack_strong_proof_records")
    proof_coverage = {
        field: {"gtl": [0, 0, 0], "delta": 0, "losses": []}
        for field in proof_fields
    }
    for index, (off, on) in enumerate(zip(off_rows, on_rows)):
        identity = (off["suite"], off["seed"], off["spot_count"], off["fuel_profile"], off["role_mode"])
        other = (on["suite"], on["seed"], on["spot_count"], on["fuel_profile"], on["role_mode"])
        if identity != other:
            raise SystemExit(f"pair identity mismatch at {index}: {identity} != {other}")
        a, b = score(off), score(on)
        tier = first_tier(a, b)
        first_tiers[tier] += 1
        if b > a:
            outcome = 0
            wins += 1
        elif b < a:
            outcome = 2
            losses += 1
        else:
            outcome = 1
            ties += 1
        delta = tuple(b[i] - a[i] for i in range(3))
        deltas = [deltas[i] + delta[i] for i in range(3)]
        activated = int(on.get("post_ack_diversified_contingencies", "0")) > 0
        activation_cases += int(activated)
        if tier:
            differing_with_activation += int(activated)
            differing_without_activation += int(not activated)
        if tier:
            tails.append({
                "seed": int(off["seed"]),
                "suite": off["suite"],
                "tier": tier,
                "delta": delta,
                "activated": activated,
                "diversified_contingencies": int(
                    on.get("post_ack_diversified_contingencies", "0")
                ),
                "completed_proof_delta": int(
                    on.get("post_ack_completed_proofs", "0")
                ) - int(off.get("post_ack_completed_proofs", "0")),
                "strong_proof_record_delta": int(
                    on.get("post_ack_strong_proof_records", "0")
                ) - int(off.get("post_ack_strong_proof_records", "0")),
            })
        for field in proof_fields:
            before = int(off.get(field, "0"))
            after = int(on.get(field, "0"))
            proof_delta = after - before
            proof_coverage[field]["delta"] += proof_delta
            proof_outcome = 0 if proof_delta > 0 else (2 if proof_delta < 0 else 1)
            proof_coverage[field]["gtl"][proof_outcome] += 1
            if proof_delta < 0:
                proof_coverage[field]["losses"].append({
                    "seed": int(off["seed"]),
                    "suite": off["suite"],
                    "delta": proof_delta,
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
    safety_fields = ("invalid", "emergency", "checkpoint_closed_loop_failures", "midday_failure_days", "terminal_sparse_failure")
    telemetry_fields = (
        "post_ack_calls",
        "post_ack_contingencies",
        "post_ack_diversified_contingencies",
        "post_ack_pending_slices",
        "post_ack_proof_calls",
        "post_ack_completed_proofs",
        "post_ack_strong_proof_records",
    )
    result = {
        "pairs": len(off_rows),
        "wtl": [wins, ties, losses],
        "score_delta": deltas,
        "first_difference_tiers": dict(sorted(first_tiers.items())),
        "safety": {
            side: {field: sum(int(row.get(field, "0")) for row in rows) for field in safety_fields}
            for side, rows in (("off", off_rows), ("on", on_rows))
        },
        "telemetry": {
            side: {field: sum(int(row.get(field, "0")) for row in rows) for field in telemetry_fields}
            for side, rows in (("off", off_rows), ("on", on_rows))
        },
        "activation": {
            "cases": activation_cases,
            "score_differences_with_activation": differing_with_activation,
            "score_differences_without_activation": differing_without_activation,
        },
        "proof_coverage": proof_coverage,
        "strata": {key: value for key, value in sorted(strata.items())},
        "differences": tails,
    }
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    if len(sys.argv) != 3:
        raise SystemExit("usage: summarize_postack_contingency_229.py OFF_LOG ON_LOG")
    summarize(sys.argv[1], sys.argv[2])
