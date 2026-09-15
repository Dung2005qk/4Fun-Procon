"""One frozen resource-conditioned marginal capability call, not promotion."""
import argparse
import json
import subprocess
from refinement_cost_353 import ROOT,sha,load,write,require
ID='ATTR-RESOURCE-CONDITIONED-MARGINAL-CAPABILITY-356'
W=ROOT/'artifacts/research/356'
D=ROOT/f'research/evidence/{ID}'
def prepare():
    paths=['research/probes/resource_marginal_356.cpp','research/probes/resource_marginal_356.py',
        'research/probes/refinement_cost_353.py',f'research/evidence/{ID}-preregistration.md',
        'artifacts/research/354/input.json','research/evidence/ATTR-TARGET-EMISSION-RETENTION-355/observed.json']
    require(sha(ROOT/paths[-1])=='FAB772CE9F5B87F2624456C941B53F0606253EF35379957A3B23B0B3A3F87432','355 identity')
    write(W/'manifest.json',{'experiment':ID,'hashes':{p:sha(ROOT/p)for p in paths},'queries':8,'calls':2})
    print('356 manifest',sha(W/'manifest.json'))
def vm():
    require(subprocess.check_output(['hostname'],text=True).strip()=='udon-f0-240-0829','authorized VM')
    require(not D.exists() and not (W/'execution.json').exists(),'existing evidence')
    m=load(W/'manifest.json')
    for p,h in m['hashes'].items():require(sha(ROOT/p)==h,'frozen drift '+p)
    lib='build-parent/libudon_shield.a';require(sha(ROOT/lib)==load(ROOT/'execution352.json')['hashes'][lib],'parent library')
    subprocess.run(['g++','-std=c++20','-O3','-DNDEBUG','-pthread','-I',str(ROOT/'parent/include'),
        str(ROOT/'research/probes/resource_marginal_356.cpp'),str(ROOT/lib),'-o',str(W/'probe')],check=True)
    write(W/'execution.json',{'manifest_sha256':sha(W/'manifest.json'),'hashes':{**m['hashes'],lib:sha(ROOT/lib),
        'artifacts/research/356/probe':sha(W/'probe')},'compiler':subprocess.check_output(['g++','--version'],text=True)})
    D.mkdir();p=subprocess.run([str(W/'probe')],input=json.dumps(load(ROOT/'artifacts/research/354/input.json')),capture_output=True,text=True,timeout=30)
    for suffix,data in [('stdout',p.stdout),('stderr',p.stderr)]:
        with (D/suffix).open('x')as f:f.write(data)
    require(p.returncode==0 and not p.stderr,'preflight operational failure; preserved')
    r=json.loads(p.stdout);write(D/'result.json',r)
    write(D/'run_complete.json',{'execution_sha256':sha(W/'execution.json'),'files':{p.name:sha(p)for p in D.iterdir()if p.is_file()}})
    check()
def check():
    c=load(D/'run_complete.json');require(sha(W/'execution.json')==c['execution_sha256'],'execution drift')
    for p,h in c['files'].items():require(sha(D/p)==h,'raw drift')
    r=load(D/'result.json');require(r==load(D/'stdout'),'raw mismatch');q=load(ROOT/'artifacts/research/354/input.json')
    for x in r.values():require(x['input_echo']==q and x['zero_validator_mismatch'] and x['input_unmodified'],'input/validator')
    a,b=r['expired'],r['bounded'];require(a['plan']==q['plan64'] and not a['queries'] and a['rollback'],'expired contract')
    require(len(b['queries'])==8 and all(x['settled']<=1250000 and x['marginal_routes']<=32 for x in b['queries']),'query cap')
    print('356 complete',sha(D/'result.json'),'base',b['base_score'],'result',b['score'],'takeover',b['takeover'],'rollback',b['rollback'],'ms',b['ms'])
    print('queries',b['queries'])
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=['prepare','vm','check']);a=p.parse_args();{'prepare':prepare,'vm':vm,'check':check}[a.mode]()
