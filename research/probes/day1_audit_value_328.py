"""Complete day1 recorded-action option values; no production search or tuning."""
import argparse
from collections import Counter
import json
from pathlib import Path
import subprocess
import time
from http_prefix_option_loss_321 import ROOT, load, digest, object_hash, require, write_new, state, close_bridge

ID = "ATTR-DAY1-AUDIT-EXACT-OPTION-328"
M = ROOT / f"research/holdouts/{ID}.json"
D = ROOT / f"research/evidence/{ID}"
S = ROOT / f"research/evidence/{ID}.summary.json"

def submitted_identity(checked, original):
    # The offline claims bridge adds claims; retain every original314 check.
    require(set(original) == {"ok", "agrees", "agents", "ledger", "score"}, "314 validated schema")
    require(all(checked[k] == v for k, v in original.items()), "selected/submitted identity")

def freeze():
    from pool_boundary_value_327 import singleton, verify_local
    from run_http_baseline_314 import Bridge
    from day2_pool_value_323 import scores
    previous = ROOT / "research/holdouts/ATTR-POOL-BOUNDARY-OPTION-VALUE-327.json"
    previous_m = load(previous); verify_local(previous_m)
    summary321 = ROOT / "research/evidence/ATTR-HTTP-PREFIX-OPTION-LOSS-321.summary.json"
    require(digest(summary321) == "E7AF660FB590FB01289E63700CE9F83F33F2B0B9317D3A04F11B36C851AC5B83", "321 summary")
    values = {c["seed"]: c for c in load(summary321)["matches"]}
    baseline = ROOT / "research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json"
    bm = load(baseline)
    cases = []; bridge = Bridge(ROOT / "artifacts/research/324/claims_bridge.exe")
    try:
        for spec, setup in zip(bm["cases"], bm["setups"], strict=True):
            seed = spec["seed"]
            folder = ROOT / "research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314"
            bodies = [e["body"] for line in (folder / f"{seed}.replay.jsonl").read_text().splitlines()
                      if (e := json.loads(line))["kind"] == "decision"]
            body = bodies[0]; require(len(bodies) == 4 and body["state"]["day"] == 1, "original day1 identity")
            q = {"setup": setup, "state": body["state"], "ledger": body["ledger"]}
            require(q["ledger"] == {"brands": [], "totalDailyDistinct": 0, "totalServings": 0} and
                    q["state"]["agents"] == [{"kind": 0, "pos": p, "fuel": setup["fuelLimits"]} for p in setup["agents"]], "initial root identity")
            scenarios = body["decision"]["manifest"]["scenarios"]
            require(len(scenarios) == 1 and scenarios[0]["class"] == "deterministic-no-road" and scenarios[0]["weight"] == 10000 and scenarios[0]["jointFeasible"], "scenario scope")
            audit = body["decision"]["audit"]["candidates"]
            require(len(audit) == 16 and sum(c["selected"] for c in audit) == 1 and len({c["stableId"] for c in audit}) == 16, "audit coverage")
            portfolios = {}; plans = {}
            action = load(folder / f"{seed}.transport.json")["actions"][0]
            for i, c in enumerate(audit):
                plan = json.loads(c["stableId"]); checked = bridge.request({**q, "op": "step", "plan": plan})
                require(checked.get("ok") and checked.get("agrees") and checked["score"] == scores(c["scoreAfterToday"]) and
                        [a["pos"] for a in checked["agents"]] == c["terminalCells"] and [a["fuel"] for a in checked["agents"]] == c["terminalFuel"], "audit action metadata")
                if c["selected"]:
                    require(plan == action["plan"] == body["decision"]["candidate"]["plan"], "selected/submitted plan identity")
                    submitted_identity(checked, action["validated"])
                label = f"p{i:02d}"
                portfolios[label] = singleton(plan, checked)
                plans[label] = {"plan": plan, "actual": checked, "audit": c}
            cases.append({"id": str(seed), "seed": seed, "family": spec["family"], "players": spec["players"],
                "ceiling": values[seed]["conditional_values"][0], "selected_value": values[seed]["conditional_values"][1],
                "actual_loss": values[seed]["days"][0], "plans": plans,
                "request": {**q, "portfolios": portfolios, "certificates": []}})
    finally: close_bridge(bridge)
    paths = set(previous_m["local_hashes"]) | {str(p.relative_to(ROOT)).replace('\\', '/') for p in
        (previous, baseline, summary321, Path(__file__), Path(__file__).with_name("test_day1_audit_value_328.py"))}
    require(len(cases) == 12, "all12 roots")
    write_new(M, {"experiment": ID, "cases": cases, "local_hashes": {p: digest(ROOT/p) for p in sorted(paths)},
        "vm_hashes": previous_m["vm_hashes"], "probe_sha256": previous_m["probe_sha256"],
        "runner_sha256": digest(Path(__file__)), "helper_sha256": digest(Path(__file__).with_name("http_prefix_option_loss_321.py")),
        "bridge": "artifacts/research/314/bridge.exe", "min_mem_available_kib": 4*1024*1024,
        "gate": "All12 roots192 original actions768 dual-days; selected321 exact value and312 ceiling; sound bounds. Existing better action on2 actual losses2 families permits distinct source design only.",
        "production_change": False, "holdout_authority": False, "score_successor_authorized": False})
    print(json.dumps({"manifest_sha256": digest(M), "roots": 12, "actions": 192, "runner_sha256": digest(Path(__file__))}))

def validate(r, c, mh, ph):
    require(r["id"] == c["id"] and r["manifest_sha256"] == mh and r["probe_sha256"] == ph and
            r["request_sha256"] == object_hash(c["request"]), "result provenance")
    o = r["oracle"]
    require(o.get("ok") and o.get("complete") and o["certificates_checked"] == 0 and
            set(o["portfolios"]) == set(c["plans"]), "complete oracle coverage")
    for label, p in o["portfolios"].items():
        require(p["complete"] and [d["day"] for d in p["days"]] == [1, 2, 3, 4] and
                p["admissible_combinations"] == p["unique_joint_roots"] == 1 and p["incompatible_combinations"] == 0 and
                p["columns_before"] == 3 and p["days"][0]["plan"] == c["plans"][label]["plan"], "singleton source identity")

def execute(manifest, mh, directory, probe):
    import fcntl
    m = load(manifest)
    require(digest(manifest) == mh and digest(Path(__file__)) == m["runner_sha256"], "manifest/runner drift")
    require(digest(Path(__file__).with_name("http_prefix_option_loss_321.py")) == m["helper_sha256"], "helper drift")
    for p, h in m["vm_hashes"].items(): require(digest(p) == h, "VM input drift: " + p)
    require(digest(probe) == m["probe_sha256"] and len(m["cases"]) == 12, "probe/count")
    directory.mkdir(exist_ok=True)
    with (directory / "runner.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        require(not list(directory.glob("*.partial")), "ambiguous partial; audit before resume")
        require({p.name for p in directory.glob("*.result.json")} <= {c["id"] + ".result.json" for c in m["cases"]}, "unknown result")
        for c in m["cases"]:
            out = directory / (c["id"] + ".result.json")
            if out.exists(): validate(load(out), c, mh, m["probe_sha256"]); continue
            mem = {line.split()[0].rstrip(':'): int(line.split()[1]) for line in Path('/proc/meminfo').read_text().splitlines()}
            require(mem["MemAvailable"] >= m["min_mem_available_kib"], "memory safety")
            partial = directory / (c["id"] + ".partial"); err = directory / (c["id"] + ".stderr")
            require(not err.exists() and not (directory / (c["id"] + ".stdout.json")).exists(), "orphaned case")
            start = time.monotonic()
            with partial.open("x", encoding="utf-8") as stdout, err.open("x", encoding="utf-8") as stderr:
                p = subprocess.run([str(probe)], input=json.dumps(c["request"])+"\n", text=True, stdout=stdout, stderr=stderr)
            require(p.returncode == 0 and err.stat().st_size == 0, "oracle process error")
            r = {"id": c["id"], "manifest_sha256": mh, "probe_sha256": m["probe_sha256"],
                "request_sha256": object_hash(c["request"]), "oracle": load(partial), "elapsed_seconds": time.monotonic()-start}
            validate(r, c, mh, m["probe_sha256"])
            commit = directory / (c["id"] + ".commit.partial"); write_new(commit, r); commit.rename(out)
            partial.rename(directory / (c["id"] + ".stdout.json"))
            print(json.dumps({"event": "case_complete", "id": c["id"], "sha256": digest(out)}), flush=True)
        marker = {"experiment": ID, "manifest_sha256": mh, "cases": 12,
                  "results": {p.name: digest(p) for p in sorted(directory.glob("*.result.json"))}}
        require(len(marker["results"]) == 12, "complete result set")
        if (directory / "run_complete.json").exists(): require(load(directory / "run_complete.json") == marker, "marker drift")
        else: write_new(directory / "run_complete.json", marker)
        print(json.dumps({"event": "run_complete", "cases": 12}), flush=True)

def bounds(audit, value):
    from day2_pool_value_323 import scores
    return {"upper_unsound": tuple(scores(audit["validUpperBound"])) < tuple(value),
        "certified_lower_unsound": audit["certified"] and tuple(scores(audit["finalCertifiedLowerBound"])) > tuple(value),
        "provisional_above_optimum": tuple(scores(audit["provisionalLowerBound"])) > tuple(value)}

def summarize(manifest, directory, output):
    from run_http_baseline_314 import Bridge
    from pool_boundary_value_327 import verify_local
    from day2_pool_value_323 import comparison, scores
    m = load(manifest); verify_local(m); mh = digest(manifest)
    marker = load(directory / "run_complete.json")
    require(marker["cases"] == 12 and marker["manifest_sha256"] == mh, "completion marker")
    require(set(marker["results"]) == {c["id"] + ".result.json" for c in m["cases"]} ==
            {p.name for p in directory.glob("*.result.json")}, "complete root coverage")
    require(not list(directory.glob("*.partial")), "partial evidence")
    bridge = Bridge(ROOT/m["bridge"]); matches = []; dual = 0; failures = []
    try:
        for c in m["cases"]:
            path = directory/(c["id"]+'.result.json'); require(digest(path) == marker["results"][path.name], "result hash")
            r = load(path); validate(r, c, mh, m["probe_sha256"]); rows = []
            selected_today = next(p["actual"]["score"] for p in c["plans"].values() if p["audit"]["selected"])
            for label, p in c["plans"].items():
                result = r["oracle"]["portfolios"][label]; agents = c["request"]["state"]["agents"]; ledger = c["request"]["ledger"]
                for day in result["days"]:
                    checked = bridge.request({"op":"step", "setup":c["request"]["setup"], "state":state(agents,day["day"]), "ledger":ledger, "plan":day["plan"]})
                    require(checked.get("ok") and checked.get("agrees") and all(checked[k] == day[k] for k in ("score","agents","ledger")), "dual suffix reconstruction")
                    agents, ledger = checked["agents"], checked["ledger"]; dual += 1
                value = result["score"]
                require(value == result["days"][-1]["score"] and tuple(value) <= tuple(c["ceiling"]), "global ceiling")
                if p["audit"]["selected"]: require(value == c["selected_value"], "selected321 identity")
                b = bounds(p["audit"], value)
                if b["upper_unsound"] or b["certified_lower_unsound"]: failures.append({"seed":c["seed"],"label":label,**b})
                rows.append({"label":label, "audit":p["audit"], "value":value, "bounds":b,
                    "versus_selected":comparison(value,c["selected_value"]),
                    "today_versus_selected":comparison(p["actual"]["score"],selected_today),
                    "provisional_slack":comparison(value,scores(p["audit"]["provisionalLowerBound"]))})
            best = max((r["value"] for r in rows),key=tuple)
            matches.append({"seed":c["seed"],"family":c["family"],"players":c["players"],"actual_loss":c["actual_loss"],
                "selected":c["selected_value"],"pool_best":best,"ceiling":c["ceiling"],"pool_vs_selected":comparison(best,c["selected_value"]),"plans":rows})
    finally: close_bridge(bridge)
    require(dual == 768 and len(matches) == 12, "complete validation coverage")
    gains = [r for r in matches if r["actual_loss"]["first_tier"] is not None and r["pool_vs_selected"]["result"] == "win"]
    report = {"experiment":ID,"manifest_sha256":mh,"run_complete_sha256":digest(directory/'run_complete.json'),
        "result_hashes":marker["results"],"roots":12,"actions":192,"dual_days":dual,"zero_reconstruction_failure":True,
        "bound_failures":failures,"matches":matches,"existing_action_source_design_qualified":not failures and len(gains)>=2 and len({r["family"] for r in gains})>=2,
        "gains":[r["seed"] for r in gains],"families":sorted({r["family"] for r in gains}),
        "production_change":False,"holdout_authority":False,"score_successor_authorized":False,
        "strata":{f:{str(k):dict(Counter(r["pool_vs_selected"]["result"] for r in matches if r[f]==k)) for k in sorted({r[f] for r in matches})} for f in ("family","players")}}
    write_new(output,report)
    print(json.dumps({"summary_sha256":digest(output),"dual_days":dual,"bound_failures":len(failures),"gains":report["gains"],"design_qualified":report["existing_action_source_design_qualified"]}))
    for r in matches: print(json.dumps({k:v for k,v in r.items() if k!='plans'}))

if __name__ == '__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','execute','summarize'))
    p.add_argument('--manifest',type=Path,default=M);p.add_argument('--manifest-sha256');p.add_argument('--directory',type=Path,default=D)
    p.add_argument('--probe',type=Path);p.add_argument('--output',type=Path,default=S);a=p.parse_args()
    if a.mode=='freeze':freeze()
    elif a.mode=='execute':execute(a.manifest,a.manifest_sha256,a.directory,a.probe)
    else:summarize(a.manifest,a.directory,a.output)
