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
    expected_cases = sum(int(row["count"]) for row in manifest)
    metadata: dict[tuple[str, int], dict[str, str]] = {}
    for row in manifest:
        for offset in range(int(row["count"])):
            key = row["suite"], int(row["first_seed"]) + offset
            if key in metadata:
                raise RuntimeError(f"duplicate manifest case {key}")
            metadata[key] = row

    results: dict[tuple[str, int, str], dict[str, str]] = {}
    day_rows: dict[tuple[str, int, str], list[dict[str, str]]] = defaultdict(list)
    completed: set[tuple[str, int]] = set()
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
            expected_version = "parent-265" if active_side == "off" else "candidate-265"
            if row.get("version") != expected_version:
                raise RuntimeError(f"version mismatch for {key}: {row.get('version')}")
            results[key] = row
        elif line.startswith("day_detail,"):
            if active_case is None or active_side is None:
                raise RuntimeError("day_detail outside side block")
            row = fields(line)
            key = active_case[0], active_case[1], active_side
            if int(row["seed"]) != active_case[1]:
                raise RuntimeError(f"day_detail mismatch for {key}")
            day_rows[key].append(row)
        elif line.startswith("side_complete,"):
            side = fields(line)["side"]
            if active_case is None or side != active_side:
                raise RuntimeError("side completion mismatch")
            key = active_case[0], active_case[1], side
            if key not in results:
                raise RuntimeError(f"side completed without result {key}")
            active_side = None
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active_case != key or active_side is not None:
                raise RuntimeError(f"case completion mismatch {key}")
            for side in ("off", "on"):
                if (key[0], key[1], side) not in results:
                    raise RuntimeError(f"missing paired result {key}/{side}")
            completed.add(key)
            active_case = None
        elif line.startswith("run_complete,"):
            run_complete = True

    if active_case is not None or active_side is not None:
        raise RuntimeError("unterminated case or side block")
    if not run_complete:
        raise RuntimeError("run_complete marker missing")
    if completed != set(metadata):
        raise RuntimeError("manifest/case_complete bijection failed")
    expected_result_keys = {
        (suite, seed, side)
        for suite, seed in metadata
        for side in ("off", "on")
    }
    if set(results) != expected_result_keys:
        raise RuntimeError("manifest/paired-result bijection failed")
    if len(completed) != expected_cases:
        raise RuntimeError(f"cases {len(completed)} != expected {expected_cases}")

    records = []
    for key in sorted(metadata):
        parent_row = results[key[0], key[1], "off"]
        candidate_row = results[key[0], key[1], "on"]
        parent = score(parent_row)
        candidate = score(candidate_row)
        delta = tuple(c - p for c, p in zip(candidate, parent))
        outcome = "win" if candidate > parent else "loss" if candidate < parent else "tie"
        records.append(
            {
                "key": key,
                "meta": metadata[key],
                "parent_row": parent_row,
                "candidate_row": candidate_row,
                "parent_days": day_rows[key[0], key[1], "off"],
                "candidate_days": day_rows[key[0], key[1], "on"],
                "parent": parent,
                "candidate": candidate,
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
    score_delta_strata: dict[str, list[int]] = defaultdict(lambda: [0, 0, 0])
    for record in records:
        meta = record["meta"]
        for axis, value in (
            ("suite", record["key"][0]),
            ("players", meta["players"]),
            ("fuel", meta["fuel_profile"]),
            ("role", meta["role_mode"]),
            ("window", meta["public_window_ms"]),
            ("horizon", str(len(record["candidate_days"]))),
        ):
            label = f"{axis}:{value}"
            strata[label][record["outcome"]] += 1
            for tier in range(3):
                score_delta_strata[label][tier] += record["delta"][tier]

    def summed(side: str, key: str) -> int:
        row_name = "parent_row" if side == "off" else "candidate_row"
        return sum(int(record[row_name].get(key, "0")) for record in records)

    safety_keys = sorted(
        {
            key
            for record in records
            for row_name in ("parent_row", "candidate_row")
            for key in record[row_name]
            if key in {"invalid", "emergency"}
            or "failure" in key
            or key.endswith("_invalid")
        }
    )
    telemetry_keys = (
        "orphan_rendezvous_groups",
        "orphan_rendezvous_bases",
        "orphan_rendezvous_plans",
        "orphan_rendezvous_valid",
        "orphan_rendezvous_strict",
        "orphan_rendezvous_candidates",
        "orphan_rendezvous_selections",
        "orphan_rendezvous_deadline_days",
    )
    default_off_failures = [
        f"{record['key'][0]}:{record['key'][1]}"
        for record in records
        if any(int(record["parent_row"].get(key, "0")) != 0 for key in telemetry_keys)
    ]
    takeover_records = [
        record
        for record in records
        if int(record["candidate_row"].get("orphan_rendezvous_selections", "0")) > 0
    ]
    takeover_strata = {
        (
            record["key"][0],
            record["meta"]["fuel_profile"],
            record["meta"]["role_mode"],
            record["meta"]["public_window_ms"],
        )
        for record in takeover_records
    }
    takeover_windows = {record["meta"]["public_window_ms"] for record in takeover_records}
    takeover_roles = {record["meta"]["role_mode"] for record in takeover_records}
    takeover_fuels = {record["meta"]["fuel_profile"] for record in takeover_records}
    takeover_suites = {record["key"][0] for record in takeover_records}

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
    zero_safety = all(
        summed(side, key) == 0
        for side in ("off", "on")
        for key in safety_keys
    )
    takeover_count = summed("on", "orphan_rendezvous_selections")
    gate = {
        "default_off_source_path_inert": not default_off_failures,
        "zero_safety_failures": zero_safety,
        "zero_tier1_or_tier2_losses": not tier12_losses,
        "paired_wins_exceed_losses": outcomes["win"] > outcomes["loss"],
        "positive_aggregate_tier3_delta": aggregate[2] > 0,
        "worst_tier3_loss_at_least_minus_5": min(tier3_losses, default=0) >= -5,
        "at_least_two_strict_certified_takeovers": takeover_count >= 2,
        "takeovers_span_two_public_strata": len(takeover_strata) >= 2,
        "every_selected_takeover_has_exact_valid_candidate":
            summed("on", "orphan_rendezvous_valid")
            >= summed("on", "orphan_rendezvous_candidates")
            >= takeover_count
            and summed("on", "orphan_rendezvous_strict") >= takeover_count,
    }
    if args.phase == "holdout":
        gaining_strata = {
            label
            for label, delta in score_delta_strata.items()
            if tuple(delta) > (0, 0, 0)
        }
        gate["gains_span_multiple_reported_strata"] = len(gaining_strata) >= 2
    gate["passed"] = all(gate.values())

    summary = {
        "experiment": "SCORE-ORPHAN-RENDEZVOUS-PAIR-SIDECAR-265",
        "phase": args.phase,
        "cases": len(records),
        "paired_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "gain_tail": {"sum": sum(gains), "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "tier12_loss_cases": tier12_losses,
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "strata_score_delta": dict(sorted(score_delta_strata.items())),
        "safety": {
            side: {key: summed(side, key) for key in safety_keys}
            for side in ("off", "on")
        },
        "sidecar_work": {
            key: summed("on", key) for key in telemetry_keys
        },
        "default_off_telemetry_failures": default_off_failures,
        "takeover_breadth": {
            "matches": len(takeover_records),
            "selections": takeover_count,
            "public_strata": len(takeover_strata),
            "windows": sorted(takeover_windows),
            "roles": sorted(takeover_roles),
            "fuels": sorted(takeover_fuels),
            "suites": sorted(takeover_suites),
        },
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
