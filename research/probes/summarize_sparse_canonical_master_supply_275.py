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
    metadata: dict[tuple[str, int], dict[str, str]] = {}
    for row in manifest:
        for offset in range(int(row["count"])):
            key = row["suite"], int(row["first_seed"]) + offset
            if key in metadata:
                raise RuntimeError(f"duplicate manifest case {key}")
            metadata[key] = row

    results: dict[tuple[str, int, str], dict[str, str]] = {}
    day_rows: dict[tuple[str, int, str], list[dict[str, str]]] = defaultdict(list)
    complete: set[tuple[str, int]] = set()
    active_case: tuple[str, int] | None = None
    active_side: str | None = None
    run_complete = False
    for line in Path(args.log).read_text(encoding="utf-8-sig").splitlines():
        if line.startswith("case_begin,"):
            row = fields(line)
            active_case = row["suite"], int(row["seed"])
        elif line.startswith("side_begin,"):
            if active_case is None or active_side is not None:
                raise RuntimeError("invalid side_begin placement")
            active_side = fields(line)["side"]
            if active_side not in {"off", "on"}:
                raise RuntimeError(f"invalid side {active_side}")
        elif line.startswith("result,"):
            if active_case is None or active_side is None:
                raise RuntimeError("result outside side block")
            row = fields(line)
            key = row["suite"], int(row["seed"]), active_side
            if key[:2] != active_case or key in results:
                raise RuntimeError(f"duplicate or mismatched result {key}")
            expected = "parent-275" if active_side == "off" else "candidate-275"
            if row.get("version") != expected:
                raise RuntimeError(f"version mismatch for {key}: {row.get('version')}")
            results[key] = row
        elif line.startswith("day_detail,"):
            if active_case is None or active_side is None:
                raise RuntimeError("day detail outside side block")
            row = fields(line)
            key = active_case[0], active_case[1], active_side
            if int(row["seed"]) != active_case[1]:
                raise RuntimeError(f"day detail mismatch {key}")
            day_rows[key].append(row)
        elif line.startswith("side_complete,"):
            side = fields(line)["side"]
            if active_case is None or side != active_side:
                raise RuntimeError("side completion mismatch")
            if (active_case[0], active_case[1], side) not in results:
                raise RuntimeError("side completed without result")
            active_side = None
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active_case != key or active_side is not None:
                raise RuntimeError(f"case completion mismatch {key}")
            complete.add(key)
            active_case = None
        elif line.startswith("run_complete,"):
            run_complete = True

    expected_keys = {
        (suite, seed, side)
        for suite, seed in metadata
        for side in ("off", "on")
    }
    if active_case is not None or active_side is not None or not run_complete:
        raise RuntimeError("incomplete atomic run")
    if complete != set(metadata) or set(results) != expected_keys:
        raise RuntimeError("manifest/result/case_complete bijection failed")

    telemetry = (
        "sparse_canonical_tasks",
        "sparse_canonical_completed_tasks",
        "sparse_canonical_settled",
        "sparse_canonical_routes",
        "sparse_canonical_novel_routes",
        "sparse_canonical_candidates",
        "sparse_canonical_deadline_days",
        "sparse_canonical_failure_days",
    )
    records = []
    for key in sorted(metadata):
        parent_row = results[key[0], key[1], "off"]
        candidate_row = results[key[0], key[1], "on"]
        missing = set(telemetry) - set(candidate_row)
        if missing:
            raise RuntimeError(f"missing candidate telemetry {key}: {sorted(missing)}")
        parent = score(parent_row)
        candidate = score(candidate_row)
        delta = tuple(c - p for c, p in zip(candidate, parent))
        outcome = "win" if candidate > parent else "loss" if candidate < parent else "tie"
        records.append({
            "key": key,
            "meta": metadata[key],
            "parent_row": parent_row,
            "candidate_row": candidate_row,
            "parent_days": day_rows[key[0], key[1], "off"],
            "candidate_days": day_rows[key[0], key[1], "on"],
            "delta": delta,
            "outcome": outcome,
        })

    outcomes = Counter(record["outcome"] for record in records)
    tiers = Counter(first_tier(record["delta"]) for record in records)
    aggregate = [sum(record["delta"][tier] for record in records) for tier in range(3)]
    gains = [
        record["delta"][first_tier(record["delta"]) - 1]
        for record in records if record["outcome"] == "win"
    ]
    losses = [
        record["delta"][first_tier(record["delta"]) - 1]
        for record in records if record["outcome"] == "loss"
    ]

    strata: dict[str, Counter[str]] = defaultdict(Counter)
    strata_delta: dict[str, list[int]] = defaultdict(lambda: [0, 0, 0])
    for record in records:
        meta = record["meta"]
        for axis, value in (
            ("suite", record["key"][0]),
            ("players", meta["players"]),
            ("fuel", meta["fuel_profile"]),
            ("role", meta["role_mode"]),
            ("spots", meta["spot_count"]),
            ("horizon", str(len(record["candidate_days"]))),
        ):
            label = f"{axis}:{value}"
            strata[label][record["outcome"]] += 1
            for tier in range(3):
                strata_delta[label][tier] += record["delta"][tier]

    def summed(side: str, key: str) -> int:
        row_name = "parent_row" if side == "off" else "candidate_row"
        return sum(int(record[row_name].get(key, "0")) for record in records)

    safety_keys = sorted({
        key
        for record in records
        for row_name in ("parent_row", "candidate_row")
        for key in record[row_name]
        if key in {"invalid", "emergency"}
        or "failure" in key
        or key.endswith("_invalid")
    })
    tier12_losses = [
        f"{record['key'][0]}:{record['key'][1]}"
        for record in records
        if record["outcome"] == "loss" and first_tier(record["delta"]) in {1, 2}
    ]
    tier3_losses = [
        record["delta"][2]
        for record in records
        if record["outcome"] == "loss" and first_tier(record["delta"]) == 3
    ]
    win_records = [record for record in records if record["outcome"] == "win"]
    win_roles = {record["meta"]["role_mode"] for record in win_records}
    win_players = {record["meta"]["players"] for record in win_records}
    win_suites = {record["key"][0] for record in win_records}
    win_fuels = {record["meta"]["fuel_profile"] for record in win_records}
    dense_controls = [record for record in records if record["key"][0] == "general"]
    dense_control_failures = []
    for record in dense_controls:
        row = record["candidate_row"]
        work = sum(int(row.get(key, "0")) for key in telemetry[:-2])
        safety_difference = any(
            int(record["parent_row"].get(key, "0")) != int(row.get(key, "0"))
            for key in safety_keys
        )
        if work != 0 or safety_difference:
            dense_control_failures.append(f"{record['key'][0]}:{record['key'][1]}")

    total_gain = sum(gains)
    total_loss = -sum(losses)
    zero_safety = all(
        summed(side, key) == 0
        for side in ("off", "on")
        for key in safety_keys
    )
    gate = {
        "zero_safety_failures": zero_safety,
        "zero_tier1_or_tier2_losses": not tier12_losses,
        "at_least_six_paired_wins": outcomes["win"] >= 6,
        "wins_span_both_roles": win_roles == {"deadline", "fixed"},
        "wins_span_players_8_9_10": win_players == {"8", "9", "10"},
        "wins_span_three_map_strata": len(win_suites) >= 3,
        "wins_span_two_fuel_classes": len(win_fuels) >= 2,
        "at_most_two_losses": outcomes["loss"] <= 2,
        "worst_tier3_loss_at_least_minus_5": min(tier3_losses, default=0) >= -5,
        "first_tier_gain_loss_ratio_at_least_four": total_gain >= 4 * total_loss,
        "dense_supported_controls_inert_and_safe": not dense_control_failures,
    }
    gate["passed"] = all(gate.values())

    summary = {
        "experiment": "SCORE-SPARSE-CANONICAL-MASTER-SUPPLY-275",
        "phase": args.phase,
        "cases": len(records),
        "paired_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "gain_tail": {"sum": total_gain, "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "tier12_loss_cases": tier12_losses,
        "dense_control_failures": dense_control_failures,
        "win_breadth": {
            "roles": sorted(win_roles),
            "players": sorted(win_players),
            "suites": sorted(win_suites),
            "fuels": sorted(win_fuels),
        },
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "strata_score_delta": dict(sorted(strata_delta.items())),
        "safety": {
            side: {key: summed(side, key) for key in safety_keys}
            for side in ("off", "on")
        },
        "candidate_work": {key: summed("on", key) for key in telemetry},
        "default_off_work": {key: summed("off", key) for key in telemetry},
        "response_ms_local_non_authoritative": {
            side: max(
                int(record["parent_row" if side == "off" else "candidate_row"].get("max_ms", "0"))
                for record in records
            )
            for side in ("off", "on")
        },
        "gate": gate,
    }
    rendered = json.dumps(summary, indent=2, sort_keys=True) + "\n"
    if args.output:
        Path(args.output).write_text(rendered, encoding="utf-8")
    print(rendered, end="")


if __name__ == "__main__":
    main()
