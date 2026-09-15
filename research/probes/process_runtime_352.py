"""Isolated, sequential process-RPC runtime diagnostic. No actual network access."""
import argparse
from collections import Counter
import json
import os
from pathlib import Path
import queue
import subprocess
import threading
import time
from protected_http_transport_341 import (ROOT,Bridge,ProtectedCase,operational,traffic_for,digest_case)
from run_pair_protected_341 import validate_side
from run_pair_score_341 import load,digest,write_new,require,decisions
from anytime_diagnostics_350 import install
from anytime_score_core_350 import check_causal
from inactive_runtime_aa_342 import compare_runs
install()

ID='ATTR-PORTABLE-RUNTIME-LOSS-REPEAT-352'
INPUT=ROOT/'input352.json'
EXEC=ROOT/'execution352.json'
D=ROOT/f'research/evidence/{ID}'
S=ROOT/f'research/evidence/{ID}.summary.json'
ORDER=('parentA','candidateA','candidateB','parentB')

def verify_file_manifest(path):
    m=load(path)
    for name,h in m['hashes'].items():require(digest(ROOT/name)==h,'352 frozen drift: '+name)
    return m

def dispatch(state,q,match):
    require(q['transportRPC']==352 and type(q['id'])is int and q['id']>0,'RPC identity')
    prefix=f'/api/v1/matches/{match}/';require(q['path'].startswith(prefix),'RPC match path')
    endpoint=q['path'][len(prefix):]
    require(endpoint in ('setup','assignment','start','state','actions','result'),'RPC endpoint')
    require(type(q['ioTimeoutMs'])is int and q['ioTimeoutMs']>=0,'RPC timeout')
    if q['method']=='GET':
        require(q['hasBody'] is False and q['bodyRaw']=='','GET body');status,body=state.get(endpoint)
    else:
        require(q['method']=='POST' and q['hasBody'] is True and 0<len(q['bodyRaw'].encode())<=1048576,'POST body')
        status,body=state.post(endpoint,json.loads(q['bodyRaw']))
    return {'id':q['id'],'status':status,'bodyRaw':json.dumps(body,separators=(',',':'))}

def side_parts(label):return label[-1],label[:-1]
def location(case,label):
    repeat,side=side_parts(label);return D/repeat/side/str(case['seed'])

def run_one(case,binary,prefix,bridge,side):
    require(not list(prefix.parent.glob(prefix.name+'.*')),'ambiguous partial side; never duplicate')
    state=ProtectedCase(case,bridge);replay=prefix.with_suffix('.replay.jsonl')
    events=[('synthetic_fixture',{'experiment':ID,'seed':case['seed'],'transport':'process-rpc'})]
    if state.roles is not None:events += [('assignment',state.roles),('assignment_result',{'valid':True,'synthetic':True})]
    with replay.open('x',encoding='utf-8',newline='\n') as f:
        for k,b in events:f.write(json.dumps({'kind':k,'body':b,'status':200,'atUnixMs':int(time.time()*1000)})+'\n')
    match=f"m-{case['seed']}";env=os.environ.copy();env['HEXUDON_TOKEN']='synthetic-loopback-only-no-credential'
    command=[str(binary),'http','--url','http://127.0.0.1:352','--match',match,'--response-ms','5000','--poll-ms','220','--replay',str(replay)]
    output=queue.Queue();p=None;last_id=0;error=None;started=time.monotonic()
    with prefix.with_suffix('.stdout').open('x',encoding='utf-8') as out,prefix.with_suffix('.stderr').open('x',encoding='utf-8') as err:
        p=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=env)
        print('side_started seed='+str(case['seed'])+' side='+side+' pid='+str(p.pid),flush=True)
        def read():
            for line in p.stdout:output.put(line)
            output.put(None)
        thread=threading.Thread(target=read,daemon=True);thread.start()
        try:
            while True:
                remaining=sum(case['setup']['daySeconds'])+90-(time.monotonic()-started)
                require(remaining>0,'352 process exceeded frozen wall budget')
                line=output.get(timeout=min(remaining,30))
                if line is None:break
                q=json.loads(line)
                if q.get('transportRPC')!=352:
                    require(q.get('synthetic') is True and state.day==case['days'],'unexpected non-RPC output')
                    out.write(line);out.flush();continue
                require(q['id']==last_id+1,'RPC sequence');last_id=q['id']
                try:r=dispatch(state,q,match)
                except Exception as e:
                    state.failure=str(e);raise
                state.requests.append({'method':q['method'],'endpoint':q['path'].rsplit('/',1)[1],
                    'status':r['status'],'wire_day':state.day,'at_ms':int(state.wall()*1000),
                    'ioTimeoutMs':q['ioTimeoutMs'],'request_body':q['bodyRaw']})
                p.stdin.write(json.dumps(r,separators=(',',':'))+'\n');p.stdin.flush()
            p.wait(timeout=10)
        except Exception as e:error=str(e)
        finally:
            # No kill/relaunch of an ambiguous accepted day. EOF releases I/O only.
            p.stdin.close();thread.join(timeout=1)
    transport={'synthetic':True,'transport':'process-rpc','case':case,'actions':state.actions,
        'requests':state.requests,'roles':state.roles,'assignment_posts':state.assignment_posts,
        'own_road_footprints':state.own,'failure':error or state.failure,'score':state.score}
    write_new(prefix.with_suffix('.transport.json'),transport)
    require(error is None and p.poll()==0 and state.failure is None,'RPC operational failure: '+str(error))
    p.stdout.close()
    require(prefix.with_suffix('.stderr').stat().st_size==0,'runtime stderr')
    require(state.day==case['days'] and len(state.actions)==case['days'],'full lifecycle')
    operational(replay,case,side)
    if side=='candidate':
        for day in decisions(replay):check_causal(day)
    check=subprocess.run([str(binary),'replay-check','--replay',str(replay)],capture_output=True,text=True,timeout=60)
    with prefix.with_suffix('.replay-check.txt').open('x',encoding='utf-8') as f:f.write(check.stdout+check.stderr)
    require(check.returncode==0 and f"summary days={case['days']} reconciled_transitions={case['days']-1}" in check.stdout,'replay lifecycle')
    write_new(prefix.with_suffix('.result.json'),{'kind':'case_complete','seed':case['seed'],
        'case_sha256':digest_case(case),'http_score':state.score,'actions':case['days'],'transitions':case['days']-1,'failure':None,
        'replay_sha256':digest(replay),'transport_sha256':digest(prefix.with_suffix('.transport.json')),
        'replay_check_sha256':digest(prefix.with_suffix('.replay-check.txt'))})
    validate_side(case,side,prefix.parent.parent)

def contracts():
    import unittest
    import test_protected_http_341 as t
    # Existing pure traffic/ACK contracts; no old Windows-only artifact assertions.
    names=['test_fixed_roles_from_day_one','test_native_real_assignment_before_state','test_invalid_roles_do_not_mutate',
        'test_native_assignment_idempotent_before_start_only','test_variable_windows_and_ten_day_completion',
        'test_late_or_changed_action_never_commits','test_unanswered_day_and_invalid_plan_fail_closed',
        'test_source_cost_and_own_road_footprint_are_exact','test_two_day_traffic_thresholds_and_expiration',
        'test_team_normalization_and_exact_threshold_boundary','test_state_traffic_frozen_for_open_day']
    t.ProtectedContract.setUpClass=classmethod(lambda cls:setattr(cls,'bridge',Bridge(ROOT/'bridge352')))
    result=unittest.TextTestRunner(verbosity=1).run(unittest.TestSuite(t.ProtectedContract(n) for n in names))
    require(result.wasSuccessful(),'inherited transport contracts')
    with BridgeContext() as bridge:
        for side in ('parent','candidate'):
            case=t.contract_case('native' if side=='candidate' else 'fixed-all-Patrol')
            folder=ROOT/'contracts352'/side;folder.mkdir(parents=True)
            run_one(case,ROOT/f'build-{side}/udonshield_btc',folder/str(case['seed']),bridge,side)
    write_new(ROOT/'contracts352.json',{'passed':True,'inherited_contracts':len(names),'full_policy_matches':2,
        'ack':8,'transitions':6,'case':'synthetic tiny traffic/window contract; no diagnostic gameplay consumed',
        'files':{p.relative_to(ROOT).as_posix():digest(p) for p in (ROOT/'contracts352').rglob('*') if p.is_file()}})

class BridgeContext:
    def __enter__(self):self.b=Bridge(ROOT/'bridge352');return self.b
    def __exit__(self,*args):
        self.b.close();self.b.process.stdout.close();self.b.process.stderr.close()

def freeze_execution():
    base=verify_file_manifest(ROOT/'package352.manifest.json')
    require(load(ROOT/'contracts352.json')['passed'],'contract gate')
    paths=['input352.json','package352.manifest.json','contracts352.json','bridge352',
        'build-parent/udonshield_btc','build-candidate/udonshield_btc','build-parent/libudon_shield.a',
        'build-candidate/libudon_shield.a','build-parent/CMakeCache.txt','build-candidate/CMakeCache.txt']
    write_new(EXEC,{'experiment':ID,'hashes':{**base['hashes'],**{p:digest(ROOT/p) for p in paths}},
        'hostname':subprocess.check_output(['hostname'],text=True).strip(),'parent_commit':load(INPUT)['parent_commit']})
    print('execution_frozen sha256='+digest(EXEC),flush=True)

def run(resume):
    require(subprocess.check_output(['hostname'],text=True).strip()=='udon-f0-240-0829','authorized host only')
    verify_file_manifest(EXEC);m=load(INPUT)
    if not resume:require(not D.exists(),'run already exists')
    D.mkdir(parents=True,exist_ok=True)
    for repeat in ('A','B'):
        for side in ('parent','candidate'):(D/repeat/side).mkdir(exist_ok=True,parents=True)
    for c in m['cases']:
        for label in ORDER:
            p=location(c,label);repeat,side=side_parts(label)
            if p.with_suffix('.result.json').exists():validate_side(c,side,D/repeat)
            else:require(not list(p.parent.glob(p.name+'.*')),'ambiguous partial case')
    with BridgeContext() as bridge:
        for c in m['cases']:
            for label in ORDER:
                p=location(c,label);repeat,side=side_parts(label)
                if not p.with_suffix('.result.json').exists():
                    run_one(c,ROOT/f'build-{side}/udonshield_btc',p,bridge,side)
                print('case_complete seed='+str(c['seed'])+' label='+label,flush=True)
            marker={'seed':c['seed'],'results':{s:digest(location(c,s).with_suffix('.result.json')) for s in ORDER}}
            path=D/f"{c['seed']}.fixture_complete.json"
            if path.exists():require(load(path)==marker,'fixture marker drift')
            else:write_new(path,marker)
    verify_file_manifest(EXEC)
    completion={'results':12,'actions':96,'transitions':84,'execution_sha256':digest(EXEC),
        'fixtures':{p.name:digest(p) for p in D.glob('*.fixture_complete.json')}}
    write_new(D/'run_complete.json',completion)
    print('run_complete results=12 actions=96 transitions=84',flush=True)
    summarize(False)

def summarize(check):
    verify_file_manifest(EXEC);m=load(INPUT);complete=load(D/'run_complete.json')
    require(complete['execution_sha256']==digest(EXEC),'completion execution')
    rows=[];controls=[]
    for c in m['cases']:
        transports={}
        for label in ORDER:
            repeat,side=side_parts(label);validate_side(c,side,D/repeat)
            transports[label]=load(location(c,label).with_suffix('.transport.json'))
        for repeat in ('A','B'):
            comparison=compare_runs(transports['parent'+repeat],transports['candidate'+repeat])
            days=decisions(location(c,'candidate'+repeat).with_suffix('.replay.jsonl'))
            activation=next((i+1 for i,d in enumerate(days) if check_causal(d)['replacements']),None)
            rows.append({'seed':c['seed'],'repeat':repeat,'family':c['family'],'role':c['role'],'window_ms':c['window_ms'],
                **comparison,'first_pricing_gain_day':activation,'pricing_before_divergence':activation is not None and
                comparison['first_divergence'] is not None and activation<=comparison['first_divergence']})
        for side in ('parent','candidate'):controls.append({'seed':c['seed'],'side':side,**compare_runs(transports[side+'A'],transports[side+'B'])})
    report={'experiment':ID,'complete':True,'execution_sha256':digest(EXEC),'completion_sha256':digest(D/'run_complete.json'),
        'results':12,'actions':96,'transitions':84,'paired_rows':rows,'repeatability':controls,
        'wtl':dict(Counter(r['comparison_B_vs_A'] for r in rows)),
        'first_tiers':dict(Counter(str(r['first_tier']) for r in rows)),
        'zero_safety_failure':True,'authority':load(INPUT)['authority'],'350_verdict_unchanged':'rejected'}
    if check:require(load(S)==report,'summary drift')
    else:write_new(S,report)
    print('summary_complete sha256='+digest(S),flush=True)

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('contracts','freeze','run','summarize'));p.add_argument('--resume',action='store_true');p.add_argument('--check',action='store_true');a=p.parse_args()
    if a.mode=='contracts':contracts()
    elif a.mode=='freeze':freeze_execution()
    elif a.mode=='run':run(a.resume)
    else:summarize(a.check)
