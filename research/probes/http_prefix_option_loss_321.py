"""Consumed HTTP-prefix attribution; no planner calls or production writes."""
import argparse
from collections import Counter, defaultdict
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import time

ID = "ATTR-HTTP-PREFIX-OPTION-LOSS-321"
ROOT = Path(__file__).resolve().parents[2]
MANIFEST = ROOT / f"research/holdouts/{ID}.json"
DIRECTORY = ROOT / f"research/evidence/{ID}"
M314 = "research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json"
D314 = "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314"
VM_HASHES = {
    "probe": "E3B5FDF419E81091E4D1CDBBEECF138CA43E6BB0F80E353C305632F5E65E7292",
    "http_prefix_oracle_321.cpp": "0AD9DF72EAC5C23312BB0BFDF51F791F137030294E5E83EA0CA03FB33E4DCD8E",
    "/home/LMC/udon312-0905/research/probes/multi_patrol_oracle.cpp": "5979B702FB175B63077CCBB250B6E55B5B974ECA80543B3B7C105123F94C1BCE",
    "/home/LMC/udon312-0905/build/libudon_shield.a": "1257982C6A7F048E9437CDD1E235D6478523F8FA4C871A1DFDDF07FB7EDEF11B",
}

def require(ok, message):
    if not ok:
        raise ValueError(message)

def digest(path):
    return hashlib.sha256(Path(path).read_bytes()).hexdigest().upper()

def object_hash(obj):
    return hashlib.sha256(json.dumps(obj, sort_keys=True, separators=(",", ":")).encode()).hexdigest().upper()

def load(path):
    return json.loads(Path(path).read_text(encoding="utf-8"))

def write_new(path, value):
    with Path(path).open("x", encoding="utf-8", newline="\n") as stream:
        json.dump(value, stream, sort_keys=True, indent=2)
        stream.write("\n")

def close_bridge(bridge):
    bridge.close()
    bridge.process.stdout.close()
    bridge.process.stderr.close()

def state(agents, day):
    return {"day": day, "endsAt": 0, "agents": copy.deepcopy(agents), "others": [], "traffics": []}

def freeze():
    from run_http_baseline_314 import Bridge, EMPTY_LEDGER
    from summarize_http_baseline_314 import summarize
    old = load(ROOT / M314)
    for path, expected in old["hashes"].items():
        require(digest(ROOT / path) == expected, "314 source drift: " + path)
    complete = summarize(ROOT / M314, ROOT / D314)
    require(complete["cases"] == 12 and complete["zero_safety_failure"], "314 not complete")
    paths = set(old["hashes"]) | {M314, D314 + ".summary.json", D314 + "/run_complete.json",
        "research/probes/http_prefix_oracle_321.cpp", "research/probes/http_prefix_option_loss_321.py",
        "research/probes/test_http_prefix_oracle_321.py", "research/probes/test_http_prefix_option_loss_321.py"}
    parity = []
    pairs = [(f"include/udon/{p.name}", str(p.relative_to(ROOT)))
             for p in sorted((ROOT / "artifacts/research/321/reference_include/udon").glob("*.hpp"))]
    pairs += [(f"src/{name}.cpp", f"artifacts/research/321/src/{name}.cpp")
              for name in ("simulator", "validator", "protocol")]
    require(len(pairs) == 16, "source parity inventory")
    for local, remote in pairs:
        a, b = (ROOT / local).read_bytes(), (ROOT / remote).read_bytes()
        require(a.replace(b"\r\n", b"\n") == b.replace(b"\r\n", b"\n"), "source parity: " + local)
        parity.append({"local": local, "reference": remote, "local_sha256": digest(ROOT / local),
                       "reference_sha256": digest(ROOT / remote), "equivalent_except_crlf": True})
        paths.update((local, remote))
    bridge = Bridge(ROOT / old["bridge_binary"])
    cases = []
    try:
        for spec, setup in zip(old["cases"], old["setups"], strict=True):
            prefix = f"{D314}/{spec['seed']}"
            for suffix in (".result.json", ".transport.json", ".replay.jsonl", ".replay-check.txt"):
                paths.add(prefix + suffix)
            transport, result = load(ROOT / (prefix + ".transport.json")), load(ROOT / (prefix + ".result.json"))
            agents = [{"kind": 0, "pos": p, "fuel": setup["fuelLimits"]} for p in setup["agents"]]
            ledger = copy.deepcopy(EMPTY_LEDGER)
            require(len(transport["actions"]) == 4, "HTTP prefix count")
            for day, action in enumerate(transport["actions"], 1):
                wire = action["state"]
                require(action["wire_day"] == day-1 and wire["day"] == day-1 and wire["agents"] == agents
                        and wire["others"] == [] and wire["traffics"] == [], "actual prefix identity")
                step = bridge.request({"op": "step", "setup": setup, "state": state(agents, day),
                                       "ledger": ledger, "plan": action["plan"]})
                require(step == action["validated"] and step["ok"] and step["agrees"], "accepted action reconstruction")
                agents, ledger = step["agents"], step["ledger"]
                if day < 4:
                    cases.append({"id": f"{spec['seed']}-d{day}", "seed": spec["seed"],
                        "family": spec["family"], "players": spec["players"], "after_day": day,
                        "global_score": spec["oracle_score"], "http_score": result["http_score"],
                        "request": {"setup": setup, "state": state(agents, day+1), "ledger": copy.deepcopy(ledger)}})
            require(step["score"] == result["http_score"], "realized final score")
    finally:
        close_bridge(bridge)
    require(len(cases) == 36 and len({c["id"] for c in cases}) == 36, "root count")
    write_new(MANIFEST, {"experiment": ID, "parent": "c76a8ea", "cases": cases,
        "local_hashes": {p: digest(ROOT / p) for p in sorted(paths)}, "vm_hashes": VM_HASHES,
        "runner_sha256": digest(Path(__file__)), "source_parity": parity,
        "bridge": old["bridge_binary"], "min_mem_available_kib": 4 * 1024 * 1024,
        "gate": "All36 complete; zero failure; exact physical suffix dual-validation; monotone conditioned optimum. "
                "Loss on at least2 cases across2 families permits source attribution of that boundary only.",
        "score_successor_authorized": False, "holdout_authority": False, "production_change": False})
    print(json.dumps({"manifest": str(MANIFEST), "sha256": digest(MANIFEST), "roots": 36, "source_parity_files": 16}))

def validate_result(result, case, manifest_hash, probe_hash):
    require(result["id"] == case["id"] and result["request_sha256"] == object_hash(case["request"])
            and result["manifest_sha256"] == manifest_hash and result["probe_sha256"] == probe_hash,
            "result identity/hash mismatch")
    value = result["oracle"]
    require(value.get("ok") is True and value.get("complete") is True and value.get("failure") is None,
            "oracle incomplete/failed")
    require(len(value["score"]) == 3 and all(type(x) is int and x >= 0 for x in value["score"]), "bad score")
    require([d["day"] for d in value["days"]] == list(range(case["after_day"]+1, 5)), "suffix day coverage")
    require(value["days"][-1]["score"] == value["score"], "suffix final score")

def execute(manifest, expected_hash, directory, probe):
    import fcntl
    m = load(manifest)
    require(digest(manifest) == expected_hash, "manifest drift")
    require(digest(Path(__file__)) == m["runner_sha256"], "runner drift")
    for path, expected in m["vm_hashes"].items():
        require(digest(probe.parent / path) == expected, "VM frozen hash drift: " + path)
    probe_hash = digest(probe)
    require(len(m["cases"]) == 36 and len({c["id"] for c in m["cases"]}) == 36, "manifest root set")
    directory.mkdir(exist_ok=True)
    with (directory / "runner.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        require(not list(directory.glob("*.partial")), "ambiguous partial evidence; manual operational audit required")
        expected_files = {c["id"] + ".result.json" for c in m["cases"]}
        require({p.name for p in directory.glob("*.result.json")} <= expected_files, "unknown completed root")
        for case in m["cases"]:
            output = directory / (case["id"] + ".result.json")
            if output.exists():
                validate_result(load(output), case, expected_hash, probe_hash)
                continue
            memory = {line.split()[0].rstrip(":"): int(line.split()[1]) for line in Path("/proc/meminfo").read_text().splitlines()}
            require(memory["MemAvailable"] >= m["min_mem_available_kib"], "RAM safety gate before root")
            require(digest(probe) == probe_hash, "binary drift between roots")
            start = time.monotonic()
            partial = directory / (case["id"] + ".partial")
            stderr = directory / (case["id"] + ".stderr")
            with partial.open("x", encoding="utf-8") as out, stderr.open("x", encoding="utf-8") as err:
                proc = subprocess.run([str(probe)], input=json.dumps(case["request"])+"\n", text=True, stdout=out, stderr=err)
            require(proc.returncode == 0 and stderr.stat().st_size == 0, "oracle process error: " + case["id"])
            result = {"id": case["id"], "manifest_sha256": expected_hash, "probe_sha256": probe_hash,
                      "request_sha256": object_hash(case["request"]), "oracle": load(partial),
                      "elapsed_seconds": time.monotonic()-start, "mem_available_before_kib": memory["MemAvailable"]}
            validate_result(result, case, expected_hash, probe_hash)
            write_new(output, result)
            # Retain the original stdout rather than deleting evidence.
            partial.rename(directory / (case["id"] + ".stdout.json"))
            print(json.dumps({"event": "case_complete", "id": case["id"], "result_sha256": digest(output)}), flush=True)
        hashes = {p.name: digest(p) for p in sorted(directory.glob("*.result.json"))}
        require(len(hashes) == 36, "incomplete root set")
        marker = {"experiment": ID, "manifest_sha256": expected_hash, "results": hashes, "cases": 36}
        completion = directory / "run_complete.json"
        if completion.exists():
            require(load(completion) == marker, "completion drift")
        else:
            write_new(completion, marker)
        print(json.dumps({"event": "run_complete", "cases": 36}), flush=True)

def option_losses(values):
    require(len(values) == 5 and all(len(v) == 3 for v in values), "conditional value dimensions")
    require(all(tuple(a) >= tuple(b) >= tuple(values[-1]) for a, b in zip(values, values[1:])), "nonmonotone conditional optimum")
    losses = []
    for day, (a, b) in enumerate(zip(values, values[1:]), 1):
        tier = next((i+1 for i, (x, y) in enumerate(zip(a, b)) if x != y), None)
        losses.append({"day": day, "first_tier": tier, "first_tier_loss": a[tier-1]-b[tier-1] if tier else 0,
                       "components_lost": [x-y for x, y in zip(a, b)]})
    require([sum(d["components_lost"][i] for d in losses) for i in range(3)] ==
            [a-b for a, b in zip(values[0], values[-1])], "component telescope")
    return losses

def summarize(manifest, directory, output):
    from run_http_baseline_314 import Bridge
    m, mh = load(manifest), digest(manifest)
    for path, expected in m["local_hashes"].items():
        require(digest(ROOT / path) == expected, "local frozen hash drift: " + path)
    marker = load(directory / "run_complete.json")
    require(marker["manifest_sha256"] == mh and marker["cases"] == 36, "completion identity")
    require(set(marker["results"]) == {c["id"] + ".result.json" for c in m["cases"]} ==
            {p.name for p in directory.glob("*.result.json")}, "exact completed root set")
    require(not list(directory.glob("*.partial")), "ambiguous partial")
    bridge = Bridge(ROOT / m["bridge"])
    by_seed = defaultdict(list)
    days_validated = 0
    try:
        for case in m["cases"]:
            name = case["id"] + ".result.json"
            require(digest(directory / name) == marker["results"][name], "atomic result drift")
            result = load(directory / name)
            validate_result(result, case, mh, m["vm_hashes"]["probe"])
            agents, ledger = case["request"]["state"]["agents"], case["request"]["ledger"]
            for day in result["oracle"]["days"]:
                step = bridge.request({"op": "step", "setup": case["request"]["setup"],
                    "state": state(agents, day["day"]), "ledger": ledger, "plan": day["plan"]})
                require(step.get("ok") and step.get("agrees") and all(step[k] == day[k] for k in ("agents", "ledger", "score")),
                        "cross-platform suffix reconstruction mismatch")
                agents, ledger = step["agents"], step["ledger"]
                days_validated += 1
            by_seed[case["seed"]].append((case, result))
    finally:
        close_bridge(bridge)
    matches, strata, first_days, tiers = [], defaultdict(Counter), Counter(), Counter()
    for seed, roots in sorted(by_seed.items()):
        roots.sort(key=lambda x: x[0]["after_day"])
        spec = roots[0][0]
        require([c["after_day"] for c, _ in roots] == [1, 2, 3], "prefix coverage")
        values = [spec["global_score"]] + [r["oracle"]["score"] for _, r in roots] + [spec["http_score"]]
        losses = option_losses(values)
        first = next((d["day"] for d in losses if d["first_tier"]), None)
        label = f"first_loss_day_{first}" if first else "no_loss"
        first_days[label] += 1
        for d in losses:
            if d["first_tier"]:
                tiers[str(d["first_tier"])] += 1
        for stratum in (spec["family"], "players_" + str(spec["players"])):
            strata[stratum][label] += 1
        matches.append({"seed": seed, "family": spec["family"], "players": spec["players"],
                        "conditional_values": values, "first_loss_day": first, "days": losses,
                        "work": [{k: r["oracle"][k] for k in ("memo_states", "day_enumerations", "joint_transitions")} for _, r in roots]})
    affected = [r for r in matches if r["first_loss_day"] is not None]
    authorized = len(affected) >= 2 and len({r["family"] for r in affected}) >= 2
    require(len(matches) == 12 and days_validated == 72, "complete reconstruction coverage")
    report = {"experiment": ID, "manifest_sha256": mh, "run_complete_sha256": digest(directory / "run_complete.json"),
        "result_hashes": marker["results"], "roots": 36, "matches": matches, "dual_validated_suffix_days": days_validated,
        "zero_failure": True, "monotonicity_pass": True, "first_loss_days": dict(first_days),
        "loss_tiers": dict(tiers), "strata": dict(strata), "source_attribution_authorized": authorized,
        "score_successor_authorized": False, "holdout_authority": False, "production_change": False}
    write_new(output, report)
    print(json.dumps({"summary_sha256": digest(output), "first_loss_days": dict(first_days), "source_attribution_authorized": authorized}))

def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("mode", choices=("freeze", "execute", "summarize"))
    parser.add_argument("--manifest", type=Path, default=MANIFEST)
    parser.add_argument("--manifest-sha256")
    parser.add_argument("--directory", type=Path, default=DIRECTORY)
    parser.add_argument("--probe", type=Path)
    parser.add_argument("--output", type=Path, default=ROOT / f"research/evidence/{ID}.summary.json")
    args = parser.parse_args()
    if args.mode == "freeze":
        freeze()
    elif args.mode == "execute":
        execute(args.manifest.resolve(), args.manifest_sha256, args.directory.resolve(), args.probe.resolve())
    else:
        summarize(args.manifest, args.directory, args.output)

if __name__ == "__main__":
    main()
