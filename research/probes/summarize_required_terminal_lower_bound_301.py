#!/usr/bin/env python3
"""Summarize complete paired evidence for experiment 301."""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path
from typing import Any


EXPECTED_MANIFEST_SHA256 = (
    "B5BEED7027A7CE32CAA8CF878F2B573425F636E004974A765E26B08A9A3BF103"
)


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def fields(line: str) -> dict[str, str]:
    parsed: dict[str, str] = {}
    for token in line.rstrip("\n").split(",")[1:]:
        key, separator, value = token.partition("=")
        if separator:
            parsed[key] = value
    return parsed


def score(row: dict[str, str]) -> tuple[int, int, int]:
    return int(row["lifetime"]), int(row["daily"]), int(row["servings"])


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
        manifest_rows = [
            row for row in csv.DictReader(stream) if row["split"] == args.phase
        ]
    metadata: dict[tuple[str, int], dict[str, str]] = {}
    for row in manifest_rows:
        for offset in range(int(row["count"])):
            key = row["suite"], int(row["first_seed"]) + offset
            if key in metadata:
                raise ValueError(f"duplicate manifest case {key}")
            metadata[key] = row
    expected = 30 if args.phase == "development" else 54
    if len(metadata) != expected:
        raise ValueError(f"unexpected {args.phase} case count: {len(metadata)}")

    results: dict[tuple[str, int], dict[str, dict[str, str]]] = defaultdict(dict)
    days: dict[tuple[str, int, str], list[dict[str, str]]] = defaultdict(list)
    completed: set[tuple[str, int]] = set()
    active: tuple[str, int] | None = None
    run_complete = False
    for line in args.log.read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("case_begin,"):
            row = fields(line)
            active = row["suite"], int(row["seed"])
        elif line.startswith("result,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            arm = row["version"]
            if active != key or arm not in ("parent-301", "candidate-301") or arm in results[key]:
                raise ValueError(f"invalid result {key}/{arm}, active={active}")
            if row.get("track") != "required-terminal-lower-bound-301":
                raise ValueError("unexpected result track")
            results[key][arm] = row
        elif line.startswith("day_detail,"):
            row = fields(line)
            if active is None or int(row["seed"]) != active[1]:
                raise ValueError("day detail outside active case")
            arm = row["version"]
            days[(active[0], active[1], arm)].append(row)
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active != key:
                raise ValueError(f"completion mismatch {key}, active={active}")
            completed.add(key)
            active = None
        elif line.startswith("run_complete,"):
            run_complete = True
    if active is not None or not run_complete or completed != set(metadata):
        raise ValueError("incomplete atomic evidence")
    if set(results) != set(metadata) or any(len(pair) != 2 for pair in results.values()):
        raise ValueError("paired result bijection failed")

    safety_fields = (
        "invalid",
        "emergency",
        "checkpoint_closed_loop_failures",
        "terminal_sparse_failure",
        "terminal_marginal_failure",
        "midday_failure_days",
        "protected_terminal_invalid",
    )
    outcomes = Counter()
    longer_outcomes = Counter()
    tiers = Counter()
    aggregate = [0, 0, 0]
    gains: list[int] = []
    losses: list[int] = []
    strata: dict[str, Counter[str]] = defaultdict(Counter)
    safety = {f"{arm}_{field}": 0 for arm in ("parent", "candidate") for field in safety_fields}
    equivalence_failures: list[str] = []
    deadline_overruns: list[str] = []
    win_suites: set[str] = set()
    win_fuels: set[str] = set()
    win_windows: set[str] = set()
    paired_rows: list[dict[str, Any]] = []

    for key in sorted(metadata, key=lambda item: item[1]):
        meta = metadata[key]
        pair = results[key]
        parent = pair["parent-301"]
        candidate = pair["candidate-301"]
        for arm_name, row in (("parent", parent), ("candidate", candidate)):
            missing = [field for field in safety_fields if field not in row]
            if missing:
                raise ValueError(f"missing safety fields {missing} for {key}/{arm_name}")
            for field in safety_fields:
                safety[f"{arm_name}_{field}"] += int(row[field])
            if int(row["max_ms"]) > int(meta["public_window_ms"]):
                deadline_overruns.append(f"{key[0]}:{key[1]}:{arm_name}")
        outcome, tier, first_delta = compare(candidate, parent)
        deltas = tuple(c - p for c, p in zip(score(candidate), score(parent)))
        outcomes[outcome] += 1
        tiers[str(tier)] += 1
        for offset, delta in enumerate(deltas):
            aggregate[offset] += delta
        for label, value in (
            (f"suite:{key[0]}", outcome),
            (f"fuel:{meta['fuel_profile']}", outcome),
            (f"players:{meta['players']}", outcome),
            (f"role:{meta['role_mode']}", outcome),
            (f"window:{meta['public_window_ms']}", outcome),
        ):
            strata[label][value] += 1
        if meta["public_window_ms"] == "5000":
            parent_days = days[(key[0], key[1], "parent-301")]
            candidate_days = days[(key[0], key[1], "candidate-301")]
            exact_days = (
                len(parent_days) == len(candidate_days)
                and all(
                    p.get("cumulative") == c.get("cumulative")
                    and p.get("plan_hash") == c.get("plan_hash")
                    and p.get("causal_parent_cumulative") == c.get("causal_parent_cumulative")
                    and p.get("causal_parent_plan_hash") == c.get("causal_parent_plan_hash")
                    for p, c in zip(parent_days, candidate_days)
                )
            )
            if outcome != "tie" or not exact_days:
                equivalence_failures.append(f"{key[0]}:{key[1]}")
        else:
            longer_outcomes[outcome] += 1
            if outcome == "win":
                gains.append(first_delta)
                win_suites.add(key[0])
                win_fuels.add(meta["fuel_profile"])
                win_windows.add(meta["public_window_ms"])
            elif outcome == "loss":
                losses.append(first_delta)
        paired_rows.append(
            {
                "suite": key[0],
                "seed": key[1],
                "window": int(meta["public_window_ms"]),
                "fuel": meta["fuel_profile"],
                "outcome": outcome,
                "first_difference_tier": tier,
                "first_difference_delta": first_delta,
                "score_delta": deltas,
                "parent_midday_target_routes": int(parent["midday_target_routes"]),
                "candidate_midday_target_routes": int(candidate["midday_target_routes"]),
                "parent_midday_target_acceptances": int(parent["midday_target_acceptances"]),
                "candidate_midday_target_acceptances": int(candidate["midday_target_acceptances"]),
            }
        )

    longer_rows = [row for row in paired_rows if row["window"] > 5000]
    no_high_tier_loss = all(
        row["outcome"] != "loss" or row["first_difference_tier"] == 3
        for row in longer_rows
    )
    bounded_tier3 = all(
        row["outcome"] != "loss" or row["first_difference_delta"] >= -4
        for row in longer_rows
    )
    minimum_wins = 3 if args.phase == "development" else 5
    gain_mass = sum(gains)
    loss_mass = -sum(losses)
    gate = {
        "zero_safety_failure": all(value == 0 for value in safety.values()),
        "within_declared_public_window_local_check": not deadline_overruns,
        "exact_5000_score_and_plan_equivalence": not equivalence_failures,
        "no_tier1_or_tier2_longer_window_loss": no_high_tier_loss,
        "tier3_downside_bounded_to_four": bounded_tier3,
        "longer_window_wins_exceed_losses": longer_outcomes["win"] > longer_outcomes["loss"],
        "minimum_strict_longer_window_wins": longer_outcomes["win"] >= minimum_wins,
        "gain_mass_at_least_twice_loss_mass": gain_mass >= 2 * loss_mass,
        "wins_span_two_map_suites": len(win_suites) >= 2,
        "wins_span_two_fuel_profiles": len(win_fuels) >= 2,
        "wins_span_both_longer_windows": win_windows == {"10000", "15000"},
    }
    gate["passed"] = all(gate.values())
    report = {
        "experiment": "SCORE-REQUIRED-TERMINAL-LOWER-BOUND-301",
        "phase": args.phase,
        "manifest_sha256": EXPECTED_MANIFEST_SHA256,
        "log_sha256": sha256(args.log),
        "paired_cases": len(metadata),
        "paired_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "longer_window_wtl": [
            longer_outcomes["win"],
            longer_outcomes["tie"],
            longer_outcomes["loss"],
        ],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "gain_tail": {"sum": gain_mass, "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "safety": safety,
        "deadline_overruns_local": deadline_overruns,
        "equivalence_5000_failures": equivalence_failures,
        "win_breadth": {
            "suites": sorted(win_suites),
            "fuels": sorted(win_fuels),
            "windows": sorted(win_windows),
        },
        "paired_rows": paired_rows,
        "gate": gate,
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        "paired_wtl": report["paired_wtl"],
        "longer_window_wtl": report["longer_window_wtl"],
        "aggregate_score_delta": aggregate,
        "gain_mass": gain_mass,
        "loss_mass": loss_mass,
        "gate": gate,
    }, sort_keys=True))


if __name__ == "__main__":
    main()
