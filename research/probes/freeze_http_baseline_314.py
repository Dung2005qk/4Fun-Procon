"""Freeze development-only consumed cases; never load the sealed CSV split."""
import json
from pathlib import Path
from run_http_baseline_314 import ROOT, Bridge, digest


def main():
    log = "research/evidence/CEILING-THREE-ACTIVE-PATROL-LOW-FUEL-PREVALENCE-312-development.log"
    if digest(ROOT/log) != "45EEEDD866D22D45AF24D6CD17ECF6C93E9C0FBA9E72FE749DD5AE4BB150C0D5":
        raise RuntimeError("312 source log mismatch")
    cases=[]
    for line in (ROOT/log).read_text().splitlines():
        if line.startswith("case,"):
            fields=dict(part.split("=",1) for part in line.split(",")[1:])
            cases.append({"seed":int(fields["seed"]), "family":fields["family"],
                "players":int(fields["players"]), "oracle_score":list(map(int,fields["oracle"].split("/"))),
                "old_engine_score":list(map(int,fields["head"].split("/")))})
    if len(cases)!=12 or len({c["seed"] for c in cases})!=12:
        raise RuntimeError("incomplete consumed development")
    bridge_path="artifacts/research/314/bridge.exe"
    bridge=Bridge(ROOT/bridge_path)
    try:
        setups=[]
        for case in cases:
            result=bridge.request({"op":"fixture", **case})
            if not result.get("ok"):
                raise RuntimeError(str(result))
            setups.append(result["setup"])
    finally:
        bridge.close()
    paths=[log, "build-release/udonshield_btc.exe", "build-release/udon_shield.lib", bridge_path,
        "research/probes/multi_patrol_oracle.cpp", "research/probes/http_baseline_bridge_314.cpp",
        "research/probes/run_http_baseline_314.py", "research/probes/summarize_http_baseline_314.py",
        "research/probes/freeze_http_baseline_314.py", "research/probes/test_http_baseline_314.py"]
    paths += [str(p.relative_to(ROOT)).replace("\\","/") for folder in ("src","include","strategies/blank_slate")
              for p in sorted((ROOT/folder).rglob("*")) if p.suffix in (".cpp",".hpp")]
    manifest={"experiment":"ATTR-THREE-PATROL-HTTP-BASELINE-314", "parent":"c76a8ea",
        "btc_binary":"build-release/udonshield_btc.exe", "bridge_binary":bridge_path,
        "hashes":{path:digest(ROOT/path) for path in paths}, "cases":cases, "setups":setups,
        "scope":"consumed development only; unchanged HTTP executable; synthetic loopback; fixed all Patrol; no real credential",
        "window_ms":5000, "poll_ms":220, "release_next_day":"prior 5-second window expires",
        "gate":"all12/48actions/36transitions; exact oracle and dual validation; zero safety; no promotion or holdout authority"}
    output=ROOT/"research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json"
    with output.open("x",encoding="utf-8",newline="\n") as stream:
        json.dump(manifest,stream,indent=2,sort_keys=True)
        stream.write("\n")
    print(json.dumps({"manifest":str(output),"sha256":digest(output),"cases":len(cases)},indent=2))


if __name__=="__main__":
    main()
