"""Full recorded-witness temporal attribution. No solve/HTTP/holdout invocation."""
import argparse
from collections import Counter, defaultdict
import json
from run_http_baseline_314 import ROOT, Bridge, digest
from run_http_witness_315 import require
from replay_witness_suffix_317 import load, check_hashes, write_new
from selected_stock_neutral_319 import validate_complete, selected_request, close_bridge

ID="ATTR-W1-CROSS-DAY-ROUTE-RETIMING-320"
M319="research/holdouts/ATTR-SELECTED-W1-STOCK-NEUTRAL-319.json"
D319=ROOT/"research/evidence/ATTR-SELECTED-W1-STOCK-NEUTRAL-319"
MANIFEST=ROOT/f"research/holdouts/{ID}.json"
DIRECTORY=ROOT/f"research/evidence/{ID}"
PROBE="artifacts/research/320/probe.exe"

def freeze():
    m=load(M319);validate_complete(m)
    paths=set(m["hashes"])|{M319,PROBE,"research/probes/w1_route_retiming_320.cpp",
        "research/probes/route_retiming_320.py","research/probes/test_route_retiming_320.py",
        "research/evidence/ATTR-SELECTED-W1-STOCK-NEUTRAL-319.summary.json"}
    cases=[];bridge=Bridge(ROOT/m["bridge_binary"])
    try:
        for spec,setup in zip(m["cases"],m["setups"],strict=True):
            path=D319/f'{spec["seed"]}.replay.jsonl';paths.add(str(path.relative_to(ROOT)).replace("\\","/"))
            body=next(json.loads(line)["body"] for line in path.read_text().splitlines() if json.loads(line)["kind"]=="decision")
            request,score=selected_request(body,setup,bridge)
            request.pop("plan");request["plans"]=body["decision"]["profile"]["outcomes"][0]["futurePlans"]
            cases.append({"spec":spec,"request":request,"recorded_score":score})
    finally:close_bridge(bridge)
    write_new(MANIFEST,{"experiment":ID,"parent":"c76a8ea","cases":cases,"probe":PROBE,
        "hashes":{p:digest(ROOT/p) for p in sorted(paths)},
        "gate":"24 roots all cuts complete;4 strict gains across2 families above recorded and all feasible normalized controls; zero mismatch; attribution only",
        "production_change":False,"holdout_authority":False})
    print("manifest_sha256 "+digest(MANIFEST))

def execute():
    m=load(MANIFEST);check_hashes(m["hashes"]);DIRECTORY.mkdir(exist_ok=False)
    process=Bridge(ROOT/m["probe"])
    try:
        for case in m["cases"]:
            r=process.request(case["request"]);require(r.get("ok"),str(r))
            require(r["baseline"]["score"]==case["recorded_score"],"recorded witness score mismatch")
            require(r["attempted"]==r["infeasible_duration"]+r["valid"]+r["duplicates"],"cut accounting")
            require(r["valid"]==len(r["rows"]),"result bijection")
            write_new(DIRECTORY/f'{case["spec"]["seed"]}.result.json',{"spec":case["spec"],"output":r})
            print("case_complete "+str(case["spec"]["seed"]),flush=True)
    finally:close_bridge(process)
    check_hashes(m["hashes"])
    write_new(DIRECTORY/"run_complete.json",{"cases":24,"manifest_sha256":digest(MANIFEST)})
    summarize()

def summarize():
    m=load(MANIFEST);check_hashes(m["hashes"])
    require(load(DIRECTORY/"run_complete.json")=={"cases":24,"manifest_sha256":digest(MANIFEST)},"not complete")
    require(len(list(DIRECTORY.glob("*.result.json")))==24,"inventory")
    results=[];strata=defaultdict(Counter);work=Counter();raw=Counter();raw_tails=[]
    for case in m["cases"]:
        path=DIRECTORY/f'{case["spec"]["seed"]}.result.json';r=load(path);o=r["output"]
        base=o["baseline"]["score"]
        normalized=[a["normalized"]["score"] for a in o["controls"] if a["normalized"] is not None]
        threshold=max([base]+normalized);best=o["best"]["score"]
        require(best>=base and o["best"]["agents"]==o["baseline"]["agents"],"best invariant")
        win=best>threshold
        for row in o["rows"]:
            score=row["replay"]["score"];outcome="win" if score>base else "loss" if score<base else "tie"
            raw[outcome]+=1
            if score!=base:
                tier=next(i for i,(a,b) in enumerate(zip(score,base,strict=True)) if a!=b)
                raw[f"tier{tier+1}-{outcome}"]+=1
                raw_tails.append({**case["spec"],"agent":row["agent"],"cuts":row["cuts"],"first_tier":tier+1,"delta":[a-b for a,b in zip(score,base)]})
        for key in ("attempted","infeasible_duration","valid","duplicates","dual_valid_days","final_state_mismatches"):
            work[key]+=o[key]
        for key in (case["spec"]["family"],f'players-{case["spec"]["players"]}'):
            strata[key]["win" if win else "tie"]+=1
        results.append({**case["spec"],"recorded":base,"normalized":normalized,"threshold":threshold,"best":best,
            "qualifies":win,"delta":[a-b for a,b in zip(best,threshold)],"result_sha256":digest(path),
            "best_plan_sha256":__import__("hashlib").sha256(json.dumps(o["best"]["plans"],separators=(",",":")).encode()).hexdigest().upper()})
    wins=[r for r in results if r["qualifies"]];families=sorted({r["family"] for r in wins})
    report={"experiment":ID,"cases":24,"results":results,"work":dict(work),"strata":dict(strata),"raw_outcomes":dict(raw),
        "raw_differences":raw_tails,"qualifying_roots":len(wins),"qualifying_families":families,
        "gate_passed":len(wins)>=4 and len(families)>=2,"zero_safety_failure":True,"manifest_sha256":digest(MANIFEST),
        "promotion_authority":False,"limitations":"Consumed selected-W1 roadless three-Patrol attribution; not closed-loop, traffic, target-host performance or convergence evidence."}
    path=ROOT/f"research/evidence/{ID}.summary.json";write_new(path,report)
    print(json.dumps({k:v for k,v in report.items() if k not in ("results","raw_differences")},indent=2))
    print("summary_sha256 "+digest(path))

if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("mode",choices=("freeze","execute","summarize"))
    {"freeze":freeze,"execute":execute,"summarize":summarize}[p.parse_args().mode]()
