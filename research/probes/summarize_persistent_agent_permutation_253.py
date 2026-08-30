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

    cases: dict[tuple[str, int, str], dict[str, str]] = {}
    plan_hashes: dict[tuple[str, int, str], list[int]] = defaultdict(list)
    completed: set[tuple[str, int, str]] = set()
    active: tuple[str, int, str] | None = None
    run_complete = False
    for line in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("case_begin,"):
            row = fields(line)
            active = row["suite"], int(row["seed"]), row["label"]
        elif line.startswith("result,"):
            row = fields(line)
            key = row["suite"], int(row["seed"]), row["version"]
            if active != key:
                raise RuntimeError(f"result outside matching case block: {key}, active={active}")
            if key in cases:
                raise RuntimeError(f"duplicate result {key}")
            cases[key] = row
        elif line.startswith("day_detail,"):
            if active is None:
                raise RuntimeError("day_detail outside case block")
            row = fields(line)
            if row["version"] != active[2] or int(row["seed"]) != active[1]:
                raise RuntimeError(f"day_detail mismatch for active case {active}")
            plan_hashes[active].append(int(row["plan_hash"]))
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"]), row["label"]
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
    if set(cases) != completed:
        raise RuntimeError("result/case_complete bijection failed")

    pairs = []
    for suite, seed in sorted(metadata):
        parent_key = suite, seed, "parent"
        candidate_key = suite, seed, "candidate"
        parent = cases.get(parent_key)
        candidate = cases.get(candidate_key)
        if parent is None or candidate is None:
            raise RuntimeError(f"missing pair {(suite, seed)}")
        parent_score, candidate_score = score(parent), score(candidate)
        delta = tuple(c - p for c, p in zip(candidate_score, parent_score))
        outcome = (
            "win" if candidate_score > parent_score
            else "loss" if candidate_score < parent_score
            else "tie"
        )
        pairs.append(
            {
                "suite": suite,
                "seed": seed,
                "meta": metadata[(suite, seed)],
                "parent": parent,
                "candidate": candidate,
                "delta": delta,
                "outcome": outcome,
                "parent_hashes": plan_hashes[parent_key],
                "candidate_hashes": plan_hashes[candidate_key],
            }
        )
    if len(pairs) != expected:
        raise RuntimeError(f"paired {len(pairs)} != expected {expected}")

    outcomes = Counter(pair["outcome"] for pair in pairs)
    tiers = Counter(first_tier(pair["delta"]) for pair in pairs)
    aggregate = [sum(pair["delta"][tier] for pair in pairs) for tier in range(3)]
    gains = [
        pair["delta"][first_tier(pair["delta"]) - 1]
        for pair in pairs
        if pair["outcome"] == "win"
    ]
    losses = [
        pair["delta"][first_tier(pair["delta"]) - 1]
        for pair in pairs
        if pair["outcome"] == "loss"
    ]

    strata: dict[str, Counter[str]] = defaultdict(Counter)
    for pair in pairs:
        meta = pair["meta"]
        for key, value in (
            ("suite", pair["suite"]),
            ("players", meta["players"]),
            ("fuel", meta["fuel_profile"]),
            ("role", meta["role_mode"]),
            ("window", meta["public_window_ms"]),
            ("horizon", str(len(pair["parent_hashes"]))),
        ):
            strata[f"{key}:{value}"][pair["outcome"]] += 1

    safety_keys = sorted(
        {
            key
            for pair in pairs
            for row in (pair["parent"], pair["candidate"])
            for key in row
            if key in {"invalid", "emergency"}
            or "failure" in key
            or key.endswith("_invalid")
        }
    )

    def summed(label: str, key: str) -> int:
        return sum(int(pair[label].get(key, "0")) for pair in pairs)

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
    }
    takeover_pairs = [
        pair
        for pair in pairs
        if int(pair["candidate"].get("permuted_apply_takeovers", "0")) > 0
    ]
    takeover_windows = {
        pair["meta"]["public_window_ms"] for pair in takeover_pairs
    }
    takeover_roles = {pair["meta"]["role_mode"] for pair in takeover_pairs}
    takeover_players = {pair["meta"]["players"] for pair in takeover_pairs}
    takeover_suites = {pair["suite"] for pair in takeover_pairs}

    lane_5000 = [
        pair for pair in pairs if pair["meta"]["public_window_ms"] == "5000"
    ]
    equivalence_failures = [
        f"{pair['suite']}:{pair['seed']}"
        for pair in lane_5000
        if score(pair["parent"]) != score(pair["candidate"])
        or pair["parent_hashes"] != pair["candidate_hashes"]
    ]
    window_aggregate: dict[str, list[int]] = {}
    for window in ("5000", "10000", "15000"):
        window_pairs = [
            pair for pair in pairs if pair["meta"]["public_window_ms"] == window
        ]
        window_aggregate[window] = [
            sum(pair["delta"][tier] for pair in window_pairs)
            for tier in range(3)
        ]

    explicit_certificate_failures = all(
        summed("candidate", key) == 0
        for key in (
            "permuted_apply_mapping_failures",
            "permuted_apply_role_failures",
            "permuted_apply_shadow_state_failures",
            "permuted_apply_shadow_ledger_failures",
            "permuted_apply_uncertified_takeovers",
        )
    )
    zero_safety = all(
        summed(label, key) == 0
        for label in ("parent", "candidate")
        for key in safety_keys
    ) and explicit_certificate_failures
    gate = {
        "complete_5000_score_and_plan_hash_equivalence": not equivalence_failures,
        "zero_loss_at_every_official_tier": all(
            component >= 0
            for pair in pairs
            for component in pair["delta"]
        ),
        "positive_aggregate_at_10000": tuple(window_aggregate["10000"]) > (0, 0, 0),
        "positive_aggregate_at_15000": tuple(window_aggregate["15000"]) > (0, 0, 0),
        "zero_safety_and_certificate_failures": zero_safety,
        "at_least_three_certified_takeovers":
            summed("candidate", "permuted_apply_takeovers") >= 3,
        "takeovers_span_both_longer_windows":
            {"10000", "15000"}.issubset(takeover_windows),
        "takeovers_span_both_roles":
            {"fixed", "deadline"}.issubset(takeover_roles),
        "takeovers_span_two_player_counts": len(takeover_players) >= 2,
        "takeovers_span_two_map_strata": len(takeover_suites) >= 2,
    }
    gate["passed"] = all(gate.values())

    summary = {
        "experiment": "SCORE-PERSISTENT-AGENT-PERMUTATION-253",
        "phase": args.phase,
        "pairs": len(pairs),
        "wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "window_aggregate_score_delta": window_aggregate,
        "gain_tail": {"sum": sum(gains), "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "safety": {
            label: {key: summed(label, key) for key in safety_keys}
            for label in ("parent", "candidate")
        },
        "candidate_work": {
            name: summed("candidate", key)
            for name, key in telemetry_keys.items()
        },
        "takeover_breadth": {
            "matches": len(takeover_pairs),
            "windows": sorted(takeover_windows),
            "roles": sorted(takeover_roles),
            "players": sorted(takeover_players),
            "suites": sorted(takeover_suites),
        },
        "equivalence_5000_failures": equivalence_failures,
        "response_ms_local_non_authoritative": {
            label: {
                "max": max(int(pair[label].get("max_ms", "0")) for pair in pairs)
            }
            for label in ("parent", "candidate")
        },
        "gate": gate,
    }
    rendered = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
