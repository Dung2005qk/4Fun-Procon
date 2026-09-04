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


def slash_score(value: str) -> tuple[int, int, int]:
    result = tuple(int(part) for part in value.split("/"))
    if len(result) != 3:
        raise RuntimeError(f"invalid score {value!r}")
    return result


def actual_score(row: dict[str, str]) -> tuple[int, int, int]:
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

    cases: dict[tuple[str, int], dict[str, str]] = {}
    days: dict[tuple[str, int], list[dict[str, str]]] = defaultdict(list)
    completed: set[tuple[str, int]] = set()
    active: tuple[str, int] | None = None
    run_complete = False
    for line in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("case_begin,"):
            row = fields(line)
            active = row["suite"], int(row["seed"])
        elif line.startswith("result,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active != key or key in cases or row.get("version") != "causal-276":
                raise RuntimeError(f"invalid result {key}, active={active}")
            cases[key] = row
        elif line.startswith("day_detail,"):
            if active is None:
                raise RuntimeError("day detail outside case")
            row = fields(line)
            if int(row["seed"]) != active[1]:
                raise RuntimeError(f"day detail mismatch for {active}")
            days[active].append(row)
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
    if set(cases) != completed or set(cases) != set(metadata):
        raise RuntimeError("manifest/result/case_complete bijection failed")

    required_result = {
        "lifetime", "daily", "servings", "checkpoint_closed_loop_parent",
        "checkpoint_closed_loop_failures", "protected_sparse_master_routes",
        "protected_sparse_master_combinations", "protected_sparse_master_candidates",
        "protected_sparse_master_valid", "protected_sparse_master_acceptances",
        "protected_sparse_master_deadline_days", "protected_sparse_master_failure_days",
        "invalid", "emergency",
    }
    records = []
    suffix_regressions: list[str] = []
    fallback_mismatches: list[str] = []
    equivalence_5000_failures: list[str] = []
    dense_control_failures: list[str] = []
    for key in sorted(metadata):
        row = cases[key]
        missing = sorted(required_result - set(row))
        if missing:
            raise RuntimeError(f"missing result fields {key}: {missing}")
        actual = actual_score(row)
        parent = slash_score(row["checkpoint_closed_loop_parent"])
        delta = tuple(a - p for a, p in zip(actual, parent))
        outcome = "win" if actual > parent else "loss" if actual < parent else "tie"
        case_days = days[key]
        if not 4 <= len(case_days) <= 10 or [int(day["day"]) for day in case_days] != list(range(1, len(case_days) + 1)):
            raise RuntimeError(f"invalid day sequence {key}")
        accepted_days = 0
        suffix_gain = [0, 0, 0]
        for day in case_days:
            acceptance = int(day.get("sparse_master_acceptances", "0"))
            work = int(day.get("sparse_master_routes", "0")) + int(day.get("sparse_master_candidates", "0"))
            if metadata[key]["public_window_ms"] == "5000":
                if work or acceptance or int(day.get("sparse_master_deadline", "0")) or int(day.get("sparse_master_failure", "0")):
                    equivalence_5000_failures.append(f"{key[0]}:{key[1]}:day{day['day']}:work")
                continue
            sparse_parent = slash_score(day["sparse_master_parent"])
            sparse_refined = slash_score(day["sparse_master_refined"])
            sparse_delta = tuple(r - p for r, p in zip(sparse_refined, sparse_parent))
            if acceptance:
                accepted_days += 1
                if acceptance != 1 or sparse_refined <= sparse_parent:
                    suffix_regressions.append(f"{key[0]}:{key[1]}:day{day['day']}:not-strict")
                if any(component < 0 for component in sparse_delta):
                    suffix_regressions.append(f"{key[0]}:{key[1]}:day{day['day']}:component")
                for tier in range(3):
                    suffix_gain[tier] += sparse_delta[tier]
            elif (sparse_parent != sparse_refined or
                  day.get("sparse_master_parent_plan_hash") != day.get("sparse_master_refined_plan_hash")):
                fallback_mismatches.append(f"{key[0]}:{key[1]}:day{day['day']}")
            if int(day.get("exact_supported", "0")) > 0 and work:
                dense_control_failures.append(f"{key[0]}:{key[1]}:day{day['day']}")
        if metadata[key]["public_window_ms"] == "5000":
            if actual != parent or any(
                day.get("cumulative") != day.get("causal_parent_cumulative") or
                day.get("plan_hash") != day.get("causal_parent_plan_hash")
                for day in case_days
            ):
                equivalence_5000_failures.append(f"{key[0]}:{key[1]}:closed-loop")
        records.append({
            "key": key, "meta": metadata[key], "row": row, "days": case_days,
            "delta": delta, "outcome": outcome, "accepted_days": accepted_days,
            "suffix_gain": suffix_gain,
        })

    outcomes = Counter(record["outcome"] for record in records)
    tiers = Counter(first_tier(record["delta"]) for record in records)
    aggregate = [sum(record["delta"][tier] for record in records) for tier in range(3)]
    takeover_records = [record for record in records if record["accepted_days"] > 0]
    gains = [
        record["delta"][first_tier(record["delta"]) - 1]
        for record in records if record["outcome"] == "win"
    ]
    losses = [
        record["delta"][first_tier(record["delta"]) - 1]
        for record in records if record["outcome"] == "loss"
    ]

    strata: dict[str, Counter[str]] = defaultdict(Counter)
    for record in records:
        for axis, value in (
            ("suite", record["key"][0]), ("players", record["meta"]["players"]),
            ("fuel", record["meta"]["fuel_profile"]), ("role", record["meta"]["role_mode"]),
            ("window", record["meta"]["public_window_ms"]), ("horizon", str(len(record["days"]))),
        ):
            strata[f"{axis}:{value}"][record["outcome"]] += 1

    def summed(name: str) -> int:
        return sum(int(record["row"].get(name, "0")) for record in records)

    safety_keys = sorted({
        name for record in records for name in record["row"]
        if name in {"invalid", "emergency"} or "failure" in name or name.endswith("_invalid")
    })
    zero_safety = all(summed(name) == 0 for name in safety_keys)
    takeover_breadth = {
        "matches": len(takeover_records),
        "days": sum(record["accepted_days"] for record in takeover_records),
        "windows": sorted({record["meta"]["public_window_ms"] for record in takeover_records}),
        "roles": sorted({record["meta"]["role_mode"] for record in takeover_records}),
        "players": sorted({record["meta"]["players"] for record in takeover_records}),
        "suites": sorted({record["key"][0] for record in takeover_records}),
        "fuels": sorted({record["meta"]["fuel_profile"] for record in takeover_records}),
    }
    gate = {
        "complete_5000_internal_equivalence": not equivalence_5000_failures,
        "zero_causal_component_regression": not suffix_regressions,
        "exact_fallback_without_acceptance": not fallback_mismatches,
        "at_least_six_takeover_matches": len(takeover_records) >= 6,
        "takeovers_span_windows_10000_15000": takeover_breadth["windows"] == ["10000", "15000"],
        "takeovers_span_both_roles": takeover_breadth["roles"] == ["deadline", "fixed"],
        "takeovers_span_players_8_9_10": takeover_breadth["players"] == ["10", "8", "9"],
        "takeovers_span_three_map_strata": len(takeover_breadth["suites"]) >= 3,
        "takeovers_span_two_fuels": len(takeover_breadth["fuels"]) >= 2,
        "positive_suffix_gain": any(sum(record["suffix_gain"]) > 0 for record in takeover_records),
        "zero_safety_and_certificate_failures": zero_safety,
        "dense_supported_controls_inert": not dense_control_failures,
    }
    gate["passed"] = all(gate.values())
    summary = {
        "experiment": "SCORE-PROTECTED-SPARSE-MASTER-SUPPLY-276",
        "phase": args.phase,
        "cases": len(records),
        "overall_checkpoint_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "gain_tail": {"sum": sum(gains), "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "strata": {name: dict(value) for name, value in sorted(strata.items())},
        "candidate_work": {
            "routes": summed("protected_sparse_master_routes"),
            "combinations": summed("protected_sparse_master_combinations"),
            "candidates": summed("protected_sparse_master_candidates"),
            "valid": summed("protected_sparse_master_valid"),
            "acceptances": summed("protected_sparse_master_acceptances"),
            "deadline_days": summed("protected_sparse_master_deadline_days"),
            "failure_days": summed("protected_sparse_master_failure_days"),
        },
        "takeover_breadth": takeover_breadth,
        "suffix_gain_components": [sum(record["suffix_gain"][tier] for record in records) for tier in range(3)],
        "suffix_regressions": suffix_regressions,
        "fallback_mismatches": fallback_mismatches,
        "equivalence_5000_failures": equivalence_5000_failures,
        "dense_control_failures": dense_control_failures,
        "safety": {name: summed(name) for name in safety_keys},
        "response_ms_local_non_authoritative": {"max": max(int(record["row"].get("max_ms", "0")) for record in records)},
        "gate": gate,
    }
    rendered = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
