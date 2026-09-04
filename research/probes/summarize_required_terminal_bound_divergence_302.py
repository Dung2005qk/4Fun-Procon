#!/usr/bin/env python3
"""Read-only causal-boundary audit for rejected experiment 301."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for chunk in iter(lambda: stream.read(1 << 20), b""):
            digest.update(chunk)
    return digest.hexdigest().upper()


def fields(line: str) -> dict[str, str]:
    return dict(item.split("=", 1) for item in line.rstrip().split(",")[1:])


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return (int(row["lifetime"]), int(row["daily"]), int(row["servings"]))


def cumulative(row: dict[str, str]) -> tuple[int, int, int]:
    return tuple(int(value) for value in row["cumulative"].split("/"))  # type: ignore[return-value]


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inputs", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    root = Path.cwd()
    input_rows = list(csv.DictReader(args.inputs.open(encoding="utf-8-sig")))
    if len(input_rows) != 6:
        raise ValueError(f"expected six frozen inputs, got {len(input_rows)}")
    resolved: dict[str, Path] = {}
    for row in input_rows:
        if row["experiment_id"] != "ATTR-REQUIRED-TERMINAL-BOUND-DIVERGENCE-302":
            raise ValueError("wrong attribution experiment")
        path = root / row["input_path"]
        actual = sha256(path)
        if actual != row["input_sha256"]:
            raise ValueError(f"hash mismatch for {path}: {actual}")
        resolved[row["input_kind"]] = path

    cases: dict[tuple[str, int], dict[str, object]] = {}
    current: tuple[str, int] | None = None
    run_complete = 0
    result_count = 0
    case_complete = 0
    seed_to_key: dict[int, tuple[str, int]] = {}
    for line in resolved["development-log"].read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("case_begin,"):
            row = fields(line)
            current = (row["suite"], int(row["seed"]))
            if current in cases:
                raise ValueError(f"duplicate case {current}")
            cases[current] = {"meta": row, "results": {}, "days": defaultdict(list)}
            seed_to_key[int(row["seed"])] = current
        elif line.startswith("result,"):
            row = fields(line)
            key = (row["suite"], int(row["seed"]))
            arm = row["version"].split("-", 1)[0]
            cases[key]["results"][arm] = row  # type: ignore[index]
            result_count += 1
        elif line.startswith("day_detail,"):
            row = fields(line)
            key = seed_to_key[int(row["seed"])]
            arm = row["version"].split("-", 1)[0]
            cases[key]["days"][arm].append(row)  # type: ignore[index]
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = (row["suite"], int(row["seed"]))
            if current != key:
                raise ValueError(f"non-atomic completion {key}, active {current}")
            if set(cases[key]["results"]) != {"parent", "candidate"}:  # type: ignore[arg-type]
                raise ValueError(f"incomplete pair {key}")
            case_complete += 1
            current = None
        elif line.startswith("run_complete,"):
            run_complete += 1

    if current is not None or len(cases) != 30 or result_count != 60 or case_complete != 30 or run_complete != 1:
        raise ValueError(
            f"bad structure cases={len(cases)} results={result_count} "
            f"case_complete={case_complete} run_complete={run_complete} active={current}"
        )

    order_outcomes: dict[str, Counter[str]] = defaultdict(Counter)
    role_outcomes: dict[str, Counter[str]] = defaultdict(Counter)
    classifications: Counter[str] = Counter()
    rows: list[dict[str, object]] = []
    mismatch_5000: list[str] = []
    loss_primary: Counter[str] = Counter()

    for key in sorted(cases):
        item = cases[key]
        meta: dict[str, str] = item["meta"]  # type: ignore[assignment]
        results: dict[str, dict[str, str]] = item["results"]  # type: ignore[assignment]
        days: dict[str, list[dict[str, str]]] = item["days"]  # type: ignore[assignment]
        parent = results["parent"]
        candidate = results["candidate"]
        parent_score = score(parent)
        candidate_score = score(candidate)
        outcome = "win" if candidate_score > parent_score else "loss" if candidate_score < parent_score else "tie"
        order = meta["order"]
        order_outcomes[order][outcome] += 1
        role_equal = parent["role_mask"] == candidate["role_mask"]
        role_outcomes["same" if role_equal else "different"][outcome] += 1

        parent_days = sorted(days["parent"], key=lambda row: int(row["day"]))
        candidate_days = sorted(days["candidate"], key=lambda row: int(row["day"]))
        if len(parent_days) != len(candidate_days):
            raise ValueError(f"day count mismatch {key}")
        earliest_plan = None
        earliest_score = None
        for parent_day, candidate_day in zip(parent_days, candidate_days, strict=True):
            day = int(parent_day["day"])
            if int(candidate_day["day"]) != day:
                raise ValueError(f"day identity mismatch {key}")
            if earliest_plan is None and parent_day["plan_hash"] != candidate_day["plan_hash"]:
                earliest_plan = day
            if earliest_score is None and cumulative(parent_day) != cumulative(candidate_day):
                earliest_score = day

        route_delta = int(candidate["midday_target_routes"]) - int(parent["midday_target_routes"])
        plan_delta = int(candidate["midday_target_plans"]) - int(parent["midday_target_plans"])
        valid_delta = int(candidate["midday_target_valid"]) - int(parent["midday_target_valid"])
        acceptance_delta = int(candidate["midday_target_acceptances"]) - int(parent["midday_target_acceptances"])
        deadline_delta = int(candidate["midday_deadline_days"]) - int(parent["midday_deadline_days"])

        tags: list[str] = []
        if not role_equal:
            tags.append("upstream-role-divergence")
        if acceptance_delta > 0:
            tags.append("extra-target-acceptance")
        elif acceptance_delta < 0:
            tags.append("fewer-target-acceptances")
        if route_delta < 0 or valid_delta < 0:
            tags.append("target-supply-reduction")
        elif route_delta > 0 or valid_delta > 0:
            tags.append("target-supply-expansion")
        if deadline_delta > 0:
            tags.append("more-midday-deadline-days")
        elif deadline_delta < 0:
            tags.append("fewer-midday-deadline-days")
        if not tags:
            tags.append("no-aggregate-target-counter-difference")
        classifications.update(tags)

        primary = "not-loss"
        if outcome == "loss":
            if not role_equal:
                primary = "upstream-role-divergence"
            elif acceptance_delta > 0:
                primary = "extra-locally-greedy-target-acceptance"
            elif acceptance_delta < 0 or route_delta < 0 or valid_delta < 0:
                primary = "target-candidate-starvation"
            else:
                primary = "unresolved-same-role-target-path"
            loss_primary[primary] += 1

        plan_equivalent = earliest_plan is None
        score_equivalent = parent_score == candidate_score
        if int(meta["public_window_ms"]) == 5000 and (not plan_equivalent or not score_equivalent):
            mismatch_5000.append(f"{key[0]}:{key[1]}")

        rows.append(
            {
                "suite": key[0],
                "seed": key[1],
                "window": int(meta["public_window_ms"]),
                "order": order,
                "outcome": outcome,
                "score_delta": [candidate_score[index] - parent_score[index] for index in range(3)],
                "role_masks": [int(parent["role_mask"]), int(candidate["role_mask"])],
                "role_equal": role_equal,
                "earliest_plan_divergence_day": earliest_plan,
                "earliest_cumulative_score_divergence_day": earliest_score,
                "midday_target_delta": {
                    "routes": route_delta,
                    "plans": plan_delta,
                    "valid": valid_delta,
                    "acceptances": acceptance_delta,
                    "deadline_days": deadline_delta,
                },
                "tags": tags,
                "loss_primary_class": primary,
            }
        )

    summary301 = json.loads(resolved["development-summary"].read_text(encoding="utf-8"))
    report = {
        "experiment": "ATTR-REQUIRED-TERMINAL-BOUND-DIVERGENCE-302",
        "input_manifest_sha256": sha256(args.inputs),
        "source_log_sha256": sha256(resolved["development-log"]),
        "source_summary_sha256": sha256(resolved["development-summary"]),
        "source_paired_wtl": summary301["paired_wtl"],
        "source_longer_window_wtl": summary301["longer_window_wtl"],
        "order_outcomes": {key: dict(value) for key, value in sorted(order_outcomes.items())},
        "role_mask_outcomes": {key: dict(value) for key, value in sorted(role_outcomes.items())},
        "five_thousand_mismatches": mismatch_5000,
        "loss_primary_classes": dict(loss_primary),
        "all_tags": dict(classifications),
        "single_loss_mechanism": len(loss_primary) == 1,
        "order_identical_5000_prefix": not mismatch_5000,
        "score_successor_authorized_by_attribution": len(loss_primary) == 1 and not mismatch_5000,
        "cases": rows,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in (
        "order_outcomes",
        "role_mask_outcomes",
        "five_thousand_mismatches",
        "loss_primary_classes",
        "single_loss_mechanism",
        "order_identical_5000_prefix",
        "score_successor_authorized_by_attribution",
    )}, sort_keys=True))


if __name__ == "__main__":
    main()
