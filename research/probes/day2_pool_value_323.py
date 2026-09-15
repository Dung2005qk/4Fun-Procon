"""Exact attribution for an already-recorded pool; not a production solver."""
import argparse
from collections import Counter, defaultdict
import json
from pathlib import Path
import subprocess
import time
from http_prefix_option_loss_321 import ROOT, load, digest, object_hash, require, write_new, validate_result, state, close_bridge

ID = "ATTR-DAY2-POOL-EXACT-VALUE-323"
M = ROOT / f"research/holdouts/{ID}.json"
D = ROOT / f"research/evidence/{ID}"
M322 = "research/holdouts/ATTR-DAY2-OPTION-PROVENANCE-322.json"
S322 = "research/evidence/ATTR-DAY2-OPTION-PROVENANCE-322.summary.json"

def scores(a):
    return [a[k] for k in ("lifetimeDistinct", "totalDailyDistinct", "totalServings")]

def comparison(a, b):
    for i, (x, y) in enumerate(zip(a, b, strict=True)):
        if x != y:
            return {"result": "win" if x > y else "loss", "tier": i+1, "difference": x-y,
                    "components": [u-v for u,v in zip(a,b)]}
    return {"result":"tie", "tier":None, "difference":0, "components":[0,0,0]}

def freeze():
    parent, summary = load(ROOT / M322), load(ROOT / S322)
    require(summary["manifest_sha256"] == digest(ROOT / M322) and summary["zero_failure"]
            and summary["roots"] == 12 and summary["audited_candidates"] == 192, "322 incomplete")
    for p, expected in parent["hashes"].items():
        require(digest(ROOT / p) == expected, "322 frozen input drift: " + p)
    paths = set(parent["hashes"]) | {M322, S322, "research/probes/day2_pool_value_323.py",
                                   "research/probes/test_day2_pool_value_323.py"}
    roots = {r["seed"]: r for r in parent["cases"]}
    cases = []
    for result in summary["results"]:
        root = roots[result["seed"]]
        require(result["selected_matches_submitted"], "selected/submitted not same")
        for row in result["candidates"]:
            require(row["candidate"] == root["audit"][row["index"]], "audit identity")
            cases.append({"id": f"{result['seed']}-c{row['index']:02d}", "seed": result["seed"],
                "family": result["family"], "players": result["players"], "index": row["index"],
                "after_day": 2, "candidate": row["candidate"], "ceiling": root["conditioned_values"][1],
                "selected_value": root["conditioned_values"][2], "http_final": root["conditioned_values"][4],
                "request": {"setup": root["request"]["setup"], "state": state(row["actual"]["agents"],3),
                            "ledger": row["actual"]["ledger"]}})
    require(len(cases) == 192 and len({c["id"] for c in cases}) == 192, "pool coverage")
    old = load(ROOT / "research/holdouts/ATTR-HTTP-PREFIX-OPTION-LOSS-321.json")
    # Build POSIX paths explicitly when freezing on Windows.
    vm = {(p if p.startswith("/") else "/home/LMC/udon321-0905/"+p):h for p,h in old["vm_hashes"].items()}
    write_new(M, {"experiment": ID, "parent":"c76a8ea", "cases": cases, "local_hashes":{p:digest(ROOT/p) for p in sorted(paths)},
        "vm_hashes":vm, "runner_sha256":digest(Path(__file__)),
        "helper_sha256":digest(ROOT/"research/probes/http_prefix_option_loss_321.py"),
        "probe_sha256":old["vm_hashes"]["probe"], "bridge":parent["bridge"], "min_mem_available_kib":4*1024*1024,
        "gate":"All192 complete384 suffix days dual-valid; exact selected/ceiling identity; bound soundness; "
               "existing better actions on2 loss cases2 families permit source work only.",
        "holdout_authority":False, "production_change":False, "score_successor_authorized":False})
    print(json.dumps({"manifest_sha256":digest(M),"requests":192,"runner_sha256":digest(Path(__file__))}))

def execute(manifest, expected, directory, probe):
    import fcntl
    m = load(manifest)
    require(digest(manifest) == expected and digest(Path(__file__)) == m["runner_sha256"], "frozen runner/manifest drift")
    require(digest(Path(__file__).with_name("http_prefix_option_loss_321.py")) == m["helper_sha256"], "helper drift")
    for p,h in m["vm_hashes"].items(): require(digest(p)==h,"frozen VM input drift: "+p)
    require(digest(probe)==m["probe_sha256"] and len(m["cases"])==192, "probe/count")
    directory.mkdir(exist_ok=True)
    with (directory/"runner.lock").open("a") as lock:
        fcntl.flock(lock, fcntl.LOCK_EX|fcntl.LOCK_NB)
        require(not list(directory.glob("*.partial")), "ambiguous partial; audit before recovery")
        require({p.name for p in directory.glob("*.result.json")} <= {c["id"]+".result.json" for c in m["cases"]}, "unexpected result")
        for case in m["cases"]:
            out = directory/(case["id"]+".result.json")
            if out.exists():
                validate_result(load(out),case,expected,m["probe_sha256"])
                continue
            memory={line.split()[0].rstrip(":"):int(line.split()[1]) for line in Path("/proc/meminfo").read_text().splitlines()}
            require(memory["MemAvailable"]>=m["min_mem_available_kib"], "RAM safety gate before root")
            partial=directory/(case["id"]+".partial"); err=directory/(case["id"]+".stderr")
            require(not err.exists() and not (directory/(case["id"]+".stdout.json")).exists(), "orphaned evidence; no blind duplicate")
            start=time.monotonic()
            with partial.open("x",encoding="utf-8") as stdout, err.open("x",encoding="utf-8") as stderr:
                p=subprocess.run([str(probe)],input=json.dumps(case["request"])+"\n",text=True,stdout=stdout,stderr=stderr)
            require(p.returncode==0 and err.stat().st_size==0,"oracle process failed: "+case["id"])
            result={"id":case["id"],"manifest_sha256":expected,"probe_sha256":m["probe_sha256"],
                    "request_sha256":object_hash(case["request"]),"oracle":load(partial),
                    "elapsed_seconds":time.monotonic()-start,"mem_available_before_kib":memory["MemAvailable"]}
            validate_result(result,case,expected,m["probe_sha256"])
            commit=directory/(case["id"]+".commit.partial")
            write_new(commit,result); commit.rename(out); partial.rename(directory/(case["id"]+".stdout.json"))
            print(json.dumps({"event":"case_complete","id":case["id"],"result_sha256":digest(out)}),flush=True)
        marker={"experiment":ID,"manifest_sha256":expected,"cases":192,
                "results":{p.name:digest(p) for p in sorted(directory.glob("*.result.json"))}}
        require(len(marker["results"])==192,"incomplete")
        if (directory/"run_complete.json").exists():
            require(load(directory/"run_complete.json")==marker,"completion drift")
        else:write_new(directory/"run_complete.json",marker)
        print(json.dumps({"event":"run_complete","cases":192}),flush=True)

def verify_value(case, value):
    require(tuple(value)<=tuple(case["ceiling"]),"pool value exceeds unrestricted321 optimum")
    if case["candidate"]["selected"]:
        require(value==case["selected_value"],"selected conditional value differs from321")
    c=case["candidate"]
    return {"upper_unsound":tuple(scores(c["validUpperBound"]))<tuple(value),
            "certified_lower_unsound":c["certified"] and tuple(scores(c["finalCertifiedLowerBound"]))>tuple(value),
            "provisional_above_optimum":tuple(scores(c["provisionalLowerBound"]))>tuple(value)}

def summarize(manifest,directory,output):
    from run_http_baseline_314 import Bridge
    m,mh=load(manifest),digest(manifest)
    for p,h in m["local_hashes"].items():require(digest(ROOT/p)==h,"local frozen input drift: "+p)
    marker=load(directory/"run_complete.json")
    require(marker["manifest_sha256"]==mh and marker["cases"]==192,"incomplete marker")
    require(set(marker["results"])=={c["id"]+".result.json" for c in m["cases"]}=={p.name for p in directory.glob("*.result.json")},"root coverage")
    require(not list(directory.glob("*.partial")),"partial evidence")
    bridge=Bridge(ROOT/m["bridge"]); by_seed=defaultdict(list); bound_failures=[]; dual_days=0
    try:
        for c in m["cases"]:
            path=directory/(c["id"]+".result.json")
            require(digest(path)==marker["results"][path.name],"atomic result drift")
            r=load(path);validate_result(r,c,mh,m["probe_sha256"])
            agents,ledger=c["request"]["state"]["agents"],c["request"]["ledger"]
            for day in r["oracle"]["days"]:
                step=bridge.request({"op":"step","setup":c["request"]["setup"],"state":state(agents,day["day"]),
                                     "ledger":ledger,"plan":day["plan"]})
                require(step["ok"] and step["agrees"] and all(step[k]==day[k] for k in ("score","agents","ledger")),"suffix reconstruction")
                agents,ledger=step["agents"],step["ledger"];dual_days+=1
            value=r["oracle"]["score"];bounds=verify_value(c,value)
            if any(bounds.values()):bound_failures.append({"id":c["id"],**bounds})
            by_seed[c["seed"]].append({"id":c["id"],"index":c["index"],"candidate":c["candidate"],
                "value":value,"versus_selected":comparison(value,c["selected_value"]),
                "provisional_slack":comparison(value,scores(c["candidate"]["provisionalLowerBound"])),
                "bounds":bounds,"request_sha256":r["request_sha256"],
                "memo_states":r["oracle"]["memo_states"],"joint_transitions":r["oracle"]["joint_transitions"]})
    finally:close_bridge(bridge)
    matches=[]
    for seed,rows in sorted(by_seed.items()):
        c=next(c for c in m["cases"] if c["seed"]==seed)
        best=max((r["value"] for r in rows),key=tuple)
        matches.append({"seed":seed,"family":c["family"],"players":c["players"],
            "global_conditioned":c["ceiling"],"selected":c["selected_value"],"pool_best":best,
            "pool_vs_selected":comparison(best,c["selected_value"]),"global_vs_pool":comparison(c["ceiling"],best),
            "pool_optimal_dispositions":dict(Counter(r["candidate"]["disposition"] for r in rows if r["value"]==best)),
            "candidate_wtl":dict(Counter(r["versus_selected"]["result"] for r in rows)),
            "exact_unique_request_count":len({r["request_sha256"] for r in rows}),"candidates":rows})
    improvements=[r for r in matches if r["pool_vs_selected"]["result"]=="win"]
    require(dual_days==384 and len(matches)==12,"reconstruction coverage")
    report={"experiment":ID,"manifest_sha256":mh,"run_complete_sha256":digest(directory/"run_complete.json"),
        "result_hashes":marker["results"],"roots":192,"dual_validated_suffix_days":dual_days,"zero_reconstruction_failure":True,
        "bound_failures":bound_failures,"matches":matches,
        "existing_action_value_work_authorized":len(improvements)>=2 and len({r["family"] for r in improvements})>=2,
        "score_successor_authorized":False,"holdout_authority":False,"production_change":False,
        "strata":{field:{key:dict(Counter(r["pool_vs_selected"]["result"] for r in matches if str(r[field])==key))
                         for key in sorted({str(r[field]) for r in matches})} for field in ("family","players")}}
    write_new(output,report)
    print(json.dumps({"summary_sha256":digest(output),"pool_improved_matches":len(improvements),
                      "bound_failure_count":len(bound_failures),"existing_action_value_work_authorized":report["existing_action_value_work_authorized"]}))

if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("mode",choices=("freeze","execute","summarize"))
    p.add_argument("--manifest",type=Path,default=M);p.add_argument("--manifest-sha256")
    p.add_argument("--directory",type=Path,default=D);p.add_argument("--probe",type=Path)
    p.add_argument("--output",type=Path,default=ROOT/f"research/evidence/{ID}.summary.json")
    a=p.parse_args()
    if a.mode=="freeze":freeze()
    elif a.mode=="execute":execute(a.manifest.resolve(),a.manifest_sha256,a.directory.resolve(),a.probe.resolve())
    else:summarize(a.manifest,a.directory,a.output)
