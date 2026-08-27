#!/usr/bin/env python3
from __future__ import annotations

import argparse
import collections
import json
import pathlib
import re


DAY_RE = re.compile(
    r"^day=(?P<day>\d+) .* daily=(?P<daily>\d+)/(?P<servings>\d+) "
    r".* terminals=(?P<terminals>[^ ]+)"
)
CACHE_RE = re.compile(
    r"^cached_contingency day=(?P<day>\d+) scenario=(?P<scenario>-?\d+) "
    r"valid=(?P<valid>[01]) daily=(?P<daily>\d+) servings=(?P<servings>\d+)"
)
SUMMARY_RE = re.compile(r"^summary .* score=(?P<score>\d+/\d+/\d+)$")
TERMINAL_RE = re.compile(r"a\d+@(?P<cell>-?\d+)/(?P<fuel>-?\d+)")


def parse_run(path: pathlib.Path) -> dict:
    days: dict[int, dict] = {}
    caches: dict[int, list[dict]] = collections.defaultdict(list)
    score = None
    for raw_line in path.read_text(encoding="utf-8").splitlines():
        line = raw_line.strip()
        match = DAY_RE.match(line)
        if match:
            terminals = [
                (int(item.group("cell")), int(item.group("fuel")))
                for item in TERMINAL_RE.finditer(match.group("terminals"))
            ]
            days[int(match.group("day"))] = {
                "daily": int(match.group("daily")),
                "servings": int(match.group("servings")),
                "terminals": terminals,
            }
            continue
        match = CACHE_RE.match(line)
        if match:
            caches[int(match.group("day"))].append(
                {
                    "scenario": int(match.group("scenario")),
                    "valid": match.group("valid") == "1",
                    "daily": int(match.group("daily")),
                    "servings": int(match.group("servings")),
                }
            )
            continue
        match = SUMMARY_RE.match(line)
        if match:
            score = match.group("score")
    if score is None or len(days) != 5:
        raise ValueError(f"incomplete run: {path}")
    day3_cells = [cell for cell, _ in days[3]["terminals"]]
    clustered_day3 = bool(day3_cells) and len(set(day3_cells)) == 1
    day4_cache = caches.get(4, [])
    return {
        "path": path.name,
        "score": score,
        "clustered_day3": clustered_day3,
        "day4_daily": days[4]["daily"],
        "day4_servings": days[4]["servings"],
        "day4_cache_count": len(day4_cache),
        "day4_full_daily_cache": any(
            entry["valid"] and entry["daily"] >= 8 for entry in day4_cache
        ),
        "cache_preview_count": sum(len(entries) for entries in caches.values()),
        "invalid_cache_previews": sum(
            1 for entries in caches.values() for entry in entries if not entry["valid"]
        ),
    }


def summarize_lane(paths: list[pathlib.Path]) -> dict:
    runs = [parse_run(path) for path in sorted(paths)]
    clustered = [run for run in runs if run["clustered_day3"]]
    return {
        "runs": len(runs),
        "scores": dict(collections.Counter(run["score"] for run in runs)),
        "cache_preview_count": sum(run["cache_preview_count"] for run in runs),
        "invalid_cache_previews": sum(run["invalid_cache_previews"] for run in runs),
        "clustered_day3": len(clustered),
        "clustered_day4_dips": sum(run["day4_daily"] < 8 for run in clustered),
        "clustered_with_full_daily_cache": sum(
            run["day4_full_daily_cache"] for run in clustered
        ),
        "clustered_full_cache_but_dip": sum(
            run["day4_full_daily_cache"] and run["day4_daily"] < 8
            for run in clustered
        ),
        "clustered_without_cache_and_dip": sum(
            not run["day4_full_daily_cache"] and run["day4_daily"] < 8
            for run in clustered
        ),
        "run_details": runs,
    }


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("logs", type=pathlib.Path)
    args = parser.parse_args()
    result = {}
    for lane in ("no-ack-control", "buggy-226", "cache-faithful"):
        result[lane] = summarize_lane(list(args.logs.glob(f"{lane}-*.log")))
    print(json.dumps(result, indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
