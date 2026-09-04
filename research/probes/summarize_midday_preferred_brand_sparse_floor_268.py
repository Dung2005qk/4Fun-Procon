#!/usr/bin/env python3
import argparse
import csv
import json
from collections import Counter, defaultdict
from pathlib import Path


def fields(line: str) -> dict[str, str]:
    out: dict[str, str] = {}
    for token in line.split(",")[1:]:
        if "=" in token:
            key, value = token.split("=", 1)
            out[key] = value
    return out


def score(value: str) -> tuple[int, int, int]:
    parts = tuple(int(part) for part in value.split("/"))
    if len(parts) != 3:
        raise RuntimeError(f"invalid score {value}")
    return parts


def actual(row: dict[str, str]) -> tuple[int, int, int]:
    return int(row["lifetime"]), int(row["daily"]), int(row["servings"])


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
                raise RuntimeError(f"duplicate manifest key {key}")
            metadata[key] = row

    results: dict[tuple[str, int], dict[str, str]] = {}
    days: dict[tuple[str, int], list[dict[str, str]]] = defaultdict(list)
    complete: set[tuple[str, int]] = set()
    active: tuple[str, int] | None = None
    run_complete = False
    for line in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("case_begin,"):
            row = fields(line)
            active = row["suite"], int(row["seed"])
        elif line.startswith("result,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active != key or key in results:
                raise RuntimeError(f"invalid result {key}, active={active}")
            results[key] = row
        elif line.startswith("day_detail,"):
            if active is None:
                raise RuntimeError("day detail outside case")
            row = fields(line)
            if int(row["seed"]) != active[1]:
                raise RuntimeError(f"day detail mismatch {active}")
            days[active].append(row)
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active != key:
                raise RuntimeError(f"completion mismatch {key}, active={active}")
            complete.add(key)
            active = None
        elif line.startswith("run_complete,"):
            run_complete = True
    if active is not None or not run_complete:
        raise RuntimeError("incomplete atomic run")
    if set(results) != complete or set(results) != set(metadata):
        raise RuntimeError("manifest/result/completion bijection failed")

    required = {
        "lifetime", "daily", "servings", "checkpoint_closed_loop_parent",
        "checkpoint_closed_loop_failures", "preferred_brand_routes",
        "preferred_brand_plans", "preferred_brand_valid",
        "preferred_brand_acceptances", "preferred_brand_rounds",
        "preferred_brand_takeovers", "preferred_brand_deadline_days",
        "preferred_brand_failure_days", "preferred_brand_tier1_gains",
        "preferred_brand_tier2_gains", "preferred_brand_serving_gain",
        "invalid", "emergency",
    }
    records = []
    for key in sorted(metadata):
        row = results[key]
        missing = required - set(row)
        if missing:
            raise RuntimeError(f"missing fields {key}: {sorted(missing)}")
        day_rows = days[key]
        if not 4 <= len(day_rows) <= 10:
            raise RuntimeError(f"bad horizon {key}: {len(day_rows)}")
        parent = score(row["checkpoint_closed_loop_parent"])
        candidate = actual(row)
        delta = tuple(c - p for c, p in zip(candidate, parent))
        outcome = "win" if candidate > parent else "loss" if candidate < parent else "tie"
        records.append({"key": key, "meta": metadata[key], "row": row, "days": day_rows, "parent": parent, "candidate": candidate, "delta": delta, "outcome": outcome})

    outcomes = Counter(record["outcome"] for record in records)
    aggregate = [sum(record["delta"][i] for record in records) for i in range(3)]
    safety_keys = sorted({
        key for record in records for key in record["row"]
        if key in {"invalid", "emergency"} or "failure" in key or key.endswith("_invalid")
    })
    summed = lambda name: sum(int(record["row"].get(name, "0")) for record in records)
    lane_5000 = [record for record in records if record["meta"]["public_window_ms"] == "5000"]
    equivalence_failures = []
    for record in lane_5000:
        row = record["row"]
        exact_days = all(
            day.get("cumulative") == day.get("causal_parent_cumulative")
            and day.get("plan_hash") == day.get("causal_parent_plan_hash")
            for day in record["days"]
        )
        candidate_work = sum(int(row[name]) for name in (
            "preferred_brand_routes", "preferred_brand_plans",
            "preferred_brand_valid", "preferred_brand_acceptances",
            "preferred_brand_rounds", "preferred_brand_takeovers",
            "preferred_brand_deadline_days", "preferred_brand_failure_days",
        ))
        if record["candidate"] != record["parent"] or not exact_days or candidate_work:
            equivalence_failures.append(f"{record['key'][0]}:{record['key'][1]}")

    takeover_records = [record for record in records if int(record["row"]["preferred_brand_takeovers"]) > 0]
    takeover_suites = sorted({record["meta"]["suite"] for record in takeover_records})
    takeover_roles = sorted({record["meta"]["role_mode"] for record in takeover_records})
    takeover_players = sorted({record["meta"]["players"] for record in takeover_records})
    candidate_gain = (
        summed("preferred_brand_tier1_gains"),
        summed("preferred_brand_tier2_gains"),
        summed("preferred_brand_serving_gain"),
    )
    zero_safety = all(summed(name) == 0 for name in safety_keys)
    componentwise_safe = all(all(value >= 0 for value in record["delta"]) for record in records)
    strata: dict[str, Counter[str]] = defaultdict(Counter)
    for record in records:
        for name, value in (
            ("suite", record["meta"]["suite"]), ("players", record["meta"]["players"]),
            ("fuel", record["meta"]["fuel_profile"]), ("role", record["meta"]["role_mode"]),
            ("window", record["meta"]["public_window_ms"]), ("horizon", str(len(record["days"]))),
        ):
            strata[f"{name}:{value}"][record["outcome"]] += 1

    gate = {
        "exact_5000_parent_equivalence": not equivalence_failures,
        "zero_safety_or_certificate_failure": zero_safety,
        "zero_deadline_rollback": summed("preferred_brand_deadline_days") == 0,
        "componentwise_protected_score_safety": outcomes["loss"] == 0 and componentwise_safe,
        "positive_candidate_specific_gain": candidate_gain > (0, 0, 0),
        "at_least_three_certified_takeovers": len(takeover_records) >= 3,
        "takeover_breadth_two_suites": len(takeover_suites) >= 2,
        "takeover_breadth_both_roles": takeover_roles == ["deadline", "fixed"],
    }
    gate["passed"] = all(gate.values())
    summary = {
        "experiment": "SCORE-MIDDAY-PREFERRED-BRAND-SPARSE-FLOOR-268",
        "phase": args.phase,
        "cases": len(records),
        "protected_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "protected_aggregate_score_delta": aggregate,
        "candidate_specific_gain": candidate_gain,
        "candidate_work": {
            "routes": summed("preferred_brand_routes"),
            "plans": summed("preferred_brand_plans"),
            "valid": summed("preferred_brand_valid"),
            "acceptances": summed("preferred_brand_acceptances"),
            "rounds": summed("preferred_brand_rounds"),
            "takeover_cases": len(takeover_records),
            "deadline_days": summed("preferred_brand_deadline_days"),
            "failure_days": summed("preferred_brand_failure_days"),
        },
        "takeover_breadth": {"suites": takeover_suites, "roles": takeover_roles, "players": takeover_players},
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "safety": {key: summed(key) for key in safety_keys},
        "equivalence_failures": equivalence_failures,
        "gate": gate,
    }
    rendered = json.dumps(summary, indent=2, sort_keys=True)
    if args.output:
        Path(args.output).write_text(rendered + "\n", encoding="utf-8")
    print(rendered)


if __name__ == "__main__":
    main()
