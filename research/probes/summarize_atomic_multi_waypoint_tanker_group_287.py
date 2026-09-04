#!/usr/bin/env python3
import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path


def fields(line: str) -> dict[str, str]:
    parsed: dict[str, str] = {}
    for token in line.split(",")[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            parsed[key] = value
    return parsed


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return tuple(int(row[key]) for key in ("lifetime", "daily", "servings"))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--phase", choices=("development", "holdout"), required=True)
    parser.add_argument("--output")
    args = parser.parse_args()

    manifest = [
        row for row in csv.DictReader(Path(args.manifest).open(encoding="utf-8-sig"))
        if row["split"] == args.phase
    ]
    metadata: dict[tuple[str, int], dict[str, str]] = {}
    for row in manifest:
        for offset in range(int(row["count"])):
            key = row["suite"], int(row["first_seed"]) + offset
            if key in metadata:
                raise RuntimeError(f"duplicate manifest case {key}")
            metadata[key] = row

    results: dict[tuple[str, int], dict[str, dict[str, str]]] = defaultdict(dict)
    days: dict[tuple[str, int, str], list[dict[str, str]]] = defaultdict(list)
    completed: set[tuple[str, int]] = set()
    active: tuple[str, int] | None = None
    run_complete = False
    for line in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("case_begin,"):
            row = fields(line)
            active = row["suite"], int(row["seed"])
        elif line.startswith("result,"):
            row = fields(line)
            version = row["version"]
            if version not in {"parent-287", "candidate-287"}:
                raise RuntimeError(f"unexpected version {version}")
            key = row["suite"], int(row["seed"])
            if active != key or version in results[key]:
                raise RuntimeError(f"invalid or duplicate result {key}/{version}")
            results[key][version] = row
        elif line.startswith("day_detail,"):
            row = fields(line)
            if active is None or int(row["seed"]) != active[1]:
                raise RuntimeError("day detail outside active case")
            days[(active[0], active[1], row["version"])].append(row)
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if key != active:
                raise RuntimeError("case completion mismatch")
            completed.add(key)
            active = None
        elif line.startswith("run_complete,"):
            run_complete = True
    if active is not None or not run_complete or set(results) != completed or set(results) != set(metadata):
        raise RuntimeError("incomplete or non-bijective atomic run")

    required = {
        "lifetime", "daily", "servings", "invalid", "emergency", "role_mask", "max_ms",
        "multi_waypoint_provider_columns", "multi_waypoint_consumer_columns",
        "multi_waypoint_selected_providers", "multi_waypoint_selected_consumers",
        "multi_waypoint_atomicity_failures",
    }
    records = []
    for key in sorted(metadata):
        arms = results[key]
        if set(arms) != {"parent-287", "candidate-287"}:
            raise RuntimeError(f"missing paired arm for {key}")
        parent = arms["parent-287"]
        candidate = arms["candidate-287"]
        for name, row in (("parent", parent), ("candidate", candidate)):
            missing = required - set(row)
            if missing:
                raise RuntimeError(f"missing {sorted(missing)} for {key}/{name}")
            detail = days[(key[0], key[1], f"{name}-287")]
            if not 4 <= len(detail) <= 10 or [int(day["day"]) for day in detail] != list(range(1, len(detail) + 1)):
                raise RuntimeError(f"invalid day sequence for {key}/{name}")
        parent_score = score(parent)
        candidate_score = score(candidate)
        delta = tuple(right - left for left, right in zip(parent_score, candidate_score))
        first_tier = next((index for index, value in enumerate(delta, 1) if value), 0)
        outcome = "win" if candidate_score > parent_score else "loss" if candidate_score < parent_score else "tie"
        records.append({
            "key": key, "meta": metadata[key], "parent": parent, "candidate": candidate,
            "delta": delta, "first_tier": first_tier, "outcome": outcome,
        })

    outcomes = Counter(record["outcome"] for record in records)
    first_tiers = Counter(record["first_tier"] for record in records)
    gains = [record["delta"][record["first_tier"] - 1] for record in records if record["outcome"] == "win"]
    losses = [record["delta"][record["first_tier"] - 1] for record in records if record["outcome"] == "loss"]
    aggregate = [sum(record["delta"][index] for record in records) for index in range(3)]
    winning_values: dict[str, set[str]] = defaultdict(set)
    strata: dict[str, Counter[str]] = defaultdict(Counter)
    for record in records:
        values = {
            "suite": record["key"][0],
            "players": record["meta"]["players"],
            "fuel": record["meta"]["fuel_profile"],
            "role_mask": record["meta"]["role_mask"],
        }
        for name, value in values.items():
            strata[f"{name}:{value}"][record["outcome"]] += 1
            if record["outcome"] == "win":
                winning_values[name].add(value)

    def arm_sum(arm: str, key: str) -> int:
        return sum(int(record[arm].get(key, "0")) for record in records)

    total_gain = sum(gains)
    total_loss = -sum(losses)
    worst_loss = min(losses, default=0)
    minimum_wins = 4 if args.phase == "development" else 6
    parent_isolated = all(
        int(record["parent"][key]) == 0
        for record in records
        for key in (
            "multi_waypoint_provider_columns", "multi_waypoint_consumer_columns",
            "multi_waypoint_selected_providers", "multi_waypoint_selected_consumers",
            "multi_waypoint_atomicity_failures",
        )
    )
    gate = {
        "parent_flag_is_operation_isolated": parent_isolated,
        "candidate_generated_multi_waypoint_supply":
            arm_sum("candidate", "multi_waypoint_provider_columns") > 0
            and arm_sum("candidate", "multi_waypoint_consumer_columns") > 0,
        "candidate_selected_complete_multi_waypoint_groups":
            arm_sum("candidate", "multi_waypoint_selected_providers") > 0
            and arm_sum("candidate", "multi_waypoint_selected_consumers") >=
                2 * arm_sum("candidate", "multi_waypoint_selected_providers"),
        "zero_atomicity_failure": arm_sum("candidate", "multi_waypoint_atomicity_failures") == 0,
        "zero_invalid_or_emergency": all(
            arm_sum(arm, key) == 0
            for arm in ("parent", "candidate")
            for key in ("invalid", "emergency")
        ),
        "canonical_checkpoint_bounded": max(int(record["candidate"]["max_ms"]) for record in records) <= 5000,
        "no_tier1_or_tier2_loss": all(record["outcome"] != "loss" or record["first_tier"] == 3 for record in records),
        "tier3_downside_bounded_to_four": worst_loss >= -4,
        "paired_wins_exceed_losses": outcomes["win"] > outcomes["loss"],
        "minimum_strict_wins": outcomes["win"] >= minimum_wins,
        "gain_mass_at_least_twice_loss_mass": total_gain > 0 and total_gain >= 2 * total_loss,
        "wins_span_two_player_strata": len(winning_values["players"]) >= 2,
        "wins_span_two_map_suites": len(winning_values["suite"]) >= 2,
        "wins_span_two_fuel_profiles": len(winning_values["fuel"]) >= 2,
        "wins_span_two_tanker_identities": len(winning_values["role_mask"]) >= 2,
    }
    gate["passed"] = all(gate.values())
    summary = {
        "experiment": "SCORE-ATOMIC-MULTI-WAYPOINT-TANKER-GROUP-287",
        "phase": args.phase,
        "paired_cases": len(records),
        "paired_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(first_tiers.items())),
        "aggregate_score_delta": aggregate,
        "gain_tail": {"sum": total_gain, "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": worst_loss},
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "candidate_work": {
            "provider_columns": arm_sum("candidate", "multi_waypoint_provider_columns"),
            "consumer_columns": arm_sum("candidate", "multi_waypoint_consumer_columns"),
            "selected_providers": arm_sum("candidate", "multi_waypoint_selected_providers"),
            "selected_consumers": arm_sum("candidate", "multi_waypoint_selected_consumers"),
            "atomicity_failures": arm_sum("candidate", "multi_waypoint_atomicity_failures"),
        },
        "safety": {
            f"{arm}_{key}": arm_sum(arm, key)
            for arm in ("parent", "candidate")
            for key in ("invalid", "emergency")
        },
        "response_ms_local_non_authoritative": {
            "parent_max": max(int(record["parent"]["max_ms"]) for record in records),
            "candidate_max": max(int(record["candidate"]["max_ms"]) for record in records),
        },
        "gate": gate,
    }
    rendered = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
