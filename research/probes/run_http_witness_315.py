"""Recover consumed oracle witnesses; never rerun the HTTP baseline or a holdout."""
import argparse
import csv
import hashlib
import json
import os
from pathlib import Path
import subprocess

EXPERIMENT = "ATTR-THREE-PATROL-HTTP-WITNESS-315"
SEEDS = (10700000, 10700001, 10700100, 10700101, 10700500, 10700501)


def require(condition, message):
    if not condition:
        raise ValueError(message)


def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()


def fields(line):
    return dict(item.split("=", 1) for item in line.split(",")[1:])


def score(text):
    values = [int(value) for value in text.split("/")]
    require(len(values) == 3 and min(values) >= 0, "invalid score tuple")
    return values


def parse_case(raw, spec):
    lines = [line for line in raw.splitlines() if line]
    cases = [fields(line) for line in lines if line.startswith("case,")]
    summaries = [fields(line) for line in lines if line.startswith("summary,")]
    require(len(cases) == len(summaries) == 1, "missing/duplicate case or summary")
    case, summary = cases[0], summaries[0]
    require(int(case["seed"]) == spec["seed"] and case["family"] == spec["family"]
            and int(case["players"]) == spec["players"] and case["fuel"] == "low"
            and case["agents"] == "3" and case["horizon"] == "4", "wrong fixture identity")
    require(case["oracle_valid"] == case["head_valid"] == "1"
            and summary["cases"] == "1" and summary["invalid"] == "0"
            and summary["head_wins"] == "0", "oracle run failed validation")
    require(score(case["oracle"]) == spec["oracle_score"], "oracle score changed versus frozen312")
    traces = []
    for line in lines:
        if not line.startswith("trace,"):
            require(line.startswith(("case,", "summary,", "stratum,")), "unexpected oracle output")
            continue
        prefix, plans = line.split(",oracle_id=", 1)
        oracle_plan, _incidental_head_plan = plans.split(",head_id=", 1)
        metadata = fields(prefix)
        plan = json.loads(oracle_plan)
        require(int(metadata["seed"]) == spec["seed"], "trace seed mismatch")
        require(isinstance(plan, list) and len(plan) == 3 and all(
            isinstance(route, list) and all(type(action) is int for action in route)
            for route in plan), "malformed oracle plan")
        traces.append({"day": int(metadata["day"]), "plan": plan,
                       "score": score(metadata["oracle_score"])})
    require([trace["day"] for trace in traces] == [1, 2, 3, 4], "incomplete/duplicate day traces")
    require(traces[-1]["score"] == spec["oracle_score"], "final trace score mismatch")
    return traces


def verify_inputs(manifest, directory):
    require(manifest["experiment"] == EXPERIMENT and
            tuple(spec["seed"] for spec in manifest["cases"]) == SEEDS, "wrong315 manifest")
    for name, expected in manifest["deployment_hashes"].items():
        require(digest(directory / name) == expected, "deployed hash mismatch: " + name)
    for role in ("oracle_binary", "oracle_source"):
        item = manifest[role]
        require(digest(item["path"]) == item["sha256"], "frozen hash mismatch: " + role)
    with (directory / manifest["adapter"]).open(newline="") as stream:
        rows = list(csv.DictReader(stream))
    require(len(rows) == 6, "adapter row count")
    for row, spec in zip(rows, manifest["cases"], strict=True):
        require(row == {"experiment_id": "CEILING-THREE-ACTIVE-PATROL-LOW-FUEL-PREVALENCE-312",
            "split": "development", "family": spec["family"], "fuel_profile": "low",
            "players": str(spec["players"]), "horizon": "4", "first_seed": str(spec["seed"]),
            "count": "1", "active_agents": "3", "total_agents": "3", "spot_count": "5",
            "role_mode": "all-patrol", "oracle_scope":
            "complete-three-active-patrol-roadless-full-match-dp"}, "adapter identity mismatch")


def main():
    import fcntl  # VM runner only; parser is also tested on the Windows host.
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--manifest-sha256", required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    require(digest(args.manifest) == args.manifest_sha256.upper(), "manifest hash mismatch")
    manifest = json.loads(args.manifest.read_text())
    base = args.manifest.resolve().parent
    verify_inputs(manifest, base)
    args.output.mkdir(exist_ok=True)
    # Kernel lock automatically releases after exit/preemption; never remove a
    # stale PID file to guess whether another runner still owns this directory.
    with (args.output / "runner.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        (args.output / "runner.pid").write_text(str(os.getpid()) + "\n")
        completed = args.output / "run_complete.json"
        if completed.exists():
            marker = json.loads(completed.read_text())
            require(marker["manifest_sha256"] == args.manifest_sha256.upper(), "existing completion mismatch")
            require(digest(args.output / "witnesses.log") == marker["log_sha256"], "completed log changed")
            print("already complete; nothing rerun", flush=True)
            return
        for spec in manifest["cases"]:
            seed = str(spec["seed"])
            result = args.output / (seed + ".result")
            stderr = args.output / (seed + ".stderr")
            partial = args.output / (seed + ".partial")
            if result.exists():
                parse_case(result.read_text(), spec)
                require(stderr.exists() and stderr.stat().st_size == 0 and not partial.exists(),
                        "ambiguous completed case")
                continue
            require(not partial.exists() and not stderr.exists(), "partial case requires explicit recovery")
            verify_inputs(manifest, base)
            require(os.sysconf("SC_AVPHYS_PAGES") * os.sysconf("SC_PAGE_SIZE") >= 4 * 1024**3,
                    "less than4GiB available before oracle case")
            print("case_started seed=" + seed, flush=True)
            command = [manifest["oracle_binary"]["path"], "--manifest", str(base / manifest["adapter"]),
                       "--split", "development", "--only-seed", seed, "--details"]
            with partial.open("x") as out, stderr.open("x") as err:
                status = subprocess.run(command, stdout=out, stderr=err, check=False).returncode
            require(status == 0 and stderr.stat().st_size == 0, "oracle error; preserve partial case")
            parse_case(partial.read_text(), spec)
            verify_inputs(manifest, base)
            require(not result.exists(), "result collision")
            partial.rename(result)
            print("case_complete seed=" + seed, flush=True)
        require(len(list(args.output.glob("*.result"))) == 6 and not list(args.output.glob("*.partial")),
                "result/partial inventory mismatch")
        payload = ""
        for spec in manifest["cases"]:
            raw = (args.output / (str(spec["seed"]) + ".result")).read_text()
            parse_case(raw, spec)
            payload += raw.rstrip() + "\ncase_complete,seed=" + str(spec["seed"]) + "\n"
        payload += "run_complete,cases=6,traces=24\n"
        log = args.output / "witnesses.log"
        if log.exists():
            require(log.read_text() == payload, "combined log collision")
        else:
            with log.open("x") as stream:
                stream.write(payload)
        verify_inputs(manifest, base)
        with completed.open("x") as stream:
            json.dump({"cases": 6, "traces": 24, "manifest_sha256": args.manifest_sha256.upper(),
                       "log_sha256": digest(log)}, stream, indent=2)
            stream.write("\n")
        print("run_complete cases=6 traces=24", flush=True)


if __name__ == "__main__":
    main()
