"""Create-only live loopback contract on unit inputs, not protected score cases."""
import json
from protected_http_transport_341 import ROOT,Bridge,run_case,operational,digest,write_new,load,require
from test_protected_http_341 import contract_case
import run_pair_holdout_341 as held


def main():
    m=held.load(held.M);held.verify(m)
    out=ROOT/f'research/evidence/{held.ID}-protected-http-preflight-v2'
    require(not out.exists(),'preflight exists; no silent overwrite/rerun')
    entries=[]
    for i,(side,mode) in enumerate((('parent','native'),('candidate','fixed-all-Patrol'))):
        case=contract_case(mode);case['seed']+=i
        entries.append({'side':side,'case':case,'binary':m[side+'_binary']})
    names=['research/probes/protected_http_transport_341.py','research/probes/protected_http_bridge_341.cpp',
           'research/probes/test_protected_http_341.py','research/probes/preflight_protected_http_341.py',
           'artifacts/research/341/protected-bridge.exe','build-release/udon_shield.lib',
           m['parent_binary'],m['candidate_binary']]
    manifest={'experiment':held.ID,'cases':entries,'hashes':{p:digest(ROOT/p) for p in names},
              'authority':'Contract-only synthetic HTTP: native/fixed assignment, variable5/10/15s days and two-day traffic. No protected score input.'}
    out.mkdir();write_new(out/'inputs.json',manifest)
    bridge=Bridge(ROOT/'artifacts/research/341/protected-bridge.exe')
    try:
        results=[]
        for e in entries:
            directory=out/e['side'];directory.mkdir()
            run_case(e['case'],e['binary'],directory,bridge,held.ID+'-contract-only',e['side'])
            prefix=directory/str(e['case']['seed'])
            telemetry=operational(prefix.with_suffix('.replay.jsonl'),e['case'],e['side'])
            results.append({'side':e['side'],'roles':telemetry['roles'],'days':telemetry['days'],
                            'result_sha256':digest(prefix.with_suffix('.result.json'))})
            print('contract_complete side='+e['side'],flush=True)
        for p,h in manifest['hashes'].items():require(digest(ROOT/p)==h,'contract dependency changed')
        write_new(out/'run_complete.json',{'complete':True,'sides':2,'actions':8,'transitions':6,
            'input_sha256':digest(out/'inputs.json'),'results':results,
            'authority':'Only lifecycle/traffic/transport contract, no score verdict or BTC performance authority.'})
        print('preflight_complete '+digest(out/'run_complete.json'),flush=True)
    finally:bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()


if __name__=='__main__':main()
