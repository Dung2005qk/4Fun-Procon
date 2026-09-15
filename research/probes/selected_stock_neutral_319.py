"""Fresh selected-witness attribution, using unchanged frozen HTTP and W1 code."""
import argparse
from collections import Counter, defaultdict
import json
from run_http_baseline_314 import ROOT, Bridge, digest, run_case
from run_http_witness_315 import require
from replay_witness_suffix_317 import check_hashes, write_new, load
from summarize_http_baseline_314 import safety
from summarize_http_witness_315 import score_list, state_key

ID="ATTR-SELECTED-W1-STOCK-NEUTRAL-319"
MANIFEST=ROOT/f"research/holdouts/{ID}.json"
DIRECTORY=ROOT/f"research/evidence/{ID}"
FAMILIES=("three-balanced","three-duplicate","three-stock","three-coverage","three-fuel","three-terminal")
BASESEED=202609050000

def close_bridge(process):
    process.close();process.process.stdout.close();process.process.stderr.close()

def freeze():
    old=load("research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json")
    prior=load("research/holdouts/ATTR-W1-STOCK-NEUTRAL-MOTION-318.json")
    check_hashes(old["hashes"]);check_hashes(prior["hashes"])
    paths=set(old["hashes"])|set(prior["hashes"])|{
        "research/probes/selected_stock_neutral_319.py","research/probes/test_selected_stock_neutral_319.py",
        "research/holdouts/ATTR-W1-STOCK-NEUTRAL-MOTION-318.json",
        "research/evidence/ATTR-W1-STOCK-NEUTRAL-MOTION-318.summary.json"}
    cases=[{"seed":BASESEED+100*f+r,"family":family,"players":8+(f+r)%3}
           for f,family in enumerate(FAMILIES) for r in range(4)]
    bridge=Bridge(ROOT/old["bridge_binary"])
    try:
        setups=[]
        for spec in cases:
            generated=bridge.request({"op":"fixture",**spec})
            require(generated.get("ok"),str(generated));setups.append(generated["setup"])
    finally:
        close_bridge(bridge)
    write_new(MANIFEST,{"experiment":ID,"parent":"c76a8ea","cases":cases,"setups":setups,
        "hashes":{p:digest(ROOT/p) for p in sorted(paths)},"btc_binary":old["btc_binary"],
        "bridge_binary":old["bridge_binary"],"probe":"artifacts/research/318/probe.exe",
        "gate":"24 complete matches96 actions72 transitions then24 selected-W1 roots;4 strict-gain roots across2 families;zero safety/mismatch; compare regenerated and recorded certificates.",
        "production_change":False,"holdout_authority":False,"btc_authority":False,
        "scope":"Fresh roadless fixed three-Patrol attribution; local timing is not performance evidence."})
    print("manifest_sha256 "+digest(MANIFEST),flush=True)

def baseline():
    m=load(MANIFEST);check_hashes(m["hashes"]);DIRECTORY.mkdir(exist_ok=False)
    bridge=Bridge(ROOT/m["bridge_binary"])
    try:
        for spec,setup in zip(m["cases"],m["setups"],strict=True):
            generated=bridge.request({"op":"fixture",**spec})
            require(generated.get("ok") and generated["setup"]==setup,"fixture mismatch")
            run_case(spec,setup,m,DIRECTORY,bridge)
            print("case_complete seed="+str(spec["seed"]),flush=True)
    finally:
        close_bridge(bridge)
    check_hashes(m["hashes"])
    write_new(DIRECTORY/"run_complete.json",{"cases":24,"manifest_sha256":digest(MANIFEST)})
    print("run_complete cases=24",flush=True)

def validate_complete(m):
    check_hashes(m["hashes"])
    require(load(DIRECTORY/"run_complete.json")=={"cases":24,"manifest_sha256":digest(MANIFEST)},"incomplete baseline")
    require(len(list(DIRECTORY.glob("*.result.json")))==24,"baseline inventory mismatch")
    for spec in m["cases"]:
        prefix=DIRECTORY/str(spec["seed"]);r=load(prefix.with_suffix(".result.json"))
        require(all(r[k]==v for k,v in spec.items()) and r["actions"]==4 and r["transitions"]==3 and r["failure"] is None,"case identity/safety")
        for suffix,key in ((".replay.jsonl","replay_sha256"),(".transport.json","transport_sha256"),(".replay-check.txt","replay_check_sha256")):
            require(digest(prefix.with_suffix(suffix))==r[key],"artifact changed")
        require(prefix.with_suffix(".stderr").stat().st_size==0,"case stderr")
        safety(prefix.with_suffix(".replay.jsonl"))

def selected_request(body,setup,bridge):
    d=body["decision"];outcomes=d["profile"]["outcomes"]
    require(len(outcomes)==1 and outcomes[0]["certified"] and not outcomes[0]["lowerBoundOnly"] and
            len(outcomes[0]["futurePlans"])==3,"missing selected full W1")
    require(body["state"]["day"]==1 and body["state"]["others"]==[] and body["state"]["traffics"]==[],"scope mismatch")
    current=bridge.request({"op":"step","setup":setup,"state":body["state"],"ledger":body["ledger"],"plan":d["candidate"]["plan"]})
    require(current.get("ok") and current["agrees"] and current["score"]==score_list(d["candidate"]["scoreAfterToday"]),"current-plan mismatch")
    agents,ledger=current["agents"],current["ledger"]
    for day,plan in enumerate(outcomes[0]["futurePlans"],2):
        checked=bridge.request({"op":"step","setup":setup,"state":{"day":day,"endsAt":0,"agents":agents,"others":[],"traffics":[]},"ledger":ledger,"plan":plan})
        require(checked.get("ok") and checked["agrees"],"recorded suffix invalid")
        agents,ledger=checked["agents"],checked["ledger"]
    require(checked["score"]==score_list(outcomes[0]["witnessScore"]),"selected certificate replay mismatch")
    return {"setup":setup,"state":{"day":2,"endsAt":0,"agents":current["agents"],"others":[],"traffics":[]},
        "ledger":current["ledger"],"plan":outcomes[0]["futurePlans"][0]},checked["score"]

def analyze():
    m=load(MANIFEST);validate_complete(m)
    out=DIRECTORY/"attribution";out.mkdir(exist_ok=False)
    bridge=Bridge(ROOT/m["bridge_binary"]);probe=Bridge(ROOT/m["probe"])
    try:
        for spec,setup in zip(m["cases"],m["setups"],strict=True):
            prefix=DIRECTORY/str(spec["seed"])
            events=[json.loads(line) for line in prefix.with_suffix(".replay.jsonl").read_text().splitlines()]
            body=next(e["body"] for e in events if e["kind"]=="decision")
            request,recorded=selected_request(body,setup,bridge)
            result=probe.request(request)
            require(result.get("ok"),str(result))
            checked=bridge.request({"op":"step",**request})
            require(checked.get("ok") and checked["agrees"] and all(checked[k]==result["recorded"][k] for k in ("agents","ledger","score")),"probe root mismatch")
            require([a["agent"] for a in result["alternatives"]]==[0,1,2],"Patrol bijection mismatch")
            write_new(out/f'{spec["seed"]}.result.json',{"spec":spec,"request":request,"recorded_certificate":recorded,"output":result})
    finally:
        close_bridge(bridge);close_bridge(probe)
    check_hashes(m["hashes"])
    write_new(out/"run_complete.json",{"roots":24,"manifest_sha256":digest(MANIFEST)})
    print("attribution_complete roots=24",flush=True)

def summarize():
    m=load(MANIFEST);validate_complete(m);out=DIRECTORY/"attribution"
    require(load(out/"run_complete.json")=={"roots":24,"manifest_sha256":digest(MANIFEST)},"incomplete attribution")
    require(len(list(out.glob("*.result.json")))==24,"attribution inventory mismatch")
    results=[];strata=defaultdict(Counter)
    for spec in m["cases"]:
        path=out/f'{spec["seed"]}.result.json';r=load(path);o=r["output"]
        threshold=max(r["recorded_certificate"],o["regenerated_control"]["score"])
        alternatives=[a["certificate"]["score"] for a in o["alternatives"] if a["neutral"]]
        best=max([threshold]+alternatives)
        win=best>threshold
        strata[spec["family"]]["win" if win else "tie"]+=1
        strata[f'players-{spec["players"]}']["win" if win else "tie"]+=1
        results.append({**spec,"recorded":r["recorded_certificate"],"regenerated":o["regenerated_control"]["score"],"best":best,
            "strict_gain":win,"first_tier":next((i+1 for i,(a,b) in enumerate(zip(best,threshold)) if a!=b),None),
            "delta":[a-b for a,b in zip(best,threshold)],"neutral_count":sum(a["neutral"] for a in o["alternatives"]),"result_sha256":digest(path)})
    wins=sum(r["strict_gain"] for r in results);families={r["family"] for r in results if r["strict_gain"]}
    report={"experiment":ID,"cases":24,"actions":96,"transitions":72,"selected_suffix_dual_valid_days":72,
        "results":results,"strata":dict(strata),"qualifying_roots":wins,"qualifying_families":sorted(families),
        "gate_passed":wins>=4 and len(families)>=2,"manifest_sha256":digest(MANIFEST),"zero_safety_failure":True,
        "promotion_authority":False,"holdout_authority":False,"btc_authority":False,
        "limits":"Fresh selected certificates only, not a candidate-vs-parent closed-loop score measurement. Roadless three-Patrol scope. No local performance claim."}
    path=ROOT/f"research/evidence/{ID}.summary.json";write_new(path,report)
    print(json.dumps({k:v for k,v in report.items() if k!="results"},indent=2));print("summary_sha256 "+digest(path))

if __name__=="__main__":
    parser=argparse.ArgumentParser();parser.add_argument("mode",choices=("freeze","baseline","analyze","summarize","execute"))
    mode=parser.parse_args().mode
    if mode=="execute":
        baseline();analyze();summarize()
    else:
        {"freeze":freeze,"baseline":baseline,"analyze":analyze,"summarize":summarize}[mode]()
