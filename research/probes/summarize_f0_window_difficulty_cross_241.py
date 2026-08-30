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


def read_result(path: Path) -> tuple[dict[str, str], dict[tuple[int, str], str]]:
    result = None
    day_hashes: dict[tuple[int, str], str] = {}
    for line in path.read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("result,"):
            if result is not None:
                raise RuntimeError(f"duplicate result in {path}")
            result = fields(line)
        elif line.startswith("day_detail,"):
            row = fields(line)
            key = int(row["day"]), row["version"]
            if key in day_hashes:
                raise RuntimeError(f"duplicate day detail {key} in {path}")
            day_hashes[key] = row["plan_hash"]
    if result is None:
        raise RuntimeError(f"missing result in {path}")
    return result, day_hashes


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return int(row["lifetime"]), int(row["daily"]), int(row["servings"])


def first_tier(delta: tuple[int, int, int]) -> int:
    for index, value in enumerate(delta, 1):
        if value:
            return index
    return 0


def fuel(row: dict[str, str]) -> str:
    family = row.get("family", "")
    if family.startswith("low-fuel-"):
        return "low"
    if family.startswith("high-fuel-"):
        return "high"
    return "default"


def outcome(parent: dict[str, str], candidate: dict[str, str]) -> tuple[str, tuple[int, int, int]]:
    pscore = score(parent)
    cscore = score(candidate)
    delta = tuple(c - p for c, p in zip(cscore, pscore))
    label = "win" if cscore > pscore else "loss" if cscore < pscore else "tie"
    return label, delta


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--result-dir", required=True)
    args = parser.parse_args()

    rows = [
        row
        for row in csv.DictReader(Path(args.manifest).open(encoding="utf-8-sig"))
        if row["split"] == "development"
    ]
    result_dir = Path(args.result_dir)
    expected_pairs = sum(int(row["count"]) for row in rows)
    pairs = []
    by_fixture: dict[tuple[str, int, str], dict[int, tuple[str, tuple[int, int, int]]]] = defaultdict(dict)

    for row in rows:
        window = int(row["public_window_ms"])
        for offset in range(int(row["count"])):
            seed = int(row["first_seed"]) + offset
            parent_path = result_dir / f"parent-{row['suite']}-w{window}-{seed}.result"
            candidate_path = result_dir / f"candidate-{row['suite']}-w{window}-{seed}.result"
            parent, parent_days = read_result(parent_path)
            candidate, candidate_days = read_result(candidate_path)
            label, delta = outcome(parent, candidate)
            order = "parent-first" if (seed + int(window > 5000)) % 2 == 0 else "candidate-first"
            equal_days = sum(
                parent_days[key] == candidate_days.get(key)
                for key in parent_days
                if key[1] == "parent" and (key[0], "signature") in candidate_days
            )
            comparable_days = sum(
                (key[0], "signature") in candidate_days
                for key in parent_days
                if key[1] == "parent"
            )
            pairs.append({
                "suite": row["suite"],
                "seed": seed,
                "role": row["role_mode"],
                "window": window,
                "order": order,
                "fuel": fuel(candidate),
                "parent": parent,
                "candidate": candidate,
                "outcome": label,
                "delta": delta,
                "equal_days": equal_days,
                "comparable_days": comparable_days,
            })
            by_fixture[(row["suite"], seed, row["role_mode"])][window] = (label, delta)

    if len(pairs) != expected_pairs:
        raise RuntimeError(f"paired {len(pairs)} != expected {expected_pairs}")
    if any(set(windows) != {5000, 15000} for windows in by_fixture.values()):
        raise RuntimeError("crossed fixture is missing one public window")

    group_counts: dict[str, Counter[str]] = defaultdict(Counter)
    group_delta: dict[str, list[int]] = defaultdict(lambda: [0, 0, 0])
    tiers = Counter()
    gains = []
    losses = []
    for pair in pairs:
        keys = (
            "global",
            f"window:{pair['window']}",
            f"suite:{pair['suite']}|window:{pair['window']}",
            f"role:{pair['role']}|window:{pair['window']}",
            f"fuel:{pair['fuel']}|window:{pair['window']}",
            f"order:{pair['order']}|window:{pair['window']}",
        )
        for key in keys:
            group_counts[key][pair["outcome"]] += 1
            for tier, value in enumerate(pair["delta"]):
                group_delta[key][tier] += value
        tier = first_tier(pair["delta"])
        tiers[tier] += 1
        if pair["outcome"] == "win":
            gains.append(pair["delta"][tier - 1])
        elif pair["outcome"] == "loss":
            losses.append(pair["delta"][tier - 1])

    safety_keys = sorted({
        key
        for pair in pairs
        for side in (pair["parent"], pair["candidate"])
        for key in side
        if key in {"invalid", "emergency"}
        or "failure" in key
        or key.endswith("_invalid")
    })
    safety = {
        label: {
            key: sum(int(pair[label].get(key, "0")) for pair in pairs)
            for key in safety_keys
        }
        for label in ("parent", "candidate")
    }
    cross = Counter()
    for windows in by_fixture.values():
        cross[f"5000:{windows[5000][0]}|15000:{windows[15000][0]}"] += 1

    summary = {
        "experiment": "ATTR-F0-WINDOW-DIFFICULTY-CROSS-241",
        "pairs": len(pairs),
        "unique_fixtures": len(by_fixture),
        "wtl": [
            group_counts["global"]["win"],
            group_counts["global"]["tie"],
            group_counts["global"]["loss"],
        ],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": group_delta["global"],
        "gain_tail": {"sum": sum(gains), "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "groups": {
            key: {
                "wtl": [value["win"], value["tie"], value["loss"]],
                "aggregate_score_delta": group_delta[key],
            }
            for key, value in sorted(group_counts.items())
        },
        "crossed_outcomes": dict(sorted(cross.items())),
        "safety": safety,
        "plan_hash_days": {
            "equal": sum(pair["equal_days"] for pair in pairs),
            "total": sum(pair["comparable_days"] for pair in pairs),
        },
    }
    print(json.dumps(summary, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
