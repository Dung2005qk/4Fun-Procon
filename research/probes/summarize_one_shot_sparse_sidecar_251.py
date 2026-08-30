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
        sidecar_key = suite, seed, "sidecar"
        parent = cases.get(parent_key)
        sidecar = cases.get(sidecar_key)
        if parent is None or sidecar is None:
            raise RuntimeError(f"missing pair {(suite, seed)}")
        parent_score, sidecar_score = score(parent), score(sidecar)
        delta = tuple(s - p for s, p in zip(sidecar_score, parent_score))
        outcome = (
            "win" if sidecar_score > parent_score
            else "loss" if sidecar_score < parent_score
            else "tie"
        )
        pairs.append(
            (
                suite,
                seed,
                parent,
                sidecar,
                delta,
                outcome,
                plan_hashes[parent_key],
                plan_hashes[sidecar_key],
            )
        )
    if len(pairs) != expected:
        raise RuntimeError(f"paired {len(pairs)} != expected {expected}")

    outcomes = Counter(pair[5] for pair in pairs)
    tiers = Counter(first_tier(pair[4]) for pair in pairs)
    aggregate = [sum(pair[4][tier] for pair in pairs) for tier in range(3)]
    gains = [
        pair[4][first_tier(pair[4]) - 1]
        for pair in pairs
        if pair[5] == "win"
    ]
    losses = [
        pair[4][first_tier(pair[4]) - 1]
        for pair in pairs
        if pair[5] == "loss"
    ]

    strata: dict[str, Counter[str]] = defaultdict(Counter)
    for suite, seed, _parent, sidecar, _delta, outcome, parent_days, _sidecar_days in pairs:
        meta = metadata[(suite, seed)]
        for key, value in (
            ("suite", suite),
            ("players", meta["players"]),
            ("fuel", meta["fuel_profile"]),
            ("role", sidecar["role_mode"]),
            ("horizon", str(len(parent_days))),
        ):
            strata[f"{key}:{value}"][outcome] += 1

    safety_keys = sorted(
        {
            key
            for pair in pairs
            for row in (pair[2], pair[3])
            for key in row
            if key in {"invalid", "emergency"}
            or "failure" in key
            or key.endswith("_invalid")
        }
    )

    def summed(index: int, key: str) -> int:
        return sum(int(pair[index].get(key, "0")) for pair in pairs)

    telemetry = {
        "representatives": "current_day_sparse_representatives",
        "settled": "current_day_sparse_settled",
        "routes": "current_day_sparse_routes",
        "plans": "current_day_sparse_plans",
        "valid": "current_day_sparse_valid",
        "strict": "current_day_sparse_strict",
        "takeovers": "current_day_sparse_takeovers",
        "no_takeover_mismatches": "current_day_sparse_no_takeover_mismatches",
        "deadline_days": "current_day_sparse_deadline_days",
        "failure_days": "current_day_sparse_failure_days",
    }
    takeover_violations = [
        f"{suite}:{seed}"
        for suite, seed, _parent, sidecar, *_rest in pairs
        if int(sidecar.get("current_day_sparse_takeovers", "0")) > 1
    ]

    no_tier12_loss = all(
        not (pair[5] == "loss" and first_tier(pair[4]) in (1, 2))
        for pair in pairs
    )
    positive_aggregate = tuple(aggregate) > (0, 0, 0)
    gains_all_players = all(
        strata[f"players:{players}"]["win"] > 0 for players in ("8", "9", "10")
    )
    gains_both_roles = all(
        strata[f"role:{role}"]["win"] > 0 for role in ("fixed", "deadline")
    )
    no_losing_stratum = all(
        counts["loss"] <= counts["win"] for counts in strata.values()
    )
    bounded_individual_losses = all(
        first_tier(pair[4]) == 3 and abs(pair[4][2]) <= 10
        for pair in pairs
        if pair[5] == "loss"
    )
    gain_sum = sum(gains)
    loss_sum_abs = -sum(losses)
    bounded_total_loss_tail = loss_sum_abs * 10 < gain_sum if gain_sum > 0 else False
    zero_safety = all(
        summed(index, key) == 0
        for index in (2, 3)
        for key in safety_keys
    )

    gate = {
        "zero_safety": zero_safety,
        "no_tier1_or_tier2_loss": no_tier12_loss,
        "positive_official_aggregate": positive_aggregate,
        "gain_in_players_8_9_10": gains_all_players,
        "gain_in_fixed_and_deadline_roles": gains_both_roles,
        "no_public_stratum_losses_exceed_wins": no_losing_stratum,
        "each_tier3_loss_at_most_10": bounded_individual_losses,
        "total_loss_tail_below_10_percent_of_gains": bounded_total_loss_tail,
        "at_most_one_takeover_per_match": not takeover_violations,
        "zero_causal_no_takeover_self_mismatch":
            summed(3, "current_day_sparse_no_takeover_mismatches") == 0,
    }
    gate["passed"] = all(gate.values())

    summary = {
        "experiment": "SCORE-ONE-SHOT-SPARSE-SIDECAR-251",
        "phase": args.phase,
        "pairs": len(pairs),
        "wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "gain_tail": {"sum": gain_sum, "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "safety": {
            label: {key: summed(index, key) for key in safety_keys}
            for label, index in (("parent", 2), ("sidecar", 3))
        },
        "candidate_work": {
            name: summed(3, key) for name, key in telemetry.items()
        },
        "response_ms_local_non_authoritative": {
            label: {
                "max": max(int(pair[index].get("max_ms", "0")) for pair in pairs)
            }
            for label, index in (("parent", 2), ("sidecar", 3))
        },
        "takeover_violations": takeover_violations,
        "gate": gate,
    }
    rendered = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
