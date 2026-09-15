"""Frozen pair-capability runner; complete results only, no promotion authority."""
import argparse
from collections import Counter
from pathlib import Path
import json
from joint_response_composition_334 import source, M330, S330, D330
from run_http_baseline_314 import ROOT, Bridge, digest
from http_prefix_option_loss_321 import load, require, write_new, close_bridge
from pool_boundary_value_327 import verify_local
from day2_pool_value_323 import comparison

ID="ATTR-W1-PAIR-RESOURCE-RESPONSE-336"
M=ROOT/f"research/holdouts/{ID}.json"
D=ROOT/f"research/evidence/{ID}"
S=ROOT/f"research/evidence/{ID}.summary.json"
PAIRS=[[0,1],[0,2],[1,2]]


def freeze():
    old=source()
    proof=ROOT/"research/evidence/ATTR-W1-REFERENCE-COORDINATION-335.summary.json"
    require(digest(proof)=="DA1440100B2E0D24A71AE16CD91947109E23BC247F42547F22EE463C5991A71C","335 obstruction drift")
    paths={M330,S330,D330/"run_complete.json",proof,Path(__file__),ROOT/"research/probes/pair_resource_response_336.cpp",
        ROOT/"research/probes/test_pair_resource_response_336.py",ROOT/"artifacts/research/336/probe.exe",
        ROOT/"research/probes/joint_response_composition_334.py"}
    paths.update(ROOT/p for p in old["local_hashes"]);paths.update(D330.glob("*.result.json"))
    write_new(M,{"experiment":ID,"parent":"c76a8ea","cases":old["cases"],"probe":"artifacts/research/336/probe.exe",
        "bridge":old["bridge"],"local_hashes":{str(p.relative_to(ROOT)).replace("\\","/"):digest(p) for p in sorted(paths)},
        "memo_limit":250000,"transition_limit":30000000,"expected_roots":27,"expected_responses":81,
        "expected_dual_days":243,"gate":"Complete81 dual-valid responses; pair-best exceeds best-single and original selected certificate on2better-action roots2families. Separate fresh prevalence only.",
        "production_change":False,"holdout_authority":False})
    print("manifest_sha256="+digest(M),flush=True)


def validate(out,c,bridge,singles):
    require(out.get("ok") and out.get("complete") and out["baseline"]==c["baseline"] and
        [r["pair"] for r in out["responses"]]==PAIRS,"pair response incomplete or wrong control")
    dual=0
    for r in out["responses"]:
        require(r["complete"] and [d["day"] for d in r["days"]]==[2,3,4] and
            tuple(c["baseline"])<=tuple(r["score"])<=tuple(c["exact_value"]),"pair control/bounds")
        require(type(r["memo_states"]) is int and 0<=r["memo_states"]<=250000 and
            type(r["transitions"]) is int and 0<=r["transitions"]<=30000000,"work safety ceiling")
        for a in r["pair"]:require(tuple(r["score"])>=tuple(singles[a]["score"]),"pair lost exact singleton domain")
        q=c["request"];state=q["state"];ledger=q["ledger"];fixed=next(a for a in range(3) if a not in r["pair"])
        for i,d in enumerate(r["days"]):
            require(d["plan"][fixed]==q["plans"][i][fixed],"fixed third action changed")
            checked=bridge.request({"op":"step","setup":q["setup"],"state":state,"ledger":ledger,"plan":d["plan"]})
            require(checked.get("ok") and checked["agrees"] and all(checked[k]==d[k] for k in ("score","agents","ledger")),"pair dual identity")
            require(checked["agents"][fixed]==c["fixed_states"][i][fixed],"fixed third state drift")
            state={**state,"day":state["day"]+1,"agents":checked["agents"]};ledger=checked["ledger"];dual+=1
        require(r["score"]==r["days"][-1]["score"],"final pair ledger")
    return dual


def run():
    m=load(M);verify_local(m);D.mkdir(exist_ok=False)
    probe=Bridge(ROOT/m["probe"]);judge=Bridge(ROOT/m["bridge"])
    try:
        for i,c in enumerate(m["cases"]):
            q={**c["request"],"memo_limit":m["memo_limit"],"transition_limit":m["transition_limit"]}
            out=probe.request(q)
            # Keep any exhaustion/error separately; never claim complete or silently skip.
            if not out.get("ok") or not out.get("complete"):
                write_new(D/(c["id"]+".incomplete.json"),{"id":c["id"],"output":out,"manifest_sha256":digest(M)})
                raise RuntimeError("registered pair response incomplete; preserve evidence and do not raise caps")
            singles=load(D330/(c["id"]+".result.json"))["output"]["responses"]
            validate(out,c,judge,singles)
            write_new(D/(c["id"]+".result.json"),{"id":c["id"],"request":q,"output":out,"manifest_sha256":digest(M)})
            print(f"case_complete count={i+1} responses={(i+1)*3}",flush=True)
    finally:close_bridge(probe);close_bridge(judge)
    verify_local(m)
    write_new(D/"run_complete.json",{"experiment":ID,"roots":27,"responses":81,"manifest_sha256":digest(M),
        "result_hashes":{p.name:digest(p) for p in sorted(D.glob("*.result.json"))}})
    summarize()


def summarize():
    m=load(M);verify_local(m);mark=load(D/"run_complete.json")
    require(mark["roots"]==27 and mark["responses"]==81 and mark["manifest_sha256"]==digest(M),"completion")
    require(set(mark["result_hashes"])=={c["id"]+".result.json" for c in m["cases"]}=={p.name for p in D.glob("*.result.json")},"inventory")
    require(not list(D.glob("*.incomplete.json")),"incomplete evidence")
    selected={c["seed"]:c["baseline"] for c in m["cases"] if c["audit"]["selected"]}
    bridge=Bridge(ROOT/m["bridge"]);rows=[];dual=0
    try:
        for c in m["cases"]:
            p=D/(c["id"]+".result.json");require(digest(p)==mark["result_hashes"][p.name],"result drift")
            row=load(p);out=row["output"]
            require(row["manifest_sha256"]==digest(M),"provenance")
            singles=load(D330/(c["id"]+".result.json"))["output"]["responses"]
            dual+=validate(out,c,bridge,singles)
            best=max((r["score"] for r in out["responses"]),key=tuple);single=max((r["score"] for r in singles),key=tuple)
            rows.append({"id":c["id"],"seed":c["seed"],"family":c["family"],"players":c["players"],
                "original":c["baseline"],"best_single":single,"best_pair":best,"selected_certificate":selected[c["seed"]],
                "exact_value":c["exact_value"],"better_action":tuple(c["exact_value"])>tuple(c["selected_exact_value"]),
                "versus_single":comparison(best,single),"versus_selected_certificate":comparison(best,selected[c["seed"]]),
                "responses":out["responses"]})
    finally:close_bridge(bridge)
    require(dual==243,"dual coverage")
    qualified=[r for r in rows if r["better_action"] and r["versus_single"]["result"]=="win" and r["versus_selected_certificate"]["result"]=="win"]
    report={"experiment":ID,"complete":True,"roots":27,"responses":81,"dual_days":dual,"zero_failure":True,
        "gate_passed":len({r["seed"] for r in qualified})>=2 and len({r["family"] for r in qualified})>=2,
        "qualified":[r["id"] for r in qualified],"versus_single":dict(Counter(r["versus_single"]["result"] for r in rows)),
        "strata":{k:{str(v):dict(Counter(r["versus_single"]["result"] for r in rows if r[k]==v)) for v in sorted({r[k] for r in rows})} for k in ("family","players")},
        "rows":rows,"manifest_sha256":digest(M),"run_complete_sha256":digest(D/"run_complete.json"),
        "result_hashes":mark["result_hashes"],"production_change":False,"score_promotion_authority":False}
    write_new(S,report);print(json.dumps({k:report[k] for k in ("complete","roots","responses","dual_days","gate_passed","qualified","versus_single")}))
    print("summary_sha256="+digest(S),flush=True)


if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("mode",choices=("freeze","run","summarize"));args=p.parse_args()
    {"freeze":freeze,"run":run,"summarize":summarize}[args.mode]()
