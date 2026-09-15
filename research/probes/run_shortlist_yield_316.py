"""Complete recorded-pool attribution; no production invocation or oracle input."""
import argparse
import json
from pathlib import Path
from run_http_baseline_314 import ROOT, Bridge, digest
from summarize_http_baseline_314 import summarize as baseline_summary
from summarize_http_witness_315 import score_list
from run_http_witness_315 import require

ID = "ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316"
MANIFEST = ROOT / ("research/holdouts/" + ID + ".json")
DIRECTORY = ROOT / ("research/evidence/" + ID)


def freeze():
    old_path = ROOT / "research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json"
    old = json.loads(old_path.read_text())
    baseline_summary(old_path, ROOT / "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314")
    for path, expected in old["hashes"].items():
        require(digest(ROOT/path) == expected, "314 frozen input changed")
    paths = list(old["hashes"]) + [str(old_path.relative_to(ROOT)).replace("\\", "/"),
        "research/evidence/ATTR-THREE-PATROL-HTTP-WITNESS-315.summary.json",
        "research/probes/http_shortlist_yield_316.cpp", "research/probes/run_shortlist_yield_316.py",
        "research/probes/test_shortlist_yield_316.py", "research/probes/run_http_witness_315.py",
        "research/probes/summarize_http_witness_315.py", "artifacts/research/316/probe.exe"]
    cases = []
    for spec, setup in zip(old["cases"], old["setups"], strict=True):
        replay = "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314/" + str(spec["seed"]) + ".replay.jsonl"
        events = [json.loads(line) for line in (ROOT/replay).read_text().splitlines() if line]
        body = next(event["body"] for event in events if event["kind"] == "decision")
        scenario = body["decision"]["manifest"]["scenarios"]
        require(len(scenario) == 1 and scenario[0]["class"] == "deterministic-no-road"
                and scenario[0]["weight"] == 10000 and scenario[0]["jointFeasible"] is True
                and not any(scenario[0]["opponentCarryFootprint"] + scenario[0]["opponentCurrentFootprint"])
                and not scenario[0]["pessimisticFallback"], "unexpected recorded scenario")
        candidates = body["decision"]["audit"]["candidates"]
        require(len(candidates) > 0 and sum(row["selected"] for row in candidates) == 1,
                "missing candidate pool/selected reference")
        cases.append({"seed": spec["seed"], "family": spec["family"], "players": spec["players"],
            "setup": setup, "state": body["state"], "ledger": body["ledger"], "candidates": candidates,
            "recorded_selected_certificate": score_list(next(row for row in candidates if row["selected"])["finalCertifiedLowerBound"])})
        paths.append(replay)
    manifest = {"experiment": ID, "parent": "c76a8ea", "cases": cases,
        "requests": sum(len(case["candidates"]) for case in cases), "probe": "artifacts/research/316/probe.exe",
        "hashes": {path: digest(ROOT/path) for path in sorted(set(paths))},
        "gate": "All12 cases and every recorded day1 candidate. F0 cap24, W1 cap200 mode7, isolated W1 1000ms. "
                "Require exact current outcomes and dual-valid suffix scores. Record F0 mismatches separately. "
                "At least2 cases with unshortlisted W1 certificate above both recorded and same-condition selected "
                "certificate authorizes further capability attribution only; no SCORE cap/order change.",
        "scope": "Consumed development and isolated cold calls only; no new prevalence, closed-loop or performance authority.",
        "production_change": False, "holdout_authority": False}
    with MANIFEST.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(manifest, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps({"manifest": str(MANIFEST), "sha256": digest(MANIFEST), "requests": manifest["requests"]}))


def validate_row(output, candidate):
    require(output.get("ok") and output["certified"] and output["dual_valid_days"] == 3,
            "invalid/incomplete current candidate or W1 witness")
    require(output["current_score"] == score_list(candidate["scoreAfterToday"]), "current score mismatch")
    require([agent["pos"] for agent in output["current_agents"]] == candidate["terminalCells"]
            and [agent["fuel"] for agent in output["current_agents"]] == candidate["terminalFuel"],
            "current terminal mismatch")
    if output["lower_bound_only"]:
        require(tuple(output["replayed_score"]) >= tuple(output["w1_score"]), "invalid lower bound")
    else:
        require(output["replayed_score"] == output["w1_score"], "W1 replay score mismatch")


def run():
    manifest = json.loads(MANIFEST.read_text())
    for path, expected in manifest["hashes"].items():
        require(digest(ROOT/path) == expected, "frozen input mismatch: " + path)
    DIRECTORY.mkdir(exist_ok=False)
    bridge = Bridge(ROOT / manifest["probe"])
    validator = Bridge(ROOT / "artifacts/research/314/bridge.exe")
    try:
        for case in manifest["cases"]:
            partial = DIRECTORY / (str(case["seed"]) + ".partial")
            with partial.open("x", encoding="utf-8", newline="\n") as stream:
                for index, candidate in enumerate(case["candidates"]):
                    request = {"setup": case["setup"], "state": case["state"], "ledger": case["ledger"],
                        "plan": json.loads(candidate["stableId"]), "upper": score_list(candidate["validUpperBound"])}
                    output = bridge.request(request)
                    validate_row(output, candidate)
                    checked = validator.request({"op": "step", **request})
                    require(checked.get("ok") and checked["agrees"] and
                            checked["ledger"] == output["current_ledger"] and
                            checked["agents"] == output["current_agents"] and
                            checked["score"] == output["current_score"], "current dual replay mismatch")
                    stream.write(json.dumps({"index": index, "output": output}, sort_keys=True) + "\n")
                    stream.flush()
            partial.rename(DIRECTORY / (str(case["seed"]) + ".result.jsonl"))
            print("case_complete seed=" + str(case["seed"]), flush=True)
    finally:
        for process in (bridge, validator):
            process.close()
            process.process.stdout.close()
            process.process.stderr.close()
    for path, expected in manifest["hashes"].items():
        require(digest(ROOT/path) == expected, "frozen input changed during run")
    with (DIRECTORY/"run_complete.json").open("x", encoding="utf-8") as stream:
        json.dump({"cases": 12, "requests": manifest["requests"], "manifest_sha256": digest(MANIFEST)}, stream, indent=2)
    print("run_complete cases=12", flush=True)


def summarize():
    manifest = json.loads(MANIFEST.read_text())
    for path, expected in manifest["hashes"].items():
        require(digest(ROOT/path) == expected, "frozen input mismatch: " + path)
    require(json.loads((DIRECTORY/"run_complete.json").read_text()) ==
            {"cases": 12, "requests": manifest["requests"], "manifest_sha256": digest(MANIFEST)}, "completion mismatch")
    require(len(list(DIRECTORY.glob("*.result.jsonl"))) == 12 and not list(DIRECTORY.glob("*.partial")), "bad inventory")
    results = []
    for case in manifest["cases"]:
        path = DIRECTORY / (str(case["seed"]) + ".result.jsonl")
        rows = [json.loads(line) for line in path.read_text().splitlines()]
        require([row["index"] for row in rows] == list(range(len(case["candidates"]))), "candidate bijection mismatch")
        for row, candidate in zip(rows, case["candidates"], strict=True):
            validate_row(row["output"], candidate)
        selected = next(row["output"]["w1_score"] for row, candidate in zip(rows, case["candidates"], strict=True)
                        if candidate["selected"])
        threshold = max(tuple(selected), tuple(case["recorded_selected_certificate"]))
        winners = [row["index"] for row, candidate in zip(rows, case["candidates"], strict=True)
                   if candidate["disposition"] == "not-shortlisted" and tuple(row["output"]["w1_score"]) > threshold]
        f0_mismatch = [row["index"] for row, candidate in zip(rows, case["candidates"], strict=True)
                       if row["output"]["f0_score"] != score_list(candidate["provisionalLowerBound"])]
        results.append({"seed": case["seed"], "family": case["family"], "candidates": len(rows),
            "recorded_selected_certificate": case["recorded_selected_certificate"], "isolated_selected_certificate": selected,
            "unshortlisted_strict_winners": winners, "f0_mismatches": f0_mismatch,
            "best_w1_score": max(row["output"]["w1_score"] for row in rows), "result_sha256": digest(path)})
    qualified_cases = sum(bool(result["unshortlisted_strict_winners"]) for result in results)
    report = {"experiment": ID, "cases": 12, "requests": manifest["requests"], "results": results,
        "qualifying_cases": qualified_cases, "next_attribution_authorized": qualified_cases >= 2,
        "f0_mismatch_count": sum(len(result["f0_mismatches"]) for result in results),
        "manifest_sha256": digest(MANIFEST), "score_promotion_authority": False, "holdout_authority": False,
        "scope": "Isolated cold certification, not a causal timed HTTP rerun; no local performance or ceiling claim."}
    with (ROOT/("research/evidence/"+ID+".summary.json")).open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(report, stream, indent=2, sort_keys=True)
        stream.write("\n")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("freeze", "run", "summarize"))
    mode = parser.parse_args().mode
    {"freeze": freeze, "run": run, "summarize": summarize}[mode]()
