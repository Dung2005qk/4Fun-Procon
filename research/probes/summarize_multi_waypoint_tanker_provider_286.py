#!/usr/bin/env python3
import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path


def fields(line: str) -> dict[str, str]:
    parsed: dict[str, str] = {}
    for token in line.rstrip("\n").split(",")[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            parsed[key] = value
    return parsed


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return int(row["lifetime"]), int(row["daily"]), int(row["servings"])


def delta(candidate: tuple[int, int, int], parent: tuple[int, int, int]) -> tuple[int, int, int]:
    return tuple(right - left for left, right in zip(parent, candidate))


def first_difference(value: tuple[int, int, int]) -> int:
    for index, component in enumerate(value, 1):
        if component:
            return index
    return 0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--phase", choices=("development", "holdout"), required=True)
    parser.add_argument("--output")
    args = parser.parse_args()

    manifest_rows = [
        row
        for row in csv.DictReader(Path(args.manifest).open(encoding="utf-8-sig"))
        if row["split"] == args.phase
    ]
    metadata: dict[tuple[str, int], dict[str, str]] = {}
    for row in manifest_rows:
        for offset in range(int(row["count"])):
            key = row["suite"], int(row["first_seed"]) + offset
            if key in metadata:
                raise RuntimeError(f"duplicate manifest case {key}")
            metadata[key] = row

    results: dict[tuple[str, int], dict[str, dict[str, str]]] = defaultdict(dict)
    day_rows: dict[tuple[str, int, str], list[dict[str, str]]] = defaultdict(list)
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
            if version not in {"parent-286", "candidate-286"}:
                raise RuntimeError(f"unexpected version {version}")
            key = row["suite"], int(row["seed"])
            if active != key or version in results[key]:
                raise RuntimeError(f"invalid or duplicate result {key}/{version}, active={active}")
            results[key][version] = row
        elif line.startswith("day_detail,"):
            row = fields(line)
            version = row["version"]
            if active is None or int(row["seed"]) != active[1]:
                raise RuntimeError(f"day detail outside matching case: active={active}")
            day_rows[(active[0], active[1], version)].append(row)
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active != key:
                raise RuntimeError(f"completion mismatch {key}, active={active}")
            completed.add(key)
            active = None
        elif line.startswith("run_complete,"):
            run_complete = True

    if active is not None or not run_complete:
        raise RuntimeError("incomplete atomic run")
    if set(results) != completed or set(results) != set(metadata):
        raise RuntimeError("manifest/result/case_complete bijection failed")
    if any(set(arms) != {"parent-286", "candidate-286"} for arms in results.values()):
        raise RuntimeError("a completed case lacks exactly one parent and candidate result")

    required = {
        "lifetime",
        "daily",
        "servings",
        "invalid",
        "emergency",
        "role_mask",
        "max_ms",
        "multi_waypoint_provider_columns",
        "multi_waypoint_consumer_columns",
        "multi_waypoint_selected_providers",
        "multi_waypoint_selected_consumers",
    }
    records = []
    for key in sorted(metadata):
        parent = results[key]["parent-286"]
        candidate = results[key]["candidate-286"]
        for arm_name, row in (("parent", parent), ("candidate", candidate)):
            missing = sorted(required - set(row))
            if missing:
                raise RuntimeError(f"missing fields for {key}/{arm_name}: {missing}")
            days = day_rows[(key[0], key[1], f"{arm_name}-286")]
            if not 4 <= len(days) <= 10:
                raise RuntimeError(f"invalid day count for {key}/{arm_name}: {len(days)}")
            if [int(day["day"]) for day in days] != list(range(1, len(days) + 1)):
                raise RuntimeError(f"invalid day sequence for {key}/{arm_name}")
        parent_score = score(parent)
        candidate_score = score(candidate)
        paired_delta = delta(candidate_score, parent_score)
        outcome = "win" if candidate_score > parent_score else "loss" if candidate_score < parent_score else "tie"
        records.append(
            {
                "key": key,
                "meta": metadata[key],
                "parent": parent,
                "candidate": candidate,
                "parent_score": parent_score,
                "candidate_score": candidate_score,
                "delta": paired_delta,
                "first_tier": first_difference(paired_delta),
                "outcome": outcome,
            }
        )

    outcomes = Counter(record["outcome"] for record in records)
    first_tiers = Counter(record["first_tier"] for record in records)
    gains = [
        record["delta"][record["first_tier"] - 1]
        for record in records
        if record["outcome"] == "win"
    ]
    losses = [
        record["delta"][record["first_tier"] - 1]
        for record in records
        if record["outcome"] == "loss"
    ]
    aggregate = [sum(record["delta"][index] for record in records) for index in range(3)]

    strata: dict[str, Counter[str]] = defaultdict(Counter)
    winning_values: dict[str, set[str]] = defaultdict(set)
    for record in records:
        meta = record["meta"]
        values = {
            "suite": record["key"][0],
            "players": meta["players"],
            "fuel": meta["fuel_profile"],
            "role_mask": meta["role_mask"],
        }
        for name, value in values.items():
            strata[f"{name}:{value}"][record["outcome"]] += 1
            if record["outcome"] == "win":
                winning_values[name].add(value)

    def arm_sum(arm: str, key: str) -> int:
        return sum(int(record[arm].get(key, "0")) for record in records)

    safety_fields = ("invalid", "emergency")
    zero_safety = all(
        arm_sum(arm, key) == 0
        for arm in ("parent", "candidate")
        for key in safety_fields
    )
    parent_isolated = all(
        int(record["parent"][key]) == 0
        for record in records
        for key in (
            "multi_waypoint_provider_columns",
            "multi_waypoint_consumer_columns",
            "multi_waypoint_selected_providers",
            "multi_waypoint_selected_consumers",
        )
    )
    no_upper_tier_loss = all(
        record["outcome"] != "loss" or record["first_tier"] == 3
        for record in records
    )
    worst_loss = min(losses, default=0)
    total_gain = sum(gains)
    total_loss = -sum(losses)
    minimum_wins = 4 if args.phase == "development" else 6
    gate = {
        "parent_flag_is_operation_isolated": parent_isolated,
        "candidate_generated_multi_waypoint_supply":
            arm_sum("candidate", "multi_waypoint_provider_columns") > 0
            and arm_sum("candidate", "multi_waypoint_consumer_columns") > 0,
        "candidate_selected_multi_waypoint_supply":
            arm_sum("candidate", "multi_waypoint_selected_providers") > 0
            and arm_sum("candidate", "multi_waypoint_selected_consumers") > 0,
        "zero_invalid_or_emergency": zero_safety,
        "canonical_checkpoint_bounded":
            max(int(record["candidate"]["max_ms"]) for record in records) <= 5000,
        "no_tier1_or_tier2_loss": no_upper_tier_loss,
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
        "experiment": "SCORE-MULTI-WAYPOINT-TANKER-PROVIDER-286",
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
        },
        "safety": {
            f"{arm}_{key}": arm_sum(arm, key)
            for arm in ("parent", "candidate")
            for key in safety_fields
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
