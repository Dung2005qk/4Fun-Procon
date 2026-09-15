"""Certified column-domain mapping and complete consumed-portfolio attribution."""
import argparse
import copy
import json
from pathlib import Path
import subprocess
import time
from http_prefix_option_loss_321 import ROOT, load, digest, object_hash, require, write_new, state, close_bridge

ID="ATTR-CERTIFIED-PORTFOLIO-REPRESENTATIVES-326"
M=ROOT/f"research/holdouts/{ID}.json"
D=ROOT/f"research/evidence/{ID}"
S=ROOT/f"research/evidence/{ID}.summary.json"
S325=ROOT/"research/evidence/ATTR-INDEPENDENT-COLUMN-RETENTION-325.summary.json"
VMROOT="/home/LMC/udon326-0906"
LABELS={"raw-legacy","raw-expanded","legacy","merged","representatives"}

def claims(col):return frozenset(v["spot"] for v in col["firstVisits"] if v["claimed"])
def identity(col):return (col["contingencyBundle"],tuple(col["plan"][0]))
def wait(col):return len(col["plan"][0])==1 and col["plan"][0][0]<0
def dominates(rep,old):
    return rep["agent"]==old["agent"] and rep["contingencyBundle"]<0 and old["contingencyBundle"]<0 and \
        rep["terminalCell"]==old["terminalCell"] and rep["terminalFuel"]>=old["terminalFuel"] and claims(rep)>=claims(old)
def strict(rep,old):return dominates(rep,old) and (rep["terminalFuel"]>old["terminalFuel"] or claims(rep)>claims(old))
def original_order(col):
    brands=sum(1<<b for b in {v["brandIndex"] for v in col["firstVisits"]})
    actions='-1:'+str(col["contingencyBundle"])+':'+''.join(str(a)+',' for a in col["plan"][0])+'|'
    return (-col["priority"],-brands,-col["estimatedServings"],actions,col["columnId"])

def representatives(original,raw):
    result=[];certificates=[];statistics=[]
    for agent,(old_columns,raw_columns) in enumerate(zip(original,raw,strict=True)):
        chosen=[];seen=set();replaced=0
        for old in old_columns:
            rep=old
            if old["contingencyBundle"]<0 and not wait(old):
                candidates=[r for r in raw_columns if strict(r,old)]
                if candidates:
                    rep=max(candidates,key=lambda r:(r["terminalFuel"],len(claims(r)),r["priority"],-r["columnId"]))
            unchanged=rep==old
            require(unchanged or strict(rep,old),"strict resource representative")
            certificates.append({"agent":agent,"original":old,"representative":rep,"unchanged":unchanged})
            replaced+=not unchanged
            if identity(rep) not in seen:chosen.append(rep);seen.add(identity(rep))
        retained=len(chosen)
        for col in sorted(raw_columns,key=original_order):
            if len(chosen)==len(old_columns):break
            if col["contingencyBundle"]<0 and identity(col) not in seen:
                chosen.append(col);seen.add(identity(col))
        require(len(chosen)==len(old_columns),"same retention capacity")
        for cert in (c for c in certificates if c["agent"]==agent):
            require(identity(cert["representative"]) in seen,"unwired representative")
        result.append(chosen)
        statistics.append({"agent":agent,"capacity":len(old_columns),"replaced":replaced,"freed_and_filled":len(chosen)-retained})
    return result,certificates,statistics

def freeze(probe_hash):
    from independent_retention_325 import M as M325, base
    from build_portfolio_oracle_326 import transform
    base.verify(load(M325))
    require(digest(S325)=="25B76905EA5E520DD9B2FA65D2E99552CD674361E629DB9998D17C5B50C8B35F","325 summary")
    report=load(S325);require(report["qualified"]==12 and report["column_metadata_mismatches"]==0,"325 qualification")
    reference=load(ROOT/base.M322);roots={r["seed"]:r for r in reference["cases"]}
    require((ROOT/"artifacts/research/326/portfolio_oracle_326.cpp").read_text()==transform(),"mechanical326 source")
    cases=[]
    for row in report["results"]:
        require(row["qualified"],"no drifted root")
        ref=roots[row["seed"]]
        require(all(a["kind"]==0 for a in ref["request"]["state"]["agents"]),"all Patrol only")
        require(all(cell!=1 for line in ref["request"]["setup"]["map"]["cells"] for cell in line),"roadless only")
        pools={k:[[r["column"] for r in a] for a in p["by_agent"]] for k,p in row["portfolios"].items()}
        reps,certs,stats=representatives(pools["legacy"],pools["raw-legacy"])
        pools["representatives"]=reps
        cases.append({"id":str(row["seed"]),"seed":row["seed"],"family":row["family"],"players":row["players"],
            "ceiling":ref["conditioned_values"][1],"selected":ref["conditioned_values"][2],"historical_loss":ref["option_loss"],
            "mapping_statistics":stats,"request":{**ref["request"],"portfolios":pools,"certificates":certs}})
    paths=set(load(M325)["hashes"])|{str(p.relative_to(ROOT)).replace('\\','/') for p in (M325,S325,
        ROOT/"research/probes/portfolio_representatives_326.py",ROOT/"research/probes/test_portfolio_representatives_326.py",
        ROOT/"research/probes/portfolio_value_326.inc",ROOT/"research/probes/build_portfolio_oracle_326.py",
        ROOT/"research/probes/http_prefix_oracle_321.cpp",ROOT/"research/probes/multi_patrol_oracle.cpp",
        ROOT/"artifacts/research/326/source-roundtrip.json",ROOT/"artifacts/research/326/portfolio_oracle_326.cpp",
        ROOT/"artifacts/research/326/portfolio_oracle.exe")}
    vm={VMROOT+"/portfolio_oracle_326.cpp":digest(ROOT/"artifacts/research/326/portfolio_oracle_326.cpp"),
        "/home/LMC/udon312-0905/research/probes/multi_patrol_oracle.cpp":digest(ROOT/"research/probes/multi_patrol_oracle.cpp"),
        "/home/LMC/udon312-0905/build/libudon_shield.a":"1257982C6A7F048E9437CDD1E235D6478523F8FA4C871A1DFDDF07FB7EDEF11B"}
    write_new(M,{"experiment":ID,"parent":"c76a8ea","cases":cases,"probe_sha256":probe_hash,
        "local_hashes":{p:digest(ROOT/p) for p in sorted(paths)},"vm_hashes":vm,
        "runner_sha256":digest(Path(__file__)),"helper_sha256":digest(ROOT/"research/probes/http_prefix_option_loss_321.py"),
        "bridge":"artifacts/research/314/bridge.exe","min_mem_available_kib":4*1024*1024,
        "gate":"All12 roots5 full-portfolio exact optima each; all180 suffix days dual-valid. Every original column mapped to same-state-or-dominating resource representative. "
               "No transformed-vs-legacy exact loss or value above unrestricted321 optimum. Recovery on>=2 actual loss roots permits fresh SCORE registration only.",
        "limits":"Consumed small roadless all-Patrol attribution only. Mathematical domain dominance is not timed-policy monotonicity. No production or holdout authority."})
    print(json.dumps({"manifest_sha256":digest(M),"cases":12,"portfolios":60,"probe_sha256":probe_hash}))

def validate(result,case,mh,ph):
    require(result["id"]==case["id"] and result["manifest_sha256"]==mh and result["probe_sha256"]==ph and
            result["request_sha256"]==object_hash(case["request"]),"result provenance")
    oracle=result["oracle"]
    require(oracle.get("ok") and oracle.get("complete") and set(oracle["portfolios"])==LABELS,"oracle completion")
    require(all(p["complete"] and len(p["days"])==3 for p in oracle["portfolios"].values()),"full suffix coverage")
    require(oracle["certificates_checked"]==len(case["request"]["certificates"]),"every certificate checked")

def execute(manifest,mh,directory,probe):
    import fcntl
    m=load(manifest)
    require(digest(manifest)==mh and digest(Path(__file__))==m["runner_sha256"],"runner/manifest drift")
    require(digest(Path(__file__).with_name("http_prefix_option_loss_321.py"))==m["helper_sha256"],"helper drift")
    for p,h in m["vm_hashes"].items():require(digest(p)==h,"VM input drift:"+p)
    require(digest(probe)==m["probe_sha256"],"probe drift")
    directory.mkdir(exist_ok=True)
    with (directory/"runner.lock").open("a") as lock:
        fcntl.flock(lock,fcntl.LOCK_EX|fcntl.LOCK_NB)
        require(not list(directory.glob("*.partial")),"ambiguous partial; audit first")
        require({p.name for p in directory.glob("*.result.json")}<={c["id"]+".result.json" for c in m["cases"]},"unknown results")
        for case in m["cases"]:
            out=directory/(case["id"]+".result.json")
            if out.exists():validate(load(out),case,mh,m["probe_sha256"]);continue
            memory={line.split()[0].rstrip(':'):int(line.split()[1]) for line in Path('/proc/meminfo').read_text().splitlines()}
            require(memory["MemAvailable"]>=m["min_mem_available_kib"],"memory safety before root")
            partial=directory/(case["id"]+".partial");err=directory/(case["id"]+".stderr")
            require(not err.exists() and not (directory/(case["id"]+".stdout.json")).exists(),"orphaned evidence")
            started=time.monotonic()
            with partial.open("x",encoding="utf-8") as stdout,err.open("x",encoding="utf-8") as stderr:
                p=subprocess.run([str(probe)],input=json.dumps(case["request"])+"\n",text=True,stdout=stdout,stderr=stderr)
            require(p.returncode==0 and err.stat().st_size==0,"oracle failed:"+case["id"])
            result={"id":case["id"],"manifest_sha256":mh,"probe_sha256":m["probe_sha256"],
                "request_sha256":object_hash(case["request"]),"oracle":load(partial),"elapsed_seconds":time.monotonic()-started}
            validate(result,case,mh,m["probe_sha256"])
            commit=directory/(case["id"]+".commit.partial");write_new(commit,result);commit.rename(out)
            partial.rename(directory/(case["id"]+".stdout.json"))
            print(json.dumps({"event":"case_complete","id":case["id"],"sha256":digest(out)}),flush=True)
        marker={"experiment":ID,"manifest_sha256":mh,"cases":12,"results":{p.name:digest(p) for p in sorted(directory.glob("*.result.json"))}}
        require(len(marker["results"])==12,"complete set")
        if (directory/"run_complete.json").exists():require(load(directory/"run_complete.json")==marker,"marker drift")
        else:write_new(directory/"run_complete.json",marker)
        print(json.dumps({"event":"run_complete","cases":12}),flush=True)

def summarize(manifest,directory,output):
    from run_http_baseline_314 import Bridge
    from day2_pool_value_323 import comparison
    m=load(manifest);mh=digest(manifest);marker=load(directory/"run_complete.json")
    for p,h in m["local_hashes"].items():require(digest(ROOT/p)==h,"local input drift:"+p)
    require(marker["cases"]==12 and marker["manifest_sha256"]==mh,"incomplete marker")
    require(set(marker["results"])=={c["id"]+".result.json" for c in m["cases"]}=={p.name for p in directory.glob("*.result.json")},"exact result set")
    require(not list(directory.glob("*.partial")),"partial evidence")
    bridge=Bridge(ROOT/m["bridge"]);rows=[];days=0
    try:
        for case in m["cases"]:
            path=directory/(case["id"]+".result.json");require(digest(path)==marker["results"][path.name],"result drift")
            result=load(path);validate(result,case,mh,m["probe_sha256"])
            values={}
            for label,pool in result["oracle"]["portfolios"].items():
                agents,ledger=case["request"]["state"]["agents"],case["request"]["ledger"]
                for day in pool["days"]:
                    checked=bridge.request({"op":"step","setup":case["request"]["setup"],"state":state(agents,day["day"]),"ledger":ledger,"plan":day["plan"]})
                    require(checked.get("ok") and checked.get("agrees") and all(checked[k]==day[k] for k in ("score","agents","ledger")),"dual suffix reconstruction")
                    agents,ledger=checked["agents"],checked["ledger"];days+=1
                require(pool["score"]==pool["days"][-1]["score"] and tuple(pool["score"])<=tuple(case["ceiling"]),"unrestricted ceiling")
                values[label]=pool["score"]
            require(tuple(values["representatives"])>=tuple(values["legacy"]),"certificate failed exact future preservation")
            require(tuple(values["raw-legacy"])>=tuple(values["legacy"]),"raw subset relation")
            require(tuple(values["merged"])>=tuple(values["legacy"]),"merged subset relation")
            rows.append({"seed":case["seed"],"family":case["family"],"players":case["players"],"ceiling":case["ceiling"],
                "selected":case["selected"],"values":values,"raw_vs_legacy":comparison(values["raw-legacy"],values["legacy"]),
                "representatives_vs_legacy":comparison(values["representatives"],values["legacy"]),
                "mapping_statistics":case["mapping_statistics"],"historical_loss":case["historical_loss"],"work":result["oracle"]})
    finally:close_bridge(bridge)
    gains=[r for r in rows if r["representatives_vs_legacy"]["result"]=="win" and r["historical_loss"]["first_tier"] is not None]
    require(days==180 and len(rows)==12,"complete reconstruction")
    report={"experiment":ID,"manifest_sha256":mh,"run_complete_sha256":digest(directory/"run_complete.json"),
        "result_hashes":marker["results"],"cases":12,"portfolios":60,"dual_validated_days":days,"zero_failure":True,
        "recovery_seeds":[r["seed"] for r in gains],"fresh_score_registration_qualified":len(gains)>=2,
        "production_change":False,"holdout_authority":False,"results":rows,"limits":m["limits"]}
    write_new(output,report)
    print(json.dumps({k:v for k,v in report.items() if k not in ("results","result_hashes")},indent=2))
    for r in rows:print(json.dumps({k:v for k,v in r.items() if k!="work"}))
    print("summary_sha256 "+digest(output))

if __name__=="__main__":
    p=argparse.ArgumentParser();p.add_argument("mode",choices=("freeze","execute","summarize"))
    p.add_argument("--manifest",type=Path,default=M);p.add_argument("--manifest-sha256");p.add_argument("--probe-hash")
    p.add_argument("--directory",type=Path,default=D);p.add_argument("--probe",type=Path);p.add_argument("--output",type=Path,default=S)
    a=p.parse_args()
    if a.mode=="freeze":freeze(a.probe_hash)
    elif a.mode=="execute":execute(a.manifest,a.manifest_sha256,a.directory,a.probe)
    else:summarize(a.manifest,a.directory,a.output)
