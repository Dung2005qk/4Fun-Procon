"""Complete-run analysis only; synthetic runtime evidence is not BTC authority."""
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path

from run_http_baseline_314 import ROOT, digest


def require(ok, message):
    if not ok:
        raise ValueError(message)


def safety(replay):
    events = [json.loads(line) for line in Path(replay).read_text().splitlines() if line]
    by_kind = defaultdict(list)
    for event in events:
        by_kind[event["kind"]].append(event)
    for kind in ("day_state", "decision", "protected_slack", "actions", "action_result"):
        require(len(by_kind[kind]) == 4, f"missing or duplicated {kind}")
    forbidden = {"actions_fallback", "actions_recovery_wait", "actions_server_wait",
                 "actions_deadline_skip", "actions_transport_retry", "virtual_parent_dropped"}
    require(not (forbidden & by_kind.keys()), "fallback, retry or state-boundary failure")
    days = []
    for day in range(4):
        state = by_kind["day_state"][day]
        decision = by_kind["decision"][day]["body"]["decision"]
        ack = by_kind["action_result"][day]
        refiner = by_kind["protected_slack"][day]["body"]
        require(state["body"]["day"] == day and decision["dayNumber"] == day+1,
                "day identity mismatch")
        require(ack["status"] == 200 and ack["body"].get("valid") is True
                and ack["body"]["day"] == day+1, "invalid/stale ACK")
        require(not decision["emergency"] and decision["candidate"]["simulation"]["valid"], "invalid/emergency decision")
        require(decision["audit"]["selectionReason"] == "certified-undominated-current-floor", "wrong selection policy")
        require(decision["deadline"]["networkMs"] >= 1600, "missing HTTP calibration")
        require(decision["deadline"]["totalMs"] <= 5000, "main budget widened")
        require(not any(value for key,value in refiner.items() if key.endswith("Failure")), "checkpoint failure")
        require(not refiner.get("publicContinuationAuthorized", False), "unexpected public continuation")
        require(ack["atUnixMs"] <= state["body"]["endsAt"], "late response")
        days.append({"day": day+1, "response_ms": ack["atUnixMs"]-state["atUnixMs"],
            "main_ms": decision["timing"]["totalMs"], "main_budget_ms": decision["deadline"]["totalMs"],
            "reconciled_authoritative_state": decision["reconciledAuthoritativeState"],
            "refiner": refiner,
            "cache_repair": decision["cacheRepair"]})
    return {"days": days, "session_checkpoints": len(by_kind["session_checkpoint"]),
            "safety_pass": True}


def compare(left, right):
    for tier, (a,b) in enumerate(zip(left, right, strict=True), 1):
        if a != b:
            return ("win" if a>b else "loss"), tier, a-b
    return "tie", 0, 0


def summarize(manifest_path, directory):
    manifest = json.loads(manifest_path.read_text())
    marker = json.loads((directory/"run_complete.json").read_text())
    require(marker == {"cases": 12, "manifest_sha256": digest(manifest_path)}, "completion/hash mismatch")
    require(len(list(directory.glob("*.result.json"))) == 12, "incomplete/duplicate result set")
    results, strata, wtl = [], defaultdict(Counter), Counter()
    for spec in manifest["cases"]:
        prefix = directory / str(spec["seed"])
        result_path = prefix.with_suffix(".result.json")
        result = json.loads(result_path.read_text())
        require(all(result[k] == v for k,v in spec.items()), "case provenance mismatch")
        for suffix, key in ((".replay.jsonl", "replay_sha256"), (".transport.json", "transport_sha256"),
                            (".replay-check.txt", "replay_check_sha256")):
            require(digest(prefix.with_suffix(suffix)) == result[key], "case artifact hash mismatch")
        telemetry = safety(prefix.with_suffix(".replay.jsonl"))
        comparison, tier, gain = compare(spec["oracle_score"], result["http_score"])
        require(comparison != "loss", "HTTP exceeded complete oracle: interpretation blocked")
        wtl[comparison] += 1
        strata[spec["family"]][comparison] += 1
        result.update({"oracle_vs_http": comparison, "first_tier": tier, "oracle_gain": gain,
            "http_vs_old_engine_noncausal": compare(result["http_score"], spec["old_engine_score"]),
            "telemetry": telemetry, "result_sha256": digest(result_path)})
        results.append(result)
    return {"experiment": manifest["experiment"], "cases":12, "actions":48, "transitions":36,
        "synthetic_only":True, "manifest_sha256":digest(manifest_path),
        "oracle_http_wtl": {key:wtl[key] for key in ("win","tie","loss")},
        "family_strata":dict(strata), "results":results, "zero_safety_failure":True,
        "maximum_response_ms":max(day["response_ms"] for r in results for day in r["telemetry"]["days"]),
        "maximum_main_ms":max(day["main_ms"] for r in results for day in r["telemetry"]["days"]),
        "verdict":"residual-survives-full-http-path" if wtl["win"] else "no-residual-under-measured-fixed-role-cadence",
        "causal_attribution_authorized":bool(wtl["win"]), "score_successor_authorized":False,
        "holdout_open_authorized":False, "btc_authority":False,
        "limits":"Single local run; fixed all-Patrol resume; no real opponents or road traffic; five-second windows and actual idle scheduling; not a causal comparison to prior engine runs or a ceiling/promotion/latency verdict."}


def main():
    parser=argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--directory", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args=parser.parse_args()
    report=summarize(args.manifest,args.directory)
    with args.output.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report,stream,indent=2,sort_keys=True)
        stream.write("\n")
    print(json.dumps({k:v for k,v in report.items() if k not in ("results","family_strata")},indent=2))


if __name__ == "__main__":
    main()
