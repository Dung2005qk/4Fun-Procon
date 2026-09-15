"""Recorded shared-state suffix attribution only; never invoke an optimizer."""
import argparse
import json
from collections import Counter
from run_http_baseline_314 import ROOT, Bridge, digest
from run_http_witness_315 import require
from summarize_http_witness_315 import state_key

ID = "ATTR-SHARED-STATE-WITNESS-SUFFIX-317"
TARGETS = ((10700100, 5), (10700101, 6), (10700500, 4))
MANIFEST = ROOT / f"research/holdouts/{ID}.json"
SUMMARY = ROOT / f"research/evidence/{ID}.summary.json"
M315 = "research/holdouts/ATTR-THREE-PATROL-HTTP-WITNESS-315.json"
M316 = "research/holdouts/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316.json"
S315 = "research/evidence/ATTR-THREE-PATROL-HTTP-WITNESS-315.summary.json"


def load(path):
    return json.loads((ROOT / path).read_text(encoding="utf-8"))


def check_hashes(hashes):
    for path, expected in hashes.items():
        require(digest(ROOT / path) == expected, "frozen input changed: " + path)


def write_new(path, value):
    with path.open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write("\n")


def freeze():
    a, b = load(M315), load(M316)
    check_hashes(a["local_hashes"])
    check_hashes(b["hashes"])
    paths = set(a["local_hashes"]) | set(b["hashes"]) | {M315, M316, S315,
        "research/probes/replay_witness_suffix_317.py",
        "research/probes/test_witness_suffix_317.py",
        "research/evidence/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316.summary.json",
        "research/evidence/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316/run_complete.json"}
    for seed, _ in TARGETS:
        paths.add(f"research/evidence/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316/{seed}.result.jsonl")
    write_new(MANIFEST, {"experiment": ID, "targets": TARGETS, "parent": "c76a8ea",
        "hashes": {p: digest(ROOT / p) for p in sorted(paths)},
        "bridge": "artifacts/research/314/bridge.exe",
        "gate": "All3 cases18 suffix days; shared physical agents and exact ledger; zero failure. "
                "Recurrent first boundary in at least2 cases authorizes existing-source attribution only.",
        "holdout_authority": False, "production_change": False})
    print("frozen " + digest(MANIFEST))


def shared_start(oracle, output):
    require(state_key(oracle["oracle_agents"], oracle["oracle_ledger"]) ==
            state_key(output["current_agents"], output["current_ledger"]), "not the same exact day1 state/ledger")
    require(oracle["oracle_score"] == output["current_score"], "initial score mismatch")


def first_tier(a, b):
    return next((i + 1 for i, (x, y) in enumerate(zip(a, b, strict=True)) if x != y), None)


def compare_case(case, oracle, output, bridge):
    shared_start(oracle["days"][0], output)
    require(output["ok"] and output["certified"] and not output["lower_bound_only"] and
            len(output["future_plans"]) == 3, "incomplete exact W1 witness")
    agents = {p: output["current_agents"] for p in ("w1", "oracle")}
    ledgers = {p: output["current_ledger"] for p in agents}
    scores = {p: output["current_score"] for p in agents}
    days = []
    for index, reference in enumerate(oracle["days"][1:]):
        day = index + 2
        require(reference["day"] == day, "oracle day mismatch")
        shared = state_key(agents["w1"], ledgers["w1"]) == state_key(agents["oracle"], ledgers["oracle"])
        row = {"day": day, "shared_input": shared, "paths": {}}
        for path, plan in (("w1", output["future_plans"][index]), ("oracle", reference["oracle_plan"])):
            before = {"agents": agents[path], "ledger": ledgers[path], "score": scores[path]}
            result = bridge.request({"op": "step", "setup": case["setup"],
                "state": {"day": day, "endsAt": 0, "agents": agents[path], "others": [], "traffics": []},
                "ledger": ledgers[path], "plan": plan})
            require(result.get("ok") and result.get("agrees"), "suffix dual validation failure")
            if path == "oracle":
                require(state_key(result["agents"], result["ledger"]) ==
                        state_key(reference["oracle_agents"], reference["oracle_ledger"]) and
                        result["score"] == reference["oracle_score"], "oracle reference mismatch")
            row["paths"][path] = {"input": before, "plan": plan, "output": result,
                "score_increment": [a-b for a,b in zip(result["score"], scores[path], strict=True)]}
            agents[path], ledgers[path], scores[path] = result["agents"], result["ledger"], result["score"]
        row["same_output"] = state_key(agents["w1"], ledgers["w1"]) == state_key(agents["oracle"], ledgers["oracle"])
        row["same_unordered_output"] = state_key(agents["w1"], ledgers["w1"], True) == state_key(agents["oracle"], ledgers["oracle"], True)
        row["first_differing_tier"] = first_tier(scores["w1"], scores["oracle"])
        row["w1_vs_oracle"] = "W" if scores["w1"] > scores["oracle"] else "L" if scores["w1"] < scores["oracle"] else "T"
        days.append(row)
    require(scores["w1"] == output["w1_score"] == output["replayed_score"], "final W1 reference mismatch")
    return {"seed": case["seed"], "family": case["family"], "days": days,
        "first_state_divergence": next((d["day"] for d in days if not d["same_output"]), None),
        "first_unordered_divergence": next((d["day"] for d in days if not d["same_unordered_output"]), None),
        "first_score_difference": next((d["day"] for d in days if d["first_differing_tier"]), None),
        "first_loss": next((d["day"] for d in days if d["w1_vs_oracle"] == "L"), None),
        "final_scores": scores}


def run():
    manifest = load(MANIFEST)
    require(manifest["experiment"] == ID and manifest["targets"] == [list(x) for x in TARGETS], "manifest identity")
    check_hashes(manifest["hashes"])
    m316, s315 = load(M316), load(S315)
    bridge = Bridge(ROOT / manifest["bridge"])
    results = []
    try:
        for seed, index in TARGETS:
            case = next(c for c in m316["cases"] if c["seed"] == seed)
            oracle = next(c for c in s315["results"] if c["seed"] == seed)
            path = ROOT / f"research/evidence/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316/{seed}.result.jsonl"
            rows = [json.loads(line) for line in path.read_text().splitlines()]
            require([r["index"] for r in rows] == list(range(len(case["candidates"]))), "316 row bijection mismatch")
            results.append(compare_case(case, oracle, rows[index]["output"], bridge))
    finally:
        bridge.close()
        bridge.process.stdout.close()
        bridge.process.stderr.close()
    check_hashes(manifest["hashes"])
    report = {"experiment": ID, "cases": len(results), "dual_valid_suffix_days": 18,
        "run_complete": True, "manifest_sha256": digest(MANIFEST), "results": results,
        "first_boundary_counts": dict(Counter(r["first_state_divergence"] for r in results)),
        "promotion_authority": False, "holdout_authority": False,
        "limits": "Consumed roadless fixed-role attribution only. No new optimizer or production run. "
                  "Later unshared inputs cannot prove a local selection error. No local latency authority."}
    write_new(SUMMARY, report)
    print(json.dumps({"summary_sha256": digest(SUMMARY), "cases": 3, "dual_valid_suffix_days": 18,
        "results": [{k:v for k,v in r.items() if k != "days"} for r in results]}, indent=2))


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("freeze", "run"))
    {"freeze": freeze, "run": run}[parser.parse_args().mode]()
