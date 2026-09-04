#!/usr/bin/env python3
import argparse
import hashlib
import json
import re
from pathlib import Path


SUMMARY = re.compile(
    r"^summary role_mask=(?P<mask>\d+) days=(?P<days>\d+) "
    r"score=(?P<life>\d+)/(?P<daily>\d+)/(?P<servings>\d+)$"
)
BEGIN = re.compile(r"^case_begin order=(?P<order>forward|reverse) mask=(?P<mask>\d+) ")
COMPLETE = re.compile(
    r"^case_complete order=(?P<order>forward|reverse) mask=(?P<mask>\d+) exit=0$"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def compare(left: tuple[int, int, int], right: tuple[int, int, int]) -> int:
    return (left > right) - (left < right)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    lines = args.log.read_text(encoding="utf-8").splitlines()
    current: tuple[str, int] | None = None
    scores: dict[tuple[str, int], tuple[int, int, int]] = {}
    complete: set[tuple[str, int]] = set()
    failures: list[str] = []
    run_complete = 0

    for line in lines:
        if match := BEGIN.match(line):
            current = (match.group("order"), int(match.group("mask")))
        elif match := SUMMARY.match(line):
            if current is None or current[1] != int(match.group("mask")):
                failures.append(f"orphan summary: {line}")
                continue
            scores[current] = tuple(int(match.group(name)) for name in ("life", "daily", "servings"))
        elif match := COMPLETE.match(line):
            key = (match.group("order"), int(match.group("mask")))
            complete.add(key)
            if key not in scores:
                failures.append(f"complete without summary: {key}")
            current = None
        elif line == "run_complete cases=12":
            run_complete += 1

    expected = {(order, mask) for order in ("forward", "reverse") for mask in (0, 1, 2, 4, 8, 16)}
    if complete != expected:
        failures.append(f"completion mismatch missing={sorted(expected-complete)} extra={sorted(complete-expected)}")
    if set(scores) != expected:
        failures.append(f"score mismatch missing={sorted(expected-set(scores))} extra={sorted(set(scores)-expected)}")
    if run_complete != 1:
        failures.append(f"run_complete count={run_complete}")

    result: dict[str, object] = {
        "log": str(args.log),
        "log_sha256": sha256(args.log),
        "complete_cases": len(complete),
        "run_complete": run_complete,
        "failures": failures,
    }
    if not failures:
        parent = {order: scores[(order, 0)] for order in ("forward", "reverse")}
        masks: dict[str, object] = {}
        stable_tier2_wins = 0
        for mask in (1, 2, 4, 8, 16):
            values = {order: scores[(order, mask)] for order in ("forward", "reverse")}
            comparisons = {order: compare(values[order], parent[order]) for order in values}
            tier2_win = all(
                values[order][0] >= parent[order][0] and values[order][1] > parent[order][1]
                for order in values
            )
            if tier2_win:
                stable_tier2_wins += 1
            masks[str(mask)] = {
                "scores": {order: list(value) for order, value in values.items()},
                "comparisons": comparisons,
                "stable_tier2_win": tier2_win,
            }

        median_by_order: dict[str, list[int]] = {}
        median_tier2_better = True
        for order in ("forward", "reverse"):
            ordered = sorted(scores[(order, mask)] for mask in (1, 2, 4, 8, 16))
            median = ordered[len(ordered) // 2]
            median_by_order[order] = list(median)
            median_tier2_better &= median[0] >= parent[order][0] and median[1] > parent[order][1]

        gate_pass = stable_tier2_wins >= 3 and median_tier2_better
        result.update(
            {
                "all_patrol": {order: list(value) for order, value in parent.items()},
                "one_tanker": masks,
                "stable_tier2_wins": stable_tier2_wins,
                "median_one_tanker": median_by_order,
                "median_tier2_better": median_tier2_better,
                "gate_pass": gate_pass,
                "verdict": "positive-attribution" if gate_pass else "closed-negative",
            }
        )

    args.output.write_text(json.dumps(result, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2, sort_keys=True))
    if failures:
        raise SystemExit(2)


if __name__ == "__main__":
    main()
