#!/usr/bin/env python3
"""Aggregate the frozen development/holdout logs for experiment 219."""

from __future__ import annotations

import argparse
import collections
import json
from pathlib import Path


def fields(line: str) -> dict[str, str]:
    parts = line.rstrip().split(",")
    return {
        key: value
        for part in parts[1:]
        if "=" in part
        for key, value in [part.split("=", 1)]
    }


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return (int(row["lifetime"]), int(row["daily"]), int(row["servings"]))


def slash_score(value: str) -> tuple[int, int, int]:
    parsed = tuple(int(part) for part in value.split("/"))
    if len(parsed) != 3:
        raise ValueError(f"invalid slash score: {value}")
    return parsed


def score_delta(
    parent: tuple[int, int, int],
    candidate: tuple[int, int, int],
) -> tuple[int, int, int]:
    return tuple(right - left for left, right in zip(parent, candidate))


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("logdir", type=Path)
    parser.add_argument(
        "--stage",
        choices=("development", "holdout"),
        default="development",
    )
    args = parser.parse_args()

    controls: dict[int, dict[str, object]] = {}
    public_rows: list[dict[str, str]] = []
    run_complete: list[str] = []
    current_case: dict[str, str] = {}
    current_result_key: int | None = None

    for path in sorted(args.logdir.glob(
        "DEADLINE-CHECKPOINT-CLOSED-LOOP-219-*.log"
    )):
        phase = "control" if "-control-" in path.name else "public15"
        for raw in path.read_text(encoding="utf-8", errors="strict").splitlines():
            if raw.startswith("case_begin,"):
                current_case = fields(raw)
                current_result_key = None
            elif raw.startswith("result,"):
                row = fields(raw)
                row.update({f"case_{key}": value for key, value in current_case.items()})
                if phase == "control":
                    key = int(row["seed"])
                    controls[key] = {"result": row, "hashes": []}
                    current_result_key = key
                else:
                    public_rows.append(row)
            elif raw.startswith("day_detail,") and current_result_key is not None:
                controls[current_result_key]["hashes"].append(
                    int(fields(raw)["plan_hash"])
                )
            elif raw.startswith("run_complete,"):
                run_complete.append(f"{path.name}:{raw}")

    control_seeds = sorted(controls)
    control_mismatches: list[dict[str, object]] = []
    for seed in control_seeds:
        payload = controls[seed]
        row = payload["result"]
        candidate = score(row)
        parent = slash_score(row["checkpoint_closed_loop_parent"])
        if (
            candidate != parent
            or int(row.get("public_window_probe_days", "0")) != 0
            or int(row.get("checkpoint_closed_loop_takeovers", "0")) != 0
            or int(row.get("checkpoint_closed_loop_failures", "0")) != 0
        ):
            control_mismatches.append(
                {
                    "seed": seed,
                    "candidate_score": candidate,
                    "parent_score": parent,
                    "public_window_probe_days": int(
                        row.get("public_window_probe_days", "0")
                    ),
                    "checkpoint_closed_loop_takeovers": int(
                        row.get("checkpoint_closed_loop_takeovers", "0")
                    ),
                    "checkpoint_closed_loop_failures": int(
                        row.get("checkpoint_closed_loop_failures", "0")
                    ),
                }
            )

    safety_fields = (
        "invalid",
        "emergency",
        "midday_failure_days",
        "terminal_sparse_failure",
        "public_window_probe_failure_days",
        "checkpoint_closed_loop_failures",
    )
    safety_totals = collections.Counter()
    for payload in controls.values():
        row = payload["result"]
        for key in safety_fields:
            safety_totals[key] += int(row.get(key, "0"))
    for row in public_rows:
        for key in safety_fields:
            safety_totals[key] += int(row.get(key, "0"))

    wtl = collections.Counter()
    first_tier = collections.Counter()
    aggregate_delta = [0, 0, 0]
    takeovers = 0
    probe_deadlines = 0
    strata: dict[str, dict[str, collections.Counter[str]]] = {
        axis: collections.defaultdict(collections.Counter)
        for axis in ("tier", "fuel", "role", "family")
    }
    losses: list[dict[str, object]] = []
    for row in public_rows:
        parent = slash_score(row["checkpoint_closed_loop_parent"])
        candidate = score(row)
        delta = score_delta(parent, candidate)
        for index, value in enumerate(delta):
            aggregate_delta[index] += value
        verdict = "win" if candidate > parent else "loss" if candidate < parent else "tie"
        wtl[verdict] += 1
        if verdict != "tie":
            tier = next(index + 1 for index, value in enumerate(delta) if value != 0)
            first_tier[str(tier)] += 1 if verdict == "win" else -1
        if verdict == "loss":
            losses.append(
                {"seed": int(row["seed"]), "parent": parent, "candidate": candidate}
            )
        takeovers += int(row.get("checkpoint_closed_loop_takeovers", "0"))
        probe_deadlines += int(row.get("public_window_probe_deadline_days", "0"))
        values = {
            "tier": row["suite"].removeprefix("stratified-"),
            "fuel": row["fuel_profile"],
            "role": row["role_mode"],
            "family": row["family"],
        }
        for axis, value in values.items():
            strata[axis][value][verdict] += 1
            strata[axis][value]["servings_delta"] += delta[2]

    summary = {
        "stage": args.stage,
        "run_complete_markers": len(run_complete),
        "control_results": len(controls),
        "control_cases": len(control_seeds),
        "control_mismatches": control_mismatches,
        "public_cases": len(public_rows),
        "public_wtl": {
            "wins": wtl["win"],
            "ties": wtl["tie"],
            "losses": wtl["loss"],
        },
        "public_aggregate_delta": aggregate_delta,
        "public_first_tier_balance": dict(first_tier),
        "public_takeovers": takeovers,
        "public_probe_deadline_days": probe_deadlines,
        "losses": losses,
        "safety_totals": dict(safety_totals),
        "strata": {
            axis: {value: dict(counts) for value, counts in groups.items()}
            for axis, groups in strata.items()
        },
    }
    print(json.dumps(summary, indent=2, sort_keys=True))
    complete = (
        len(control_seeds) == 24 and len(public_rows) == 60
        if args.stage == "development"
        else len(control_seeds) == 0 and len(public_rows) == 108
    )
    safe = not control_mismatches and not losses and not any(safety_totals.values())
    return 0 if complete and safe else 1


if __name__ == "__main__":
    raise SystemExit(main())
