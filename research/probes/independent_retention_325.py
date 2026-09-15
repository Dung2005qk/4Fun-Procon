"""Complete pre-prune attribution; respects canonical atomic bundle identity."""
import argparse
import itertools
import json
from collections import Counter
from run_http_baseline_314 import ROOT,Bridge,digest
from http_prefix_option_loss_321 import load,require,write_new,close_bridge
from summarize_http_baseline_314 import summarize as lifecycle
from summarize_http_witness_315 import state_key
import pre_f0_attribution_324 as base
from build_pre_prune_capture_325 import HOOKS,transform

ID="ATTR-INDEPENDENT-COLUMN-RETENTION-325"
M=ROOT/f"research/holdouts/{ID}.json"
D=ROOT/f"research/evidence/{ID}"
S=ROOT/f"research/evidence/{ID}.summary.json"

def bundle_admissible(values):
    return all(v<0 for v in values) or (values[0]>=0 and all(v==values[0] for v in values))

def compatible_assignments(by_agent,bridge,request,target):
    found=[]
    for mapping in itertools.permutations(range(3)):
        options=[[r for r in by_agent[a] if mapping[a] in r["optimal_agent_matches"]] for a in range(3)]
        if not all(options): continue
        modes=set(-1 if r["column"]["contingencyBundle"]<0 else r["column"]["contingencyBundle"] for r in options[0])
        for mode in sorted(modes):
            selected=[next((r for r in opts if (-1 if r["column"]["contingencyBundle"]<0 else r["column"]["contingencyBundle"])==mode),None) for opts in options]
            if not all(selected): continue
            require(bundle_admissible([r["column"]["contingencyBundle"] for r in selected]),"bundle classification")
            plan=[r["column"]["plan"][0] for r in selected]
            actual=base.checked_step(bridge,request,plan)
            require(base.same_outcome(actual,target,True),"bundle-feature team validity")
            found.append({"mode":mode,"mapping":mapping,"columnIds":[r["column"]["columnId"] for r in selected],
                "plan":plan,"actual":actual,"physical_match":base.same_outcome(actual,target)})
    return found

def freeze():
    m=load(base.M);base.verify(m)
    require(digest(base.S)=="AEAD4273510F7746BBCB5D574B0236F2F4099942D4211BC4C96657FDD4B23582","324 summary")
    paths=set(m["hashes"])|{str(base.M.relative_to(ROOT)).replace("\\","/"),str(base.S.relative_to(ROOT)).replace("\\","/"),
        "artifacts/research/325/capture_btc.exe","artifacts/research/325/bundle_contract.exe","artifacts/research/325/source-roundtrip.json",
        *[f"research/probes/{n}" for n in ("pre_prune_capture_325.hpp","build_pre_prune_capture_325.py","independent_retention_325.py",
                                          "test_independent_retention_325.py","bundle_mode_contract_325.cpp")],
        *[f"artifacts/research/325/{n}.capture325.cpp" for n in HOOKS]}
    for name,hooks in HOOKS.items():
        require(transform((ROOT/f"src/{name}.cpp").read_text(),hooks)==(ROOT/f"artifacts/research/325/{name}.capture325.cpp").read_text(),"source roundtrip")
    m.update({"experiment":ID,"btc_binary":"artifacts/research/325/capture_btc.exe",
        "hashes":{p:digest(ROOT/p) for p in sorted(paths)},
        "gate":"All12 captures48actions36transitions and exact314 historical identity. All raw/retained columns and pools dual-valid. "
               ">=2 matching loss roots with raw independent/bundle-admissible optimal features lost by retention permits new invariant design only.",
        "limits":"Consumed narrow synthetic development; no bundle unlocking or policy changes. Single optimum membership is sufficient, not necessary. "
                 "Copy overhead not equivalent; drifted roots excluded without reruns. No SCORE promotion or holdout authority."})
    write_new(M,m)
    print(json.dumps({"manifest_sha256":digest(M),"cases":12,"hashes":len(m["hashes"])}))

def run():
    # Frozen324 loopback runner, different registered manifest/output only.
    base.M,base.D=M,D
    base.run()

def summarize():
    m=load(M);base.verify(m)
    operational=lifecycle(M,D)  # Completion gate precedes candidate inspection.
    reference={r["seed"]:r for r in load(ROOT/base.M322)["cases"]}
    bridge=Bridge(ROOT/m["claims_bridge"])
    results=[]
    try:
        for spec in m["cases"]:
            seed=spec["seed"]
            decisions=base.decisions(D/f"{seed}.replay.jsonl")
            old_dec=base.decisions(ROOT/f"{base.D314}/{seed}.replay.jsonl")
            old_transport=load(ROOT/f"{base.D314}/{seed}.transport.json")
            transport=load(D/f"{seed}.transport.json")
            require([b["capture324"]["active"] for b in decisions]==[False,True,False,False],"capture day scope")
            body=decisions[1];capture=body["capture324"]
            require(set(capture["portfolios"])=={"legacy","merged","raw-legacy","raw-expanded"} and
                set(capture["pools"])=={"initial-master","legacy-master","pre-f0"},"capture boundaries")
            checks,qualified=base.qualifying(old_transport,transport,old_dec[1],body)
            require(capture["day"]==2 and state_key(capture["agents"],capture["ledger"])==state_key(body["state"]["agents"],body["ledger"]),"collector root")
            ref=reference[seed]
            request={"setup":ref["request"]["setup"],"state":body["state"],"ledger":body["ledger"]}
            target=base.checked_step(bridge,request,ref["oracle"]["days"][0]["plan"] if qualified else transport["actions"][1]["plan"])
            if qualified: require(base.same_outcome(target,ref["oracle"]["days"][0]),"oracle conditioned identity")
            portfolios={label:base.columns_rows(value,bridge,request,target) for label,value in capture["portfolios"].items()}
            for value in portfolios.values():
                value["canonical_admissible_assignments"]=compatible_assignments(value["by_agent"],bridge,request,target)
            pools={label:base.pool_rows(value,bridge,request,target) for label,value in capture["pools"].items()}
            pools["final-audit"]=base.pool_rows([{"stableId":c["stableId"],"plan":json.loads(c["stableId"])}
                for c in body["decision"]["audit"]["candidates"]],bridge,request,target)
            signature={k:{"columns":sum(len(a) for a in p["by_agent"]),
                "any_feature_assembly":len(p["optimal_feature_assignments"]),
                "independent":sum(a["mode"]<0 for a in p["canonical_admissible_assignments"]),
                "atomic_bundle":sum(a["mode"]>=0 for a in p["canonical_admissible_assignments"])} for k,p in portfolios.items()}
            raw_has=any(signature[k]["independent"] or signature[k]["atomic_bundle"] for k in ("raw-legacy","raw-expanded"))
            retained_has=any(signature[k]["independent"] or signature[k]["atomic_bundle"] for k in ("legacy","merged"))
            lost=qualified and ref["option_loss"]["first_tier"]>0 and raw_has and not retained_has
            results.append({"seed":seed,"family":spec["family"],"players":spec["players"],"qualified":qualified,
                "qualification_checks":checks,"target_is_historical_optimal_witness":qualified,
                "historical_day2_loss":ref["option_loss"],"target":target,"portfolios":portfolios,"pools":pools,
                "signature":signature,"raw_to_retained_witness_loss":lost,
                "same_final_pool_ids":[c["stableId"] for c in old_dec[1]["decision"]["audit"]["candidates"]]==[c["stableId"] for c in body["decision"]["audit"]["candidates"]]})
    finally: close_bridge(bridge)
    lost=[r for r in results if r["raw_to_retained_witness_loss"]]
    metadata=sum(len(p["metadata_mismatches"]) for r in results for p in r["portfolios"].values())
    report={"experiment":ID,"manifest_sha256":digest(M),"cases":12,"actions":48,"transitions":36,
        "qualified":sum(r["qualified"] for r in results),"same_final_pool_ids":sum(r["same_final_pool_ids"] for r in results),
        "dual_validated_columns":sum(len(a) for r in results for p in r["portfolios"].values() for a in p["by_agent"]),
        "dual_validated_team_plans":sum(len(p) for r in results for p in r["pools"].values()),
        "column_metadata_mismatches":metadata,"zero_dual_failure":True,"zero_safety_failure":True,
        "raw_to_retained_loss_seeds":[r["seed"] for r in lost],"loss_families":sorted({r["family"] for r in lost}),
        "distinct_retention_invariant_design_authorized":len(lost)>=2 and metadata==0,
        "results":results,"lifecycle":operational,"score_successor_authorized":False,"production_change":False,
        "strata":{f:dict(Counter(str(r["qualified"]) for r in results if r["family"]==f)) for f in sorted({r["family"] for r in results})},
        "limits":m["limits"]}
    write_new(S,report)
    print(json.dumps({k:v for k,v in report.items() if k not in ("results","lifecycle","strata")},indent=2))
    for r in results: print(json.dumps({k:r[k] for k in ("seed","qualified","signature","raw_to_retained_witness_loss")}))
    print("summary_sha256 "+digest(S))

if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("mode",choices=("freeze","run","summarize"))
    {"freeze":freeze,"run":run,"summarize":summarize}[p.parse_args().mode]()
