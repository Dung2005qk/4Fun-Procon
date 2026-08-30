#!/usr/bin/env python3
import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path


def fields(line: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for token in line.rstrip("\n").split(",")[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            result[key] = value
    return result


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return int(row["lifetime"]), int(row["daily"]), int(row["servings"])


def first_tier(delta: tuple[int, int, int]) -> int:
    for index, value in enumerate(delta, 1):
        if value:
            return index
    return 0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--phase", choices=("development", "holdout"), required=True)
    args = parser.parse_args()

    manifest = [
        row
        for row in csv.DictReader(Path(args.manifest).open(encoding="utf-8-sig"))
        if row["split"] == args.phase
    ]
    expected = sum(int(row["count"]) for row in manifest)
    metadata: dict[tuple[str, int], dict[str, str]] = {}
    for row in manifest:
        for offset in range(int(row["count"])):
            key = row["suite"], int(row["first_seed"]) + offset
            if key in metadata:
                raise RuntimeError(f"duplicate manifest case {key}")
            metadata[key] = row

    cases: dict[tuple[str, int, str], dict[str, str]] = {}
    completed: set[tuple[str, int, str]] = set()
    run_complete = False
    for line in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("result,"):
            row = fields(line)
            key = row["suite"], int(row["seed"]), row["version"]
            if key in cases:
                raise RuntimeError(f"duplicate result {key}")
            cases[key] = row
        elif line.startswith("case_complete,"):
            row = fields(line)
            completed.add((row["suite"], int(row["seed"]), row["label"]))
        elif line.startswith("run_complete,"):
            run_complete = True
    if not run_complete:
        raise RuntimeError("run_complete marker missing")
    if set(cases) != completed:
        raise RuntimeError("result/case_complete bijection failed")

    pairs = []
    for suite, seed in sorted(metadata):
        parent = cases.get((suite, seed, "parent"))
        sparse = cases.get((suite, seed, "sparse"))
        if parent is None or sparse is None:
            raise RuntimeError(f"missing pair {(suite, seed)}")
        pscore, sscore = score(parent), score(sparse)
        delta = tuple(s - p for s, p in zip(sscore, pscore))
        outcome = "win" if sscore > pscore else "loss" if sscore < pscore else "tie"
        pairs.append((suite, seed, parent, sparse, delta, outcome))
    if len(pairs) != expected:
        raise RuntimeError(f"paired {len(pairs)} != expected {expected}")

    outcomes = Counter(pair[5] for pair in pairs)
    tiers = Counter(first_tier(pair[4]) for pair in pairs)
    aggregate = [sum(pair[4][tier] for pair in pairs) for tier in range(3)]
    gains = [pair[4][first_tier(pair[4]) - 1] for pair in pairs if pair[5] == "win"]
    losses = [pair[4][first_tier(pair[4]) - 1] for pair in pairs if pair[5] == "loss"]
    strata: dict[str, Counter[str]] = defaultdict(Counter)
    for suite, seed, _parent, sparse, _delta, outcome in pairs:
        meta = metadata[(suite, seed)]
        for key, value in (
            ("suite", suite),
            ("players", meta["players"]),
            ("fuel", meta["fuel_profile"]),
            ("role", sparse["role_mode"]),
        ):
            strata[f"{key}:{value}"][outcome] += 1

    safety_keys = sorted(
        {
            key
            for _, _, parent, sparse, _, _ in pairs
            for key in set(parent) | set(sparse)
            if key in {"invalid", "emergency"}
            or "failure" in key
            or key.endswith("_invalid")
        }
    )

    def summed(label: str, key: str) -> int:
        index = 2 if label == "parent" else 3
        return sum(int(pair[index].get(key, "0")) for pair in pairs)

    telemetry = {
        "representatives": "sparse_candidate_representatives",
        "settled": "sparse_candidate_settled",
        "routes": "sparse_candidate_routes",
        "valid": "sparse_candidate_valid",
        "strict": "sparse_candidate_strict",
        "supplied": "sparse_candidates_supplied",
        "deadline_days": "sparse_candidate_deadline_days",
    }
    summary = {
        "experiment": "SCORE-SPARSE-ROUTE-CANDIDATE-SUPPLY-248",
        "phase": args.phase,
        "pairs": len(pairs),
        "wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "gain_tail": {"sum": sum(gains), "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "safety": {
            label: {key: summed(label, key) for key in safety_keys}
            for label in ("parent", "sparse")
        },
        "candidate_work": {
            name: summed("sparse", key) for name, key in telemetry.items()
        },
        "response_ms": {
            label: {
                "max": max(
                    int(pair[2 if label == "parent" else 3].get("max_ms", "0"))
                    for pair in pairs
                )
            }
            for label in ("parent", "sparse")
        },
    }
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
