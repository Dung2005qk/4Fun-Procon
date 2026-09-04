#!/usr/bin/env python3
"""Audit consumed 286/287 evidence for an exact service-event-set successor.

This probe is attribution-only.  It never runs a solver, opens a holdout, or
changes a production plan.  It verifies the frozen evidence, classifies the
complete paired outcomes by selected multi-waypoint topology, and extracts the
historical source patch calls that define the two rejected mechanisms.
"""

from __future__ import annotations

import argparse
import csv
import hashlib
import json
import re
from collections import defaultdict
from pathlib import Path
from typing import Any


EXPECTED_MANIFEST_SHA256 = (
    "75FAD77BE4960F7218456404BDDB86CEF2F695D537934AA2C26C2E01C03A0908"
)

# Exact response-item line numbers and call ids from the durable Codex session.
# They contain only the source patches that created 286 and refined it into 287.
SOURCE_CALLS = {
    286: {
        122833: "call_VeaRr3PWDc6TpUARnOpUYjgG",
        122841: "call_Ex71lFJwwKLmVe60uG7wavOe",
        122846: "call_uxiTtcKLL0JejZ2nQxayusWi",
        122856: "call_96mQbgbariFqnWHINIBOy70f",
        122861: "call_FkBCe5Bf9yRyOjk3AStwqJNn",
        122866: "call_ldbUFMf8uTo25ai2aegeEUdl",
        122872: "call_xmJRVyzYqdHbO06ADuhUjYPx",
    },
    287: {
        123293: "call_AwmvBNFYotXcqwREVvuutr1x",
        123298: "call_gTMcwdu48emVqo6VIVFXteHj",
        123303: "call_bqorXwV6Marv2R5iut9feQaK",
        123313: "call_l2mNS2NpcuIpxuuYUPHDiWcQ",
        123318: "call_tnB5gVCjp8xW683V6d3SOwae",
        123323: "call_B8eUU7arltcUnFnkUWRhQAsG",
        123333: "call_vcpPTQLjmpmoBXqw6ZbO6UlA",
        123342: "call_G6zdAcMZIirZ53WZiSgDJV90",
        123352: "call_yLrNefavf3CIk95rXDSaVAAh",
        123365: "call_uRSPhqsw8CYE2NIEl13ODHcJ",
    },
}


def sha256(path: Path) -> str:
    digest = hashlib.sha256()
    with path.open("rb") as stream:
        for block in iter(lambda: stream.read(1024 * 1024), b""):
            digest.update(block)
    return digest.hexdigest().upper()


def parse_fields(line: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for item in line.rstrip("\n").split(",")[1:]:
        key, separator, value = item.partition("=")
        if separator:
            fields[key] = value
    return fields


def read_results(path: Path, expected_cases: int, expected_results: int) -> list[dict[str, str]]:
    lines = path.read_text(encoding="utf-8-sig").splitlines()
    results = [parse_fields(line) for line in lines if line.startswith("result,")]
    completes = [line for line in lines if line.startswith("case_complete,")]
    run_complete = [line for line in lines if line.startswith("run_complete,")]
    if len(results) != expected_results or len(completes) != expected_cases or len(run_complete) != 1:
        raise ValueError(
            f"atomic count mismatch in {path}: results={len(results)}, "
            f"cases={len(completes)}, run_complete={len(run_complete)}"
        )
    by_seed: dict[str, int] = defaultdict(int)
    for result in results:
        by_seed[result["seed"]] += 1
    if len(by_seed) != expected_cases or any(count != 2 for count in by_seed.values()):
        raise ValueError(f"paired result bijection failed in {path}")
    return results


def official_compare(candidate: dict[str, str], parent: dict[str, str]) -> tuple[str, int, int]:
    for tier, key in enumerate(("lifetime", "daily", "servings"), start=1):
        delta = int(candidate[key]) - int(parent[key])
        if delta > 0:
            return "win", tier, delta
        if delta < 0:
            return "loss", tier, delta
    return "tie", 0, 0


def selection_class(result: dict[str, str]) -> str:
    providers = int(result["multi_waypoint_selected_providers"])
    consumers = int(result["multi_waypoint_selected_consumers"])
    if providers > 0 and consumers > 0:
        return "matched-provider-consumer"
    if providers > 0:
        return "orphan-provider"
    if consumers > 0:
        return "consumer-only"
    return "inactive"


def classify_pairs(results: list[dict[str, str]], suffix: str) -> dict[str, Any]:
    grouped: dict[str, list[dict[str, str]]] = defaultdict(list)
    for result in results:
        grouped[result["seed"]].append(result)
    classes: dict[str, dict[str, Any]] = defaultdict(
        lambda: {
            "wtl": {"win": 0, "tie": 0, "loss": 0},
            "first_difference_gain_mass": 0,
            "first_difference_loss_mass": 0,
            "aggregate_delta": [0, 0, 0],
            "seeds": [],
            "suites": set(),
            "fuels": set(),
            "players": set(),
        }
    )
    paired: list[dict[str, Any]] = []
    for seed, pair in sorted(grouped.items(), key=lambda item: int(item[0])):
        parent = next(row for row in pair if row["version"].startswith("parent-"))
        candidate = next(row for row in pair if row["version"].startswith("candidate-"))
        outcome, tier, first_delta = official_compare(candidate, parent)
        category = selection_class(candidate)
        deltas = [
            int(candidate[key]) - int(parent[key])
            for key in ("lifetime", "daily", "servings")
        ]
        record = classes[category]
        record["wtl"][outcome] += 1
        record["seeds"].append(int(seed))
        record["suites"].add(candidate["suite"])
        record["fuels"].add(candidate["fuel_profile"])
        record["players"].add(int(candidate["players"]))
        for index, delta in enumerate(deltas):
            record["aggregate_delta"][index] += delta
        if outcome == "win":
            record["first_difference_gain_mass"] += first_delta
        elif outcome == "loss":
            record["first_difference_loss_mass"] += -first_delta
        paired.append(
            {
                "experiment": suffix,
                "seed": int(seed),
                "class": category,
                "outcome": outcome,
                "first_difference_tier": tier,
                "first_difference_delta": first_delta,
                "aggregate_delta": deltas,
                "selected_providers": int(candidate["multi_waypoint_selected_providers"]),
                "selected_consumers": int(candidate["multi_waypoint_selected_consumers"]),
                "atomicity_failures": int(candidate.get("multi_waypoint_atomicity_failures", "0")),
                "suite": candidate["suite"],
                "fuel": candidate["fuel_profile"],
                "players": int(candidate["players"]),
                "role_mask": int(candidate["role_mask"]),
            }
        )
    normalized: dict[str, Any] = {}
    for category, values in classes.items():
        normalized[category] = dict(values)
        for key in ("suites", "fuels", "players"):
            normalized[category][key] = sorted(values[key])
    return {"classes": normalized, "pairs": paired}


def extract_source_patches(session: Path) -> tuple[dict[int, str], str]:
    wanted = {line: call for calls in SOURCE_CALLS.values() for line, call in calls.items()}
    found: dict[int, str] = {}
    patch_expression = re.compile(r'const patch = ("(?:\\.|[^"\\])*");', re.DOTALL)
    with session.open("r", encoding="utf-8") as stream:
        for line_number, line in enumerate(stream, start=1):
            if line_number not in wanted:
                continue
            record = json.loads(line)
            payload = record.get("payload", {})
            if payload.get("call_id") != wanted[line_number]:
                raise ValueError(f"source call mismatch at session line {line_number}")
            match = patch_expression.search(payload.get("input", ""))
            if match is None:
                raise ValueError(f"missing patch literal at session line {line_number}")
            found[line_number] = json.loads(match.group(1))
    if set(found) != set(wanted):
        raise ValueError(f"missing source calls: {sorted(set(wanted) - set(found))}")
    combined_parts = []
    for experiment in (286, 287):
        for line_number in SOURCE_CALLS[experiment]:
            combined_parts.append(
                f"# experiment={experiment} session_line={line_number} "
                f"call_id={SOURCE_CALLS[experiment][line_number]}\n"
            )
            combined_parts.append(found[line_number].rstrip() + "\n")
    combined = "".join(combined_parts)
    return found, combined


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--session", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--source-patch-output", type=Path, required=True)
    args = parser.parse_args()

    if sha256(args.manifest) != EXPECTED_MANIFEST_SHA256:
        raise ValueError("frozen attribution manifest hash mismatch")
    with args.manifest.open(newline="", encoding="utf-8-sig") as stream:
        rows = list(csv.DictReader(stream))
    if len(rows) != 4:
        raise ValueError(f"expected four frozen artifacts, got {len(rows)}")

    logs: dict[str, list[dict[str, str]]] = {}
    verified_artifacts: list[dict[str, Any]] = []
    for row in rows:
        path = Path(row["path"])
        actual = sha256(path)
        if actual != row["sha256"]:
            raise ValueError(f"artifact hash mismatch for {path}: {actual}")
        verified_artifacts.append({"path": str(path), "sha256": actual})
        if row["artifact_kind"] == "complete-development-log":
            logs[row["source_experiment"]] = read_results(
                path, int(row["expected_cases"]), int(row["expected_results"])
            )

    evidence286 = classify_pairs(
        logs["SCORE-MULTI-WAYPOINT-TANKER-PROVIDER-286"], "286"
    )
    evidence287 = classify_pairs(
        logs["SCORE-ATOMIC-MULTI-WAYPOINT-TANKER-GROUP-287"], "287"
    )
    source_patches, combined_source = extract_source_patches(args.session)
    args.source_patch_output.parent.mkdir(parents=True, exist_ok=True)
    args.source_patch_output.write_text(combined_source, encoding="utf-8", newline="\n")
    source_sha = sha256(args.source_patch_output)

    source286 = "\n".join(source_patches[line] for line in SOURCE_CALLS[286])
    source287 = "\n".join(source_patches[line] for line in SOURCE_CALLS[287])
    source_facts = {
        "286_enumerates_ordered_distinct_patrol_pairs": (
            "secondPatrol == firstPatrol" in source286
            and "providerPairs.push_back" in source286
        ),
        "286_builds_two_exact_consumer_event_sets": (
            "firstConsumers" in source286
            and "secondConsumers" in source286
            and "requiredRefuels.push_back" in source286
        ),
        "286_has_no_service_group_identity": "multiWaypointServiceGroup" not in source286,
        "287_requires_exactly_three_agents": "std::popcount(group.requiredMask) != 3" in source287,
        "287_requires_one_provider_two_consumers": (
            "group.providers != 1" in source287
            and "group.consumers != 2" in source287
        ),
        "287_requires_full_fixed_group_mask": "group.selectedMask != group.requiredMask" in source287,
        "287_has_no_two_agent_service_group": (
            "std::popcount(group.requiredMask) != 2" not in source287
        ),
    }

    matched = evidence286["classes"].get("matched-provider-consumer", {})
    tail_losses = [
        row for row in evidence286["pairs"]
        if row["outcome"] == "loss" and row["first_difference_delta"] < -4
    ]
    selected_groups287 = sum(
        row["selected_providers"] for row in evidence287["pairs"]
    )
    atomic_failures287 = sum(
        row["atomicity_failures"] for row in evidence287["pairs"]
    )
    gate = {
        "all_frozen_artifacts_verified": len(verified_artifacts) == 4,
        "all_286_losses_worse_than_four_are_orphan_providers": (
            bool(tail_losses)
            and all(row["class"] == "orphan-provider" for row in tail_losses)
        ),
        "matched_286_selection_has_no_loss": (
            matched.get("wtl", {}).get("loss", 0) == 0
        ),
        "matched_286_selection_has_positive_gain_mass": (
            matched.get("first_difference_gain_mass", 0) > 0
        ),
        "matched_286_selection_spans_two_suites": len(matched.get("suites", [])) >= 2,
        "matched_286_selection_spans_two_fuels": len(matched.get("fuels", [])) >= 2,
        "fixed_287_group_is_unsafe_or_too_inert": (
            atomic_failures287 > 0 or selected_groups287 < 4
        ),
        "source_proves_distinct_event_set_family": all(source_facts.values()),
    }
    gate["passed"] = all(gate.values())

    report = {
        "experiment": "ATTR-MULTI-WAYPOINT-SERVICE-PREFIX-CLOSURE-294",
        "authority": "consumed-attribution-only",
        "manifest_sha256": EXPECTED_MANIFEST_SHA256,
        "verified_artifacts": verified_artifacts,
        "source_patch": {
            "path": str(args.source_patch_output),
            "sha256": source_sha,
            "calls": SOURCE_CALLS,
            "facts": source_facts,
        },
        "evidence_286": evidence286,
        "evidence_287": evidence287,
        "tail_losses_286": tail_losses,
        "selected_complete_groups_287": selected_groups287,
        "atomicity_failures_287": atomic_failures287,
        "gate": gate,
        "verdict": (
            "qualified-for-separate-fresh-score-successor"
            if gate["passed"] else "closed-no-score-successor"
        ),
        "successor_invariant": (
            "Generate independently exact and revalidated one-event and two-event "
            "service groups before master selection. Each group owns immutable "
            "provider actions, event times, terminal state and road footprint; "
            "never truncate or rewrite a selected provider route."
        ),
        "limitations": [
            "286/287 cases are consumed and cannot promote or tune a successor.",
            "Selection counters are case aggregates, not per-event causal telemetry.",
            "The recovered source audit authorizes a fresh test; it does not prove score benefit.",
        ],
    }
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({
        "passed": gate["passed"],
        "verdict": report["verdict"],
        "matched_286_wtl": matched.get("wtl", {}),
        "matched_286_gain_mass": matched.get("first_difference_gain_mass", 0),
        "tail_losses_286": len(tail_losses),
        "selected_complete_groups_287": selected_groups287,
        "atomicity_failures_287": atomic_failures287,
        "source_patch_sha256": source_sha,
    }, sort_keys=True))


if __name__ == "__main__":
    main()
