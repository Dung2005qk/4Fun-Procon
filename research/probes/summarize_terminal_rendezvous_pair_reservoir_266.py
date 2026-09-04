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


def actual_score(row: dict[str, str]) -> tuple[int, int, int]:
    return int(row["lifetime"]), int(row["daily"]), int(row["servings"])


def slash_score(value: str) -> tuple[int, int, int]:
    values = tuple(int(part) for part in value.split("/"))
    if len(values) != 3:
        raise RuntimeError(f"invalid score {value!r}")
    return values


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
            if active != key or key in cases:
                raise RuntimeError(f"invalid or duplicate result {key}, active={active}")
            cases[key] = row
        elif line.startswith("day_detail,"):
            if active is None:
                raise RuntimeError("day_detail outside case block")
            row = fields(line)
            if int(row["seed"]) != active[1]:
                raise RuntimeError(f"day_detail mismatch for {active}")
            day_rows[active].append(row)
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
    if len(cases) != expected:
        raise RuntimeError(f"cases {len(cases)} != expected {expected}")

    records = []
    required_result_fields = {
        "lifetime",
        "daily",
        "servings",
        "terminal_rendezvous_pairs",
        "terminal_rendezvous_plans",
        "terminal_rendezvous_valid",
        "terminal_rendezvous_acceptances",
        "terminal_rendezvous_deadline",
        "terminal_rendezvous_failure",
        "terminal_rendezvous_parent",
        "terminal_rendezvous_refined",
        "checkpoint_closed_loop_failures",
        "invalid",
        "emergency",
    }
    for key in sorted(metadata):
        row = cases[key]
        missing = sorted(required_result_fields - set(row))
        if missing:
            raise RuntimeError(f"missing result fields for {key}: {missing}")
        parent = slash_score(row["terminal_rendezvous_parent"])
        actual = actual_score(row)
        if slash_score(row["terminal_rendezvous_refined"]) != actual:
            raise RuntimeError(f"refined/result score mismatch for {key}")
        days = day_rows[key]
        if not 4 <= len(days) <= 10 or [int(day["day"]) for day in days] != list(
            range(1, len(days) + 1)
        ):
            raise RuntimeError(f"invalid day-detail sequence for {key}")
        delta = tuple(a - p for a, p in zip(actual, parent))
        outcome = "win" if actual > parent else "loss" if actual < parent else "tie"
        records.append(
            {
                "key": key,
                "meta": metadata[key],
                "row": row,
                "days": days,
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
        for name, value in (
            ("suite", record["key"][0]),
            ("players", meta["players"]),
            ("fuel", meta["fuel_profile"]),
            ("role", meta["role_mode"]),
            ("window", meta["public_window_ms"]),
            ("horizon", str(len(record["days"]))),
        ):
            strata[f"{name}:{value}"][record["outcome"]] += 1

    def summed(key: str) -> int:
        return sum(int(record["row"].get(key, "0")) for record in records)

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
    lane_5000 = [
        record for record in records if record["meta"]["public_window_ms"] == "5000"
    ]
    equivalence_failures = []
    for record in lane_5000:
        row = record["row"]
        exact_day_equivalence = all(
            day.get("cumulative") == day.get("causal_parent_cumulative")
            and day.get("plan_hash") == day.get("causal_parent_plan_hash")
            for day in record["days"]
        )
        if (
            record["actual"] != record["parent"]
            or not exact_day_equivalence
            or int(row.get("terminal_rendezvous_pairs", "0")) != 0
            or int(row.get("terminal_rendezvous_plans", "0")) != 0
            or int(row.get("terminal_rendezvous_valid", "0")) != 0
            or int(row.get("terminal_rendezvous_acceptances", "0")) != 0
            or int(row.get("terminal_rendezvous_deadline", "0")) != 0
            or int(row.get("terminal_rendezvous_failure", "0")) != 0
        ):
            equivalence_failures.append(f"{record['key'][0]}:{record['key'][1]}")

    window_aggregate: dict[str, list[int]] = {}
    for window in ("5000", "10000", "15000"):
        selected = [record for record in records if record["meta"]["public_window_ms"] == window]
        window_aggregate[window] = [
            sum(record["delta"][tier] for record in selected) for tier in range(3)
        ]

    takeover_records = [
        record
        for record in records
        if int(record["row"].get("terminal_rendezvous_acceptances", "0")) > 0
    ]
    zero_safety = all(summed(key) == 0 for key in safety_keys)
    no_component_regression = all(
        component >= 0 for record in records for component in record["delta"]
    )
    takeover_windows = {
        record["meta"]["public_window_ms"] for record in takeover_records
    }
    breadth_axes = sum(
        len({record["meta"][field] for record in takeover_records}) >= 2
        for field in ("suite", "fuel_profile", "players", "role_mode")
    )
    gate = {
        "complete_5000_internal_equivalence": not equivalence_failures,
        "zero_causal_loss_or_component_regression":
            outcomes["loss"] == 0 and no_component_regression,
        "positive_aggregate_official_gain":
            outcomes["win"] > 0 and tuple(aggregate) > (0, 0, 0),
        "at_least_two_strict_takeovers": len(takeover_records) >= 2,
        "gains_in_both_longer_windows":
            {"10000", "15000"}.issubset(takeover_windows),
        "takeovers_span_at_least_two_public_axes": breadth_axes >= 2,
        "zero_safety_and_certificate_failures": zero_safety,
    }
    gate["passed"] = all(gate.values())

    summary = {
        "experiment": "SCORE-TERMINAL-RENDEZVOUS-PAIR-RESERVOIR-266",
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
        "candidate_work": {
            "pairs": summed("terminal_rendezvous_pairs"),
            "plans": summed("terminal_rendezvous_plans"),
            "valid": summed("terminal_rendezvous_valid"),
            "acceptances": summed("terminal_rendezvous_acceptances"),
            "deadline_rollbacks": summed("terminal_rendezvous_deadline"),
            "failures": summed("terminal_rendezvous_failure"),
        },
        "takeover_breadth": {
            "matches": len(takeover_records),
            "windows": sorted({r["meta"]["public_window_ms"] for r in takeover_records}),
            "roles": sorted({r["meta"]["role_mode"] for r in takeover_records}),
            "players": sorted({r["meta"]["players"] for r in takeover_records}),
            "suites": sorted({r["key"][0] for r in takeover_records}),
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
