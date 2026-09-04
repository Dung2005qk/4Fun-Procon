#!/usr/bin/env python3
import argparse
import csv
import hashlib
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


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--log", required=True)
    parser.add_argument("--phase", choices=("development", "holdout"), required=True)
    parser.add_argument("--output")
    args = parser.parse_args()

    manifest_path = Path(args.manifest)
    log_path = Path(args.log)
    manifest = [
        row
        for row in csv.DictReader(manifest_path.open(encoding="utf-8-sig"))
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
    for line in log_path.read_text(encoding="utf-8-sig").splitlines():
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
            expected_version = "parent-311" if active_side == "off" else "candidate-311"
            expected_bound = "1" if active_side == "off" else "4"
            if row.get("version") != expected_version:
                raise RuntimeError(f"version mismatch for {key}: {row.get('version')}")
            if row.get("coordinated_exact_bundles") != expected_bound:
                raise RuntimeError(f"bundle-bound mismatch for {key}")
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
            if (active_case[0], active_case[1], side) not in results:
                raise RuntimeError("side completed without result")
            active_side = None
        elif line.startswith("case_complete,"):
            row = fields(line)
            key = row["suite"], int(row["seed"])
            if active_case != key or active_side is not None:
                raise RuntimeError(f"case completion mismatch {key}")
            if any((key[0], key[1], side) not in results for side in ("off", "on")):
                raise RuntimeError(f"missing paired result {key}")
            completed.add(key)
            active_case = None
        elif line.startswith("run_complete,"):
            run_complete = True

    if active_case is not None or active_side is not None or not run_complete:
        raise RuntimeError("incomplete atomic run")
    if completed != set(metadata):
        raise RuntimeError("manifest/case_complete bijection failed")
    expected_results = {
        (suite, seed, side)
        for suite, seed in metadata
        for side in ("off", "on")
    }
    if set(results) != expected_results or len(completed) != expected_cases:
        raise RuntimeError("manifest/paired-result bijection failed")

    records = []
    required_day_fields = {
        "day",
        "cumulative",
        "plan_hash",
        "state_hash",
        "ledger_hash",
        "exact_frontier_candidates",
        "exact_frontier_bundles",
        "response_ms",
    }
    for key in sorted(metadata):
        parent_row = results[key[0], key[1], "off"]
        candidate_row = results[key[0], key[1], "on"]
        parent_days = day_rows[key[0], key[1], "off"]
        candidate_days = day_rows[key[0], key[1], "on"]
        if len(parent_days) != len(candidate_days) or not 4 <= len(parent_days) <= 10:
            raise RuntimeError(f"day-count mismatch for {key}")
        for side, days in (("off", parent_days), ("on", candidate_days)):
            if [int(day["day"]) for day in days] != list(range(1, len(days) + 1)):
                raise RuntimeError(f"invalid day sequence for {key}/{side}")
            for day in days:
                missing = required_day_fields - set(day)
                if missing:
                    raise RuntimeError(f"missing day fields for {key}/{side}: {missing}")
        parent = score(parent_row)
        candidate = score(candidate_row)
        delta = tuple(c - p for c, p in zip(candidate, parent))
        outcome = "win" if candidate > parent else "loss" if candidate < parent else "tie"
        activation_days = [
            index
            for index, day in enumerate(candidate_days)
            if int(day["exact_frontier_bundles"]) > 0
        ]
        first_activation = min(activation_days, default=len(candidate_days))
        prefix_equivalent = all(
            all(
                parent_days[index][field] == candidate_days[index][field]
                for field in ("cumulative", "plan_hash", "state_hash", "ledger_hash")
            )
            for index in range(first_activation)
        )
        records.append(
            {
                "key": key,
                "meta": metadata[key],
                "parent_row": parent_row,
                "candidate_row": candidate_row,
                "parent_days": parent_days,
                "candidate_days": candidate_days,
                "parent": parent,
                "candidate": candidate,
                "delta": delta,
                "outcome": outcome,
                "activation_days": activation_days,
                "prefix_equivalent": prefix_equivalent,
            }
        )

    outcomes = Counter(record["outcome"] for record in records)
    tiers = Counter(first_tier(record["delta"]) for record in records)
    aggregate = [sum(record["delta"][tier] for record in records) for tier in range(3)]
    deciding_delta_by_tier = {
        str(tier): sum(
            record["delta"][tier - 1]
            for record in records
            if first_tier(record["delta"]) == tier
        )
        for tier in (1, 2, 3)
    }
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
    strata_delta: dict[str, list[int]] = defaultdict(lambda: [0, 0, 0])
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
                strata_delta[label][tier] += record["delta"][tier]

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
    off_frontier_failures = [
        f"{record['key'][0]}:{record['key'][1]}"
        for record in records
        if int(record["parent_row"].get("exact_frontier_candidates", "0")) != 0
        or int(record["parent_row"].get("exact_frontier_bundles", "0")) != 0
    ]
    equivalence_failures = [
        f"{record['key'][0]}:{record['key'][1]}"
        for record in records
        if not record["prefix_equivalent"]
    ]
    inactive_case_failures = [
        f"{record['key'][0]}:{record['key'][1]}"
        for record in records
        if not record["activation_days"]
        and (
            record["parent"] != record["candidate"]
            or not record["prefix_equivalent"]
        )
    ]
    activated = [record for record in records if record["activation_days"]]
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
    loss_concentration = [
        label
        for label, counts in strata.items()
        if counts["loss"] >= 2 and counts["win"] == 0
    ]
    gain_records = [record for record in records if record["outcome"] == "win"]
    gain_breadth = {
        "suites": sorted({record["key"][0] for record in gain_records}),
        "windows": sorted({record["meta"]["public_window_ms"] for record in gain_records}),
        "fuels": sorted({record["meta"]["fuel_profile"] for record in gain_records}),
        "roles": sorted({record["meta"]["role_mode"] for record in gain_records}),
    }
    gains_span_two_strata = any(len(values) >= 2 for values in gain_breadth.values())
    deadline_failures = [
        f"{record['key'][0]}:{record['key'][1]}:{side}"
        for record in records
        for side, days in (
            ("off", record["parent_days"]),
            ("on", record["candidate_days"]),
        )
        if max(int(day["response_ms"]) for day in days) > 5000
    ]
    zero_safety = all(
        summed(side, key) == 0
        for side in ("off", "on")
        for key in safety_keys
    )
    positive_deciding_tiers = all(
        value > 0
        for tier, value in deciding_delta_by_tier.items()
        if tiers[int(tier)] > 0
    )
    gate = {
        "default_off_frontier_inert": not off_frontier_failures,
        "zero_safety_failures": zero_safety,
        "all_responses_within_5000_ms": not deadline_failures,
        "exact_pre_activation_and_inactive_equivalence":
            not equivalence_failures and not inactive_case_failures,
        "zero_tier1_or_tier2_losses": not tier12_losses,
        "paired_wins_exceed_losses": outcomes["win"] > outcomes["loss"],
        "positive_delta_at_every_deciding_tier": positive_deciding_tiers,
        "worst_tier3_loss_at_least_minus_2": min(tier3_losses, default=0) >= -2,
        "no_systematic_loss_concentration": not loss_concentration,
        "recurrent_frontier_activation": len(activated) >= 2,
        "gains_span_two_public_strata": gains_span_two_strata,
    }
    gate["passed"] = all(gate.values())

    summary = {
        "experiment": "SCORE-COORDINATED-EXACT-BUNDLE-FRONTIER-311",
        "phase": args.phase,
        "manifest_sha256": sha256(manifest_path),
        "log_sha256": sha256(log_path),
        "cases": len(records),
        "paired_wtl": [outcomes["win"], outcomes["tie"], outcomes["loss"]],
        "first_difference_tiers": dict(sorted(tiers.items())),
        "aggregate_score_delta": aggregate,
        "deciding_delta_by_tier": deciding_delta_by_tier,
        "gain_tail": {"sum": sum(gains), "max": max(gains, default=0)},
        "loss_tail": {"sum": sum(losses), "min": min(losses, default=0)},
        "tier12_loss_cases": tier12_losses,
        "loss_concentration": loss_concentration,
        "strata": {key: dict(value) for key, value in sorted(strata.items())},
        "strata_score_delta": dict(sorted(strata_delta.items())),
        "safety": {
            side: {key: summed(side, key) for key in safety_keys}
            for side in ("off", "on")
        },
        "frontier_work": {
            "off_candidates": summed("off", "exact_frontier_candidates"),
            "off_bundles": summed("off", "exact_frontier_bundles"),
            "on_candidates": summed("on", "exact_frontier_candidates"),
            "on_bundles": summed("on", "exact_frontier_bundles"),
            "activated_matches": len(activated),
            "activated_days": sum(len(record["activation_days"]) for record in activated),
        },
        "gain_breadth": gain_breadth,
        "default_off_frontier_failures": off_frontier_failures,
        "equivalence_failures": equivalence_failures,
        "inactive_case_failures": inactive_case_failures,
        "deadline_failures": deadline_failures,
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
