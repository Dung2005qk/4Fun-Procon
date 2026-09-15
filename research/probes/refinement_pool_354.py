"""One fixed exact two-plan diagnostic, no timed match or product modifications."""
import argparse
import json
import subprocess
from refinement_cost_353 import ROOT,sha,load,write,require
ID='ATTR-REFINEMENT-POOL-CERTIFICATE-GAP-354'
W=ROOT/'artifacts/research/354'
D=ROOT/f'research/evidence/{ID}'
def prepare():
    source='research/evidence/ATTR-SAME-INPUT-REFINEMENT-COST-353-validated'
    paths=['artifacts/research/353/validated/snapshot.json',source+'/10000-parentA.result.json',source+'/1654-parentA.result.json',
        source+'.summary.json','research/probes/refinement_pool_354.py','research/probes/refinement_pool_354.cpp',
        'research/probes/refinement_cost_353.py',f'research/evidence/{ID}-preregistration.md']
    require(sha(ROOT/(source+'.summary.json'))=='EA7AA4D4759958A7B033FE2003CC659B9E1A2CDBFEB6B7CF3504C2234352A468','353 summary identity')
    q=load(ROOT/paths[0]);a=load(ROOT/paths[1]);b=load(ROOT/paths[2])
    require(a['score']==[6,6,64] and b['score']==[6,6,65] and not a['deadline'] and not b['deadline'],'endpoints')
    q.update(plan64=a['plan'],plan65=b['plan']);write(W/'input.json',q)
    paths.append('artifacts/research/354/input.json')
    write(W/'manifest.json',{'experiment':ID,'hashes':{p:sha(ROOT/p)for p in paths},'mixtures':256,'queries':8,
        'authority':'opened DEV exact attribution; no promotion or cap tuning'})
    print('354 input frozen',sha(W/'manifest.json'))
def vm():
    require(subprocess.check_output(['hostname'],text=True).strip()=='udon-f0-240-0829','authorized VM')
    require(not D.exists() and not (W/'execution.json').exists(),'existing evidence, no blind duplication')
    m=load(W/'manifest.json')
    for p,h in m['hashes'].items():require(sha(ROOT/p)==h,'input/source drift '+p)
    prior=load(ROOT/'execution352.json');lib='build-parent/libudon_shield.a'
    require(sha(ROOT/lib)==prior['hashes'][lib],'library drift')
    for p,h in prior['hashes'].items():
        if p.startswith('parent/'):require(sha(ROOT/p)==h,'parent source drift')
    binary=W/'probe'
    subprocess.run(['g++','-std=c++20','-O3','-DNDEBUG','-pthread','-I',str(ROOT/'parent/include'),
        str(ROOT/'research/probes/refinement_pool_354.cpp'),str(ROOT/lib),'-o',str(binary)],check=True)
    write(W/'execution.json',{'manifest_sha256':sha(W/'manifest.json'),'hashes':{**m['hashes'],lib:sha(ROOT/lib),
        'artifacts/research/354/probe':sha(binary)},'compiler':subprocess.check_output(['g++','--version'],text=True)})
    D.mkdir();p=subprocess.run([str(binary)],input=json.dumps(load(W/'input.json')),capture_output=True,text=True,timeout=45)
    for name,data in [('stdout',p.stdout),('stderr',p.stderr)]:
        with (D/name).open('x')as f:f.write(data)
    require(p.returncode==0 and not p.stderr,'operational failure; preserved raw files')
    row=json.loads(p.stdout);require(row.pop('input_echo')==load(W/'input.json'),'input echo/mutation')
    require(row['ok'] and row['zero_validator_mismatch'],'validator failure')
    require(len(row['mixtures'])==256 and len(row['pools'])==8,'counts')
    write(D/'result.json',row)
    write(D/'run_complete.json',{'execution_sha256':sha(W/'execution.json'),
        'files':{x:sha(D/x)for x in ('stdout','stderr','result.json')},'mixtures':256,'queries':8})
    check()
def check():
    c=load(D/'run_complete.json');require(c['execution_sha256']==sha(W/'execution.json'),'execution identity')
    for p,h in c['files'].items():require(sha(D/p)==h,'raw hash drift')
    raw=load(D/'stdout');require(raw.pop('input_echo')==load(W/'input.json'),'raw input')
    require(raw==load(D/'result.json') and len(raw['mixtures'])==256 and len(raw['pools'])==8,'raw/result')
    require({r['mask']for r in raw['mixtures']}==set(range(256)),'full mask coverage')
    safe=[r for r in raw['mixtures']if r['valid'] and r['dominates64'] and r['score']>[6,6,64]]
    print('354 complete hash',sha(D/'result.json'),'safe improvements',[(r['mask'],r['score'])for r in safe])
    print('endpoint65',raw['mixtures'][255]);print('endpoint64',raw['mixtures'][0])
    for pool in raw['pools']:
        print('pool',pool['agent'],'complete',pool['complete'],'settled',pool['settled'],
              'routes',len(pool['routes']),'contains65',any(r['equals65component']for r in pool['routes']),
              'safe gains',sum(r['valid'] and r['dominates64'] and r['score']>[6,6,64]for r in pool['routes']))
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('prepare','vm','check'));a=p.parse_args()
    {'prepare':prepare,'vm':vm,'check':check}[a.mode]()
