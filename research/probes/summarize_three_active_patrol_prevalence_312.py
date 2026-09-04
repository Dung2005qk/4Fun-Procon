#!/usr/bin/env python3
import argparse
import hashlib
import json
from collections import Counter, defaultdict
from pathlib import Path


def parse_fields(line: str) -> dict[str, str]:
    fields: dict[str, str] = {}
    for item in line.rstrip().split(",")[1:]:
        key, value = item.split("=", 1)
        fields[key] = value
    return fields


def score(text: str) -> tuple[int, int, int]:
    values = tuple(int(value) for value in text.split("/"))
    if len(values) != 3:
        raise ValueError(f"invalid official score: {text}")
    return values


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("log", type=Path)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()

    raw = args.log.read_bytes()
    lines = raw.decode("utf-8-sig").splitlines()
    cases = [parse_fields(line) for line in lines if line.startswith("case,")]
    completed = [
        parse_fields(line) for line in lines if line.startswith("case_complete,")
    ]
    run_complete = [line for line in lines if line == "run_complete,cases=12"]
    case_seeds = sorted(int(case["seed"]) for case in cases)
    completed_seeds = sorted(int(case["seed"]) for case in completed)
    if (
        len(cases) != 12
        or len(completed) != 12
        or len(run_complete) != 1
        or case_seeds != completed_seeds
        or len(set(case_seeds)) != 12
    ):
        raise SystemExit(
            "expected 12 atomic cases, matching case_complete markers and one "
            f"run_complete; got cases={len(cases)} completed={len(completed)} "
            f"run_complete={len(run_complete)}"
        )

    invalid = [
        case
        for case in cases
        if case["oracle_valid"] != "1" or case["head_valid"] != "1"
    ]
    outcomes = Counter()
    tiers = Counter()
    families: dict[str, Counter] = defaultdict(Counter)
    players: dict[str, Counter] = defaultdict(Counter)
    wins: list[dict[str, object]] = []
    maximum_frontier = 0
    for case in cases:
        oracle = score(case["oracle"])
        head = score(case["head"])
        outcome = "win" if oracle > head else ("loss" if oracle < head else "tie")
        outcomes[outcome] += 1
        families[case["family"]][outcome] += 1
        players[case["players"]][outcome] += 1
        maximum_frontier = max(maximum_frontier, int(case["max_frontier"]))
        if outcome == "win":
            tier = int(case["tier"])
            tiers[str(tier)] += 1
            wins.append(
                {
                    "seed": int(case["seed"]),
                    "family": case["family"],
                    "players": int(case["players"]),
                    "oracle": oracle,
                    "head": head,
                    "first_tier": tier,
                    "gain": int(case["gain"]),
                }
            )

    winning_families = sorted(
        family for family, counts in families.items() if counts["win"] > 0
    )
    recurrent = (
        not invalid
        and outcomes["loss"] == 0
        and outcomes["win"] >= 3
        and len(winning_families) >= 2
    )
    report = {
        "experiment": "CEILING-THREE-ACTIVE-PATROL-LOW-FUEL-PREVALENCE-312",
        "phase": "development",
        "log_sha256": hashlib.sha256(raw).hexdigest().upper(),
        "cases": len(cases),
        "oracle_head_wtl": {
            "wins": outcomes["win"],
            "ties": outcomes["tie"],
            "losses": outcomes["loss"],
        },
        "first_open_tiers": dict(sorted(tiers.items())),
        "maximum_exact_frontier": maximum_frontier,
        "winning_families": winning_families,
        "wins": wins,
        "strata": {
            "family": {key: dict(value) for key, value in sorted(families.items())},
            "players": {key: dict(value) for key, value in sorted(players.items())},
        },
        "zero_invalid": not invalid,
        "recurrent_exact_gap": recurrent,
        "causal_attribution_authorized": recurrent,
        "score_successor_authorized": False,
        "holdout_open_authorized": False,
        "verdict": (
            "recurrent-fresh-exact-gap-attribution-only"
            if recurrent
            else "closed-no-recurrent-fresh-exact-gap"
        ),
    }
    args.output.write_text(
        json.dumps(report, indent=2, sort_keys=True) + "\n", encoding="utf-8"
    )
    print(json.dumps(report, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
