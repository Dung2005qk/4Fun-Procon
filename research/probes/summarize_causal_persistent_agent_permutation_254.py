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


def causal_score(row: dict[str, str]) -> tuple[int, int, int]:
    return (
        int(row["permuted_causal_parent_lifetime"]),
        int(row["permuted_causal_parent_daily"]),
        int(row["permuted_causal_parent_servings"]),
    )


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

    cases: dict[tuple[str, int], dict[str, str]] = {}
    day_rows: dict[tuple[str, int], list[dict[str, str]]] = defaultdict(list)
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
            if active != key:
                raise RuntimeError(f"result outside matching case block: {key}, active={active}")
            if key in cases:
                raise RuntimeError(f"duplicate result {key}")
            cases[key] = row
        elif line.startswith("day_detail,"):
            if active is None:
                raise RuntimeError("day_detail outside case block")
            row = fields(line)
            if int(row["seed"]) != active[1]:
                raise RuntimeError(f"day_detail mismatch for active case {active}")
            day_rows[active].append(row)
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active != key:
                raise RuntimeError(f"completion mismatch: {key}, active={active}")
            completed.add(key)
            active = None
        elif line.startswith("run_complete,"):
            run_complete = True
    if active is not None:
        raise RuntimeError(f"unterminated case block {active}")
    if not run_complete:
        raise RuntimeError("run_complete marker missing")
    if set(cases) != completed or set(cases) != set(metadata):
        raise RuntimeError("manifest/result/case_complete bijection failed")
    if len(cases) != expected:
        raise RuntimeError(f"cases {len(cases)} != expected {expected}")

    records = []
    for key in sorted(metadata):
        row = cases[key]
        parent = causal_score(row)
        actual = score(row)
        delta = tuple(a - p for a, p in zip(actual, parent))
        outcome = "win" if actual > parent else "loss" if actual < parent else "tie"
        records.append(
            {
                "key": key,
                "meta": metadata[key],
                "row": row,
                "days": day_rows[key],
                "parent": parent,
                "actual": actual,
                "delta": delta,
                "outcome": outcome,
            }
        )

    outcomes = Counter(record["outcome"] for record in records)
    tiers = Counter(first_tier(record["delta"]) for record in records)
    aggregate = [sum(record["delta"][tier] for record in records) for tier in range(3)]
    gains = [
        record["delta"][first_tier(record["delta"]) - 1]
        for record in records
        if record["outcome"] == "win"
    ]
    losses = [
        record["delta"][first_tier(record["delta"]) - 1]
        for record in records
        if record["outcome"] == "loss"
    ]

    strata: dict[str, Counter[str]] = defaultdict(Counter)
    for record in records:
        meta = record["meta"]
        for key, value in (
            ("suite", record["key"][0]),
            ("players", meta["players"]),
            ("fuel", meta["fuel_profile"]),
            ("role", meta["role_mode"]),
            ("window", meta["public_window_ms"]),
            ("horizon", str(len(record["days"]))),
        ):
            strata[f"{key}:{value}"][record["outcome"]] += 1

    safety_keys = sorted(
        {
            key
            for record in records
            for key in record["row"]
            if key in {"invalid", "emergency"}
            or "failure" in key
            or key.endswith("_invalid")
        }
    )

    def summed(key: str) -> int:
        return sum(int(record["row"].get(key, "0")) for record in records)

    telemetry_keys = {
        "tasks": "permuted_apply_tasks",
        "routes": "permuted_apply_routes",
        "cross_pairs": "permuted_apply_cross_pairs",
        "valid": "permuted_apply_valid",
        "certified": "permuted_apply_certified",
        "takeovers": "permuted_apply_takeovers",
        "lifetime_gain": "permuted_apply_lifetime_gain",
        "daily_gain": "permuted_apply_daily_gain",
        "serving_gain": "permuted_apply_serving_gain",
        "deadline_days": "permuted_apply_deadline_days",
        "mapping_failures": "permuted_apply_mapping_failures",
        "role_failures": "permuted_apply_role_failures",
        "shadow_state_failures": "permuted_apply_shadow_state_failures",
        "shadow_ledger_failures": "permuted_apply_shadow_ledger_failures",
        "uncertified_takeovers": "permuted_apply_uncertified_takeovers",
        "causal_noop_failures": "permuted_causal_noop_failures",
    }
    takeover_records = [
        record
        for record in records
        if int(record["row"].get("permuted_apply_takeovers", "0")) > 0
    ]
    takeover_windows = {record["meta"]["public_window_ms"] for record in takeover_records}
    takeover_roles = {record["meta"]["role_mode"] for record in takeover_records}
    takeover_players = {record["meta"]["players"] for record in takeover_records}
    takeover_suites = {record["key"][0] for record in takeover_records}

    lane_5000 = [
        record for record in records if record["meta"]["public_window_ms"] == "5000"
    ]
    equivalence_failures = []
    for record in lane_5000:
        day_mismatch = any(
            day.get("cumulative") != day.get("causal_parent_cumulative")
            or day.get("plan_hash") != day.get("causal_parent_plan_hash")
            for day in record["days"]
        )
        if (
            record["actual"] != record["parent"]
            or day_mismatch
            or int(record["row"].get("permuted_causal_noop_failures", "0")) != 0
        ):
            equivalence_failures.append(f"{record['key'][0]}:{record['key'][1]}")

    window_aggregate: dict[str, list[int]] = {}
    for window in ("5000", "10000", "15000"):
        window_records = [record for record in records if record["meta"]["public_window_ms"] == window]
        window_aggregate[window] = [
            sum(record["delta"][tier] for record in window_records)
            for tier in range(3)
        ]

    explicit_certificate_keys = (
        "permuted_apply_mapping_failures",
        "permuted_apply_role_failures",
        "permuted_apply_shadow_state_failures",
        "permuted_apply_shadow_ledger_failures",
        "permuted_apply_uncertified_takeovers",
        "permuted_causal_noop_failures",
    )
    zero_safety = all(summed(key) == 0 for key in safety_keys) and all(
        summed(key) == 0 for key in explicit_certificate_keys
    )
    gate = {
        "complete_5000_internal_equivalence": not equivalence_failures,
        "zero_causal_loss_or_component_regression": all(
            component >= 0 for record in records for component in record["delta"]
        ),
        "positive_aggregate_at_10000": tuple(window_aggregate["10000"]) > (0, 0, 0),
        "positive_aggregate_at_15000": tuple(window_aggregate["15000"]) > (0, 0, 0),
        "zero_safety_and_certificate_failures": zero_safety,
        "at_least_three_certified_takeovers": summed("permuted_apply_takeovers") >= 3,
        "takeovers_span_both_longer_windows": {"10000", "15000"}.issubset(takeover_windows),
        "takeovers_span_both_roles": {"fixed", "deadline"}.issubset(takeover_roles),
        "takeovers_span_two_player_counts": len(takeover_players) >= 2,
        "takeovers_span_two_map_strata": len(takeover_suites) >= 2,
    }
    gate["passed"] = all(gate.values())

    summary = {
        "experiment": "SCORE-CAUSAL-PERSISTENT-AGENT-PERMUTATION-254",
        "phase": args.phase,
        "cases": len(records),
        "causal_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "window_aggregate_score_delta": window_aggregate,
        "gain_tail": {"sum": sum(gains), "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "safety": {key: summed(key) for key in safety_keys},
        "candidate_work": {name: summed(key) for name, key in telemetry_keys.items()},
        "takeover_breadth": {
            "matches": len(takeover_records),
            "windows": sorted(takeover_windows),
            "roles": sorted(takeover_roles),
            "players": sorted(takeover_players),
            "suites": sorted(takeover_suites),
        },
        "equivalence_5000_failures": equivalence_failures,
        "response_ms_local_non_authoritative": {
            "max": max(int(record["row"].get("max_ms", "0")) for record in records)
        },
        "gate": gate,
    }
    rendered = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
