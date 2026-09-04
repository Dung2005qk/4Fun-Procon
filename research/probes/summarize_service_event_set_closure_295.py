#!/usr/bin/env python3
"""Summarize complete paired evidence for experiment 295."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


EXPECTED_MANIFEST_SHA256 = (
    "7CB896C1995156DC4ACA3D47E3FB3355A362BF964D289771A61E573F326A6152"
)
WORK_FIELDS = (
    "service_event_prefix_provider_columns",
    "service_event_full_provider_columns",
    "service_event_consumer_columns",
    "service_event_selected_prefix_groups",
    "service_event_selected_full_groups",
    "service_event_selected_consumers",
    "service_event_atomicity_failures",
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def parse_fields(line: str) -> dict[str, str]:
    result: dict[str, str] = {}
    for item in line.rstrip("\n").split(",")[1:]:
        key, separator, value = item.partition("=")
        if separator:
            result[key] = value
    return result


def compare(candidate: dict[str, str], parent: dict[str, str]) -> tuple[str, int, int]:
    for tier, key in enumerate(("lifetime", "daily", "servings"), start=1):
        delta = int(candidate[key]) - int(parent[key])
        if delta > 0:
            return "win", tier, delta
        if delta < 0:
            return "loss", tier, delta
    return "tie", 0, 0


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--log", type=Path, required=True)
    parser.add_argument("--phase", choices=("development", "holdout"), required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    if sha256(args.manifest) != EXPECTED_MANIFEST_SHA256:
        raise ValueError("frozen manifest hash mismatch")
    with args.manifest.open(newline="", encoding="utf-8-sig") as stream:
        manifest = [row for row in csv.DictReader(stream) if row["split"] == args.phase]
    expected_cases = sum(int(row["count"]) for row in manifest)
    if expected_cases != (30 if args.phase == "development" else 54):
        raise ValueError(f"unexpected {args.phase} case count: {expected_cases}")

    lines = args.log.read_text(encoding="utf-8-sig").splitlines()
    results = [parse_fields(line) for line in lines if line.startswith("result,")]
    completes = [line for line in lines if line.startswith("case_complete,")]
    run_complete = [line for line in lines if line.startswith("run_complete,")]
    if len(results) != 2 * expected_cases or len(completes) != expected_cases or len(run_complete) != 1:
        raise ValueError(
            f"incomplete atomic evidence: results={len(results)}, cases={len(completes)}, "
            f"run_complete={len(run_complete)}"
        )
    by_seed: dict[str, list[dict[str, str]]] = defaultdict(list)
    for row in results:
        if row.get("track") != "service-event-set-closure-295":
            raise ValueError("unexpected track in result")
        missing = [field for field in WORK_FIELDS if field not in row]
        if missing:
            raise ValueError(f"missing telemetry {missing} for seed {row.get('seed')}")
        by_seed[row["seed"]].append(row)
    if len(by_seed) != expected_cases or any(len(pair) != 2 for pair in by_seed.values()):
        raise ValueError("paired result bijection failed")

    outcomes = Counter()
    first_tiers = Counter()
    aggregate = [0, 0, 0]
    gains: list[int] = []
    losses: list[int] = []
    strata: dict[str, Counter[str]] = defaultdict(Counter)
    work = {field: 0 for field in WORK_FIELDS}
    parent_work = {field: 0 for field in WORK_FIELDS}
    safety = {
        "parent_invalid": 0,
        "parent_emergency": 0,
        "candidate_invalid": 0,
        "candidate_emergency": 0,
    }
    candidate_max_ms = 0
    parent_max_ms = 0
    win_players: set[str] = set()
    win_suites: set[str] = set()
    win_fuels: set[str] = set()
    win_tanker_identities: set[str] = set()
    paired_rows: list[dict[str, Any]] = []

    for seed, pair in sorted(by_seed.items(), key=lambda item: int(item[0])):
        parent = next(row for row in pair if row["version"] == "parent-295")
        candidate = next(row for row in pair if row["version"] == "candidate-295")
        outcome, tier, first_delta = compare(candidate, parent)
        outcomes[outcome] += 1
        first_tiers[str(tier)] += 1
        deltas = [
            int(candidate[key]) - int(parent[key])
            for key in ("lifetime", "daily", "servings")
        ]
        for index, delta in enumerate(deltas):
            aggregate[index] += delta
        if outcome == "win":
            gains.append(first_delta)
            win_players.add(candidate["players"])
            win_suites.add(candidate["suite"])
            win_fuels.add(candidate["fuel_profile"])
            win_tanker_identities.add(candidate["role_mask"])
        elif outcome == "loss":
            losses.append(first_delta)
        for name, value in (
            (f"players:{candidate['players']}", outcome),
            (f"suite:{candidate['suite']}", outcome),
            (f"fuel:{candidate['fuel_profile']}", outcome),
            (f"role_mask:{candidate['role_mask']}", outcome),
        ):
            strata[name][value] += 1
        for field in WORK_FIELDS:
            work[field] += int(candidate[field])
            parent_work[field] += int(parent[field])
        for arm_name, arm in (("parent", parent), ("candidate", candidate)):
            safety[f"{arm_name}_invalid"] += int(arm["invalid"])
            safety[f"{arm_name}_emergency"] += int(arm["emergency"])
        candidate_max_ms = max(candidate_max_ms, int(candidate["max_ms"]))
        parent_max_ms = max(parent_max_ms, int(parent["max_ms"]))
        paired_rows.append(
            {
                "seed": int(seed),
                "outcome": outcome,
                "first_difference_tier": tier,
                "first_difference_delta": first_delta,
                "aggregate_delta": deltas,
                "suite": candidate["suite"],
                "fuel": candidate["fuel_profile"],
                "players": int(candidate["players"]),
                "role_mask": int(candidate["role_mask"]),
                "selected_prefix_groups": int(candidate["service_event_selected_prefix_groups"]),
                "selected_full_groups": int(candidate["service_event_selected_full_groups"]),
                "selected_consumers": int(candidate["service_event_selected_consumers"]),
                "atomicity_failures": int(candidate["service_event_atomicity_failures"]),
            }
        )

    minimum_wins = 4 if args.phase == "development" else 6
    no_tier12_loss = all(
        row["outcome"] != "loss" or row["first_difference_tier"] == 3
        for row in paired_rows
    )
    bounded_tier3 = all(
        row["outcome"] != "loss" or row["first_difference_delta"] >= -4
        for row in paired_rows
    )
    gain_mass = sum(gains)
    loss_mass = -sum(losses)
    gate = {
        "zero_invalid_or_emergency": all(value == 0 for value in safety.values()),
        "canonical_checkpoint_bounded": candidate_max_ms <= 5000 and parent_max_ms <= 5000,
        "parent_flag_is_operation_isolated": all(value == 0 for value in parent_work.values()),
        "generated_prefix_groups": work["service_event_prefix_provider_columns"] > 0,
        "generated_full_groups": work["service_event_full_provider_columns"] > 0,
        "generated_consumers": work["service_event_consumer_columns"] > 0,
        "selected_prefix_groups": work["service_event_selected_prefix_groups"] > 0,
        "selected_full_groups": work["service_event_selected_full_groups"] > 0,
        "zero_atomicity_failure": work["service_event_atomicity_failures"] == 0,
        "no_tier1_or_tier2_loss": no_tier12_loss,
        "tier3_downside_bounded_to_four": bounded_tier3,
        "paired_wins_exceed_losses": outcomes["win"] > outcomes["loss"],
        "minimum_strict_wins": outcomes["win"] >= minimum_wins,
        "gain_mass_at_least_twice_loss_mass": gain_mass >= 2 * loss_mass,
        "wins_span_two_player_strata": len(win_players) >= 2,
        "wins_span_two_map_suites": len(win_suites) >= 2,
        "wins_span_two_fuel_profiles": len(win_fuels) >= 2,
        "wins_span_two_tanker_identities": len(win_tanker_identities) >= 2,
    }
    gate["passed"] = all(gate.values())

    report = {
        "experiment": "SCORE-SERVICE-EVENT-SET-CLOSURE-295",
        "phase": args.phase,
        "manifest_sha256": EXPECTED_MANIFEST_SHA256,
        "log_sha256": sha256(args.log),
        "paired_cases": expected_cases,
        "paired_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(first_tiers.items())),
        "aggregate_score_delta": aggregate,
        "gain_tail": {"sum": gain_mass, "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "candidate_work": work,
        "parent_work": parent_work,
        "safety": safety,
        "response_ms_local_non_authoritative": {
            "candidate_max": candidate_max_ms,
            "parent_max": parent_max_ms,
        },
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "paired_rows": paired_rows,
        "gate": gate,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        "paired_wtl": report["paired_wtl"],
        "aggregate_score_delta": aggregate,
        "gain_mass": gain_mass,
        "loss_mass": loss_mass,
        "candidate_work": work,
        "gate": gate,
    }, sort_keys=True))


if __name__ == "__main__":
    main()
