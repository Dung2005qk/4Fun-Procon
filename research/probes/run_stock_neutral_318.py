"""Frozen complete three-root attribution. Not a production optimization."""
import argparse
import json
from run_http_baseline_314 import ROOT, Bridge, digest
from run_http_witness_315 import require
from replay_witness_suffix_317 import check_hashes, write_new, load, TARGETS, M316

ID="ATTR-W1-STOCK-NEUTRAL-MOTION-318"
MANIFEST=ROOT/f"research/holdouts/{ID}.json"
DIRECTORY=ROOT/f"research/evidence/{ID}"
PROBE="artifacts/research/318/probe.exe"

def freeze():
    parent=load("research/holdouts/ATTR-SHARED-STATE-WITNESS-SUFFIX-317.json")
    check_hashes(parent["hashes"])
    paths=set(parent["hashes"])|{PROBE,"build-release/udon_shield.lib","src/planner.cpp","src/orienteering.cpp",
        "research/holdouts/ATTR-SHARED-STATE-WITNESS-SUFFIX-317.json",
        "research/evidence/ATTR-SHARED-STATE-WITNESS-SUFFIX-317.summary.json",
        "research/probes/w1_stock_neutral_318.cpp","research/probes/run_stock_neutral_318.py",
        "research/probes/test_stock_neutral_318.py"}
    m316=load(M316); previous=load("research/evidence/ATTR-SHARED-STATE-WITNESS-SUFFIX-317.summary.json")
    cases=[]
    for seed,_ in TARGETS:
        c=next(c for c in m316["cases"] if c["seed"]==seed)
        row=next(c for c in previous["results"] if c["seed"]==seed)["days"][0]
        require(row["shared_input"] and row["day"]==2,"shared boundary mismatch")
        before=row["paths"]["w1"]["input"]
        cases.append({"seed":seed,"setup":c["setup"],"state":{"day":2,"endsAt":0,"agents":before["agents"],"others":[],"traffics":[]},
            "ledger":before["ledger"],"plan":row["paths"]["w1"]["plan"],"expected":row["paths"]["w1"]["output"]})
    write_new(MANIFEST,{"experiment":ID,"cases":cases,"probe":PROBE,"hashes":{p:digest(ROOT/p) for p in sorted(paths)},
        "gate":"All3 roots/all3 Patrol WAIT substitutions; exact equality before full W1; compare same-condition regenerated control;2 strict wins for lead only.",
        "production_change":False,"holdout_authority":False})
    print("frozen "+digest(MANIFEST))

def run():
    m=load(MANIFEST);check_hashes(m["hashes"]);DIRECTORY.mkdir(exist_ok=False)
    bridge=Bridge(ROOT/m["probe"])
    try:
        for c in m["cases"]:
            r=bridge.request(c)
            require(r.get("ok"),str(r))
            require(all(r["recorded"][k]==c["expected"][k] for k in ("agents","ledger","score")),"recorded mismatch")
            write_new(DIRECTORY/f'{c["seed"]}.result.json',r)
    finally:
        bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    check_hashes(m["hashes"])
    write_new(DIRECTORY/"run_complete.json",{"cases":3,"manifest_sha256":digest(MANIFEST)})
    print("run_complete cases=3")

def summarize():
    m=load(MANIFEST);check_hashes(m["hashes"])
    require(load(DIRECTORY/"run_complete.json")=={"cases":3,"manifest_sha256":digest(MANIFEST)},"incomplete")
    require(len(list(DIRECTORY.glob("*.result.json")))==3,"inventory mismatch")
    rows=[]
    for c in m["cases"]:
        path=DIRECTORY/f'{c["seed"]}.result.json';r=load(path)
        require(len(r["alternatives"])==3 and [a["agent"] for a in r["alternatives"]]==[0,1,2],"alternative bijection")
        wins=[a["agent"] for a in r["alternatives"] if a["neutral"] and a["certificate"]["score"]>r["regenerated_control"]["score"]]
        rows.append({"seed":c["seed"],"control":r["regenerated_control"]["score"],"strict_winners":wins,
            "alternatives":[{"agent":a["agent"],"neutral":a["neutral"],"current":a["candidate"]["score"],"certificate":a["certificate"]} for a in r["alternatives"]],
            "result_sha256":digest(path)})
    report={"experiment":ID,"cases":3,"rows":rows,"qualified_cases":sum(bool(r["strict_winners"]) for r in rows),
        "manifest_sha256":digest(MANIFEST),"promotion_authority":False,"holdout_authority":False}
    path=ROOT/f"research/evidence/{ID}.summary.json";write_new(path,report)
    print(json.dumps(report,indent=2));print("summary_sha256 "+digest(path))

if __name__=="__main__":
    parser=argparse.ArgumentParser();parser.add_argument("mode",choices=("freeze","run","summarize"))
    {"freeze":freeze,"run":run,"summarize":summarize}[parser.parse_args().mode]()
