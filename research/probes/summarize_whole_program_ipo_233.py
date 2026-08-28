#!/usr/bin/env python3
import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path


def parse_fields(line: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for token in line.rstrip("\n").split(",")[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            fields[key] = value
    return fields


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return (int(row["lifetime"]), int(row["daily"]), int(row["servings"]))


def fuel_regime(row: dict[str, str]) -> str:
    family = row.get("family", "")
    if family.startswith("low-fuel-"):
        return "low"
    if family.startswith("high-fuel-"):
        return "high"
    return "default"


def first_tier(delta: tuple[int, int, int]) -> int:
    for index, value in enumerate(delta, 1):
        if value != 0:
            return index
    return 0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--phase", choices=("development", "holdout"), required=True)
    args = parser.parse_args()

    manifest_rows = [
        row for row in csv.DictReader(Path(args.manifest).open(encoding="utf-8-sig"))
        if row["split"] == args.phase
    ]
    expected = sum(int(row["count"]) for row in manifest_rows)
    windows: dict[tuple[str, int], int] = {}
    for row in manifest_rows:
        for offset in range(int(row["count"])):
            windows[(row["suite"], int(row["first_seed"]) + offset)] = int(row["public_window_ms"])

    results: dict[tuple[str, int, str], dict[str, str]] = {}
    day_hashes: dict[tuple[int, int, str], str] = {}
    for raw in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if raw.startswith("result,"):
            row = parse_fields(raw)
            key = (row["suite"], int(row["seed"]), row["version"])
            if key in results:
                raise RuntimeError(f"duplicate result {key}")
            results[key] = row
        elif raw.startswith("day_detail,"):
            row = parse_fields(raw)
            key = (int(row["seed"]), int(row["day"]), row["version"])
            if key in day_hashes:
                raise RuntimeError(f"duplicate day detail {key}")
            day_hashes[key] = row["plan_hash"]

    pairs = []
    for suite, seed in sorted(windows):
        parent = results.get((suite, seed, "parent"))
        candidate = results.get((suite, seed, "ipo"))
        if parent is None or candidate is None:
            raise RuntimeError(f"missing pair {(suite, seed)}")
        pscore = score(parent)
        cscore = score(candidate)
        delta = tuple(c - p for c, p in zip(cscore, pscore))
        outcome = "win" if cscore > pscore else "loss" if cscore < pscore else "tie"
        pairs.append((suite, seed, parent, candidate, delta, outcome))

    if len(pairs) != expected:
        raise RuntimeError(f"paired {len(pairs)} != expected {expected}")

    outcomes = Counter(pair[5] for pair in pairs)
    first_differences = Counter(first_tier(pair[4]) for pair in pairs)
    score_delta = [sum(pair[4][tier] for pair in pairs) for tier in range(3)]
    gains = [pair[4][first_tier(pair[4]) - 1] for pair in pairs if pair[5] == "win"]
    losses = [pair[4][first_tier(pair[4]) - 1] for pair in pairs if pair[5] == "loss"]

    strata: dict[str, Counter[str]] = defaultdict(Counter)
    for suite, seed, parent, candidate, delta, outcome in pairs:
        strata[f"suite:{suite}"][outcome] += 1
        strata[f"fuel:{fuel_regime(candidate)}"][outcome] += 1
        strata[f"role:{candidate['role_mode']}"][outcome] += 1
        strata[f"window:{windows[(suite, seed)]}"][outcome] += 1

    safety_keys = sorted({
        key for _, _, parent, candidate, _, _ in pairs
        for key in set(parent) | set(candidate)
        if key in {"invalid", "emergency"} or "failure" in key or key.endswith("_invalid")
    })
    safety = {
        label: {
            key: sum(int(pair[2 if label == "parent" else 3].get(key, "0")) for pair in pairs)
            for key in safety_keys
        }
        for label in ("parent", "ipo")
    }

    total_days = 0
    equal_days = 0
    for _, seed, parent, candidate, _, _ in pairs:
        days = max(
            int(parent.get("days", "0")),
            int(candidate.get("days", "0")),
            max((day for s, day, _ in day_hashes if s == seed), default=0),
        )
        for day in range(1, days + 1):
            p = day_hashes.get((seed, day, "parent"))
            c = day_hashes.get((seed, day, "ipo"))
            if p is not None and c is not None:
                total_days += 1
                equal_days += int(p == c)

    def summed(label: str, field: str) -> int:
        index = 2 if label == "parent" else 3
        return sum(int(pair[index].get(field, "0")) for pair in pairs)

    summary = {
        "experiment": "PERF-WHOLE-PROGRAM-IPO-233",
        "phase": args.phase,
        "pairs": len(pairs),
        "wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(first_differences.items())),
        "aggregate_score_delta": score_delta,
        "gain_tail": {"sum": sum(gains), "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "safety": safety,
        "plan_hash_days": {"equal": equal_days, "total": total_days},
        "work": {
            "parent_combinations": summed("parent", "combinations"),
            "ipo_combinations": summed("ipo", "combinations"),
            "parent_mean_ms_sum": summed("parent", "mean_ms"),
            "ipo_mean_ms_sum": summed("ipo", "mean_ms"),
            "parent_max_ms": max(int(pair[2].get("max_ms", "0")) for pair in pairs),
            "ipo_max_ms": max(int(pair[3].get("max_ms", "0")) for pair in pairs),
        },
    }
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
