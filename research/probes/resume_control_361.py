"""Frozen-byte synthetic restart/expired-window contract, never a score run."""
import copy
import hashlib
import json
import os
from pathlib import Path
import queue
import subprocess
import sys
import threading
import time

WORK = Path('/home/LMC/udon361-0909')
BASE = Path('/home/LMC/udon360-0909')
PACKAGE = Path('/home/LMC/udon357-0909')
ID = 'CONTRACT-LATE-CONTROL-RESUME-361'


def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest().upper()
def load(path): return json.loads(path.read_text())
def write(path, obj):
    with path.open('x') as f:
        json.dump(obj, f, indent=2, sort_keys=True)
        f.write('\n')


def verify():
    assert subprocess.check_output(['hostname'], text=True).strip() == 'udon-f0-240-0829'
    assert sha(BASE/'execution360.json') == '47488178393C4578A1D28795CA2EFDC9329F0B6BE96A75967774446F28FC68F9'
    assert sha(BASE/'result360.json') == 'F61EB2B622550443556D56B1975F011C46A9385ED9F1C1003889E1C608449808'
    for name, h in load(WORK/'stage361.json')['hashes'].items(): assert sha(WORK/name) == h, name
    for manifest, key in (('execution360.json','hashes'),('result360.json','files')):
        for name, h in load(BASE/manifest)[key].items(): assert sha(BASE/name) == h, name
    sys.path.insert(0, str(BASE))
    import late_control_360 as contract
    contract.verify_package(PACKAGE)


def make_server_class(rpc):
    class RestartCase(rpc.ProtectedCase):
        def __init__(self, fixture, bridge, evidence):
            super().__init__(fixture, bridge)
            self.assign(evidence['roles'])
            self.restored = []
            for a in evidence['actions'][:2]:
                assert a['wire_day'] == self.day and a['state']['agents'] == self.agents
                assert a['state']['traffics'] == rpc.traffic_for(self.setup,self.day,self.own,fixture['external_road_footprints'])
                checked = bridge.request({'op':'step','setup':self.setup,
                    'state':dict(a['state'], day=self.day+1, endsAt=a['state']['endsAt']//1000),
                    'ledger':self.ledger,'plan':a['plan']})
                assert checked == a['validated'] and checked.get('ok') and checked.get('agrees')
                self.agents,self.ledger,self.score = checked['agents'],checked['ledger'],checked['score']
                self.own.append(checked['road_footprint']); self.day += 1
                self.restored.append(checked)
            self.expired = None

        def get(self, endpoint):
            if endpoint == 'state' and self.day == 2 and self.deadline is None:
                status, body = super().get(endpoint)
                assert status == 200
                self.ends_at = int(time.time()*1000)-2000
                self.deadline = time.monotonic()-2
                self.last_state['endsAt'] = self.ends_at
                plan = [[-self.setup['daySteps'][2]] for _ in self.agents]
                checked = self.bridge.request({'op':'step','setup':self.setup,
                    'state':dict(self.last_state,day=3,endsAt=self.ends_at//1000),
                    'ledger':self.ledger,'plan':plan})
                assert checked.get('ok') and checked.get('agrees')
                self.pending,self.plan = checked,plan
                self.expired = {'state':copy.deepcopy(self.last_state),'plan':plan,'validated':checked}
                return status,self.last_state
            return super().get(endpoint)

        def post(self, endpoint, body):
            assert endpoint != 'assignment', 'restart reposted the already accepted roles'
            assert self.day == 3, 'restart submitted an old or expired day'
            return super().post(endpoint, body)
    return RestartCase


def prefix_bytes():
    src = BASE/'contracts/mixed/candidate/3419001.replay.jsonl'
    selected = []
    for line in src.read_bytes().splitlines(keepends=True):
        e = json.loads(line)
        if e['kind']=='day_state' and e['body']['day']==2: break
        selected.append(line)
    prefix = b''.join(selected)
    events=[json.loads(s) for s in prefix.splitlines()]
    assert len([e for e in events if e['kind']=='action_result']) == 2
    assert len([e for e in events if e['kind']=='day_state']) == 2
    return prefix


def run_session(link, control, rpc, bridge):
    folder = WORK/'sessions'/(link+'-'+control)
    folder.mkdir(parents=True, exist_ok=False)
    source = load(BASE/'contracts/mixed/candidate/3419001.transport.json')
    state = make_server_class(rpc)(source['case'],bridge,source)
    replay = folder/'replay.jsonl'
    before = prefix_bytes()
    with replay.open('xb') as f: f.write(before)
    trace = folder/'control.txt'
    env = os.environ.copy()
    env['HEXUDON_TOKEN']='synthetic-loopback-only-no-credential'
    env['UDON_RESOURCE_MARGINAL_357']=control
    env['UDON_360_TRACE']=str(trace)
    binary = BASE/('btc360-'+link)
    match = 'm-361-contract'
    command = [str(binary),'http','--url','http://127.0.0.1:352','--match',match,
        '--response-ms','5000','--poll-ms','220','--replay',str(replay)]
    output = queue.Queue(); started=time.monotonic(); requests=[]; error=None; last_id=0
    with (folder/'stdout').open('x') as out, (folder/'stderr').open('x') as err:
        p=subprocess.Popen(command,stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=err,text=True,env=env)
        def read():
            for line in p.stdout: output.put(line)
            output.put(None)
        thread=threading.Thread(target=read,daemon=True);thread.start()
        try:
            while True:
                assert time.monotonic()-started < 100, 'contract wall limit'
                line=output.get(timeout=30)
                if line is None: break
                q=json.loads(line)
                if q.get('transportRPC')!=352:
                    assert q.get('synthetic') is True and state.day==4
                    out.write(line);out.flush();continue
                assert q['id']==last_id+1;last_id=q['id']
                response=rpc.dispatch(state,q,match)
                requests.append({'query':q,'response':response,'wire_day':state.day})
                p.stdin.write(json.dumps(response,separators=(',',':'))+'\n');p.stdin.flush()
            p.wait(timeout=10)
        except Exception as exc: error=repr(exc)
        finally:
            p.stdin.close();thread.join(timeout=1)
    write(folder/'transport.json',{'restored':state.restored,'expired':state.expired,
        'new_actions':state.actions,'requests':requests,'own':state.own,'error':error,
        'exit_code':p.poll(),'day':state.day,'assignment_posts':state.assignment_posts,
        'prefix_sha256':hashlib.sha256(before).hexdigest().upper(),'prefix_bytes':len(before)})
    assert error is None and p.poll()==0, error
    p.stdout.close()
    assert state.day==4 and len(state.actions)==1 and state.actions[0]['wire_day']==3
    assert not state.assignment_posts and len(state.own)==4
    assert (folder/'stderr').stat().st_size==0 and not trace.exists()
    assert replay.read_bytes().startswith(before)
    events=[json.loads(s) for s in replay.read_text().splitlines()]
    assert len([e for e in events if e['kind']=='action_result'])==3
    assert len([e for e in events if e['kind']=='actions_deadline_skip'])==1
    assert len([e for e in events if e['kind']=='actions_server_wait'])==1
    # Original mixed prefix has one resource record; restart must append none.
    assert len([e for e in events if e['kind']=='resource_marginal'])==1
    checked=subprocess.run([str(binary),'replay-check','--replay',str(replay)],capture_output=True,text=True,env=env,timeout=60)
    with (folder/'replay-check.txt').open('x') as f:f.write(checked.stdout+checked.stderr)
    assert checked.returncode==0 and not checked.stderr
    assert 'summary days=4 reconciled_transitions=3' in checked.stdout
    assert 'resume accepted_days=4 last_wire_day=3' in checked.stdout
    assert not trace.exists()
    write(folder/'case_complete.json',{'link':link,'control':control,'restored_days':2,
        'expected_expired_waits':1,'new_acks':1,'resumed_treatment_reads':0,'full_replay_days':4,
        'full_replay_transitions':3,'files':{p.name:sha(p) for p in sorted(folder.iterdir()) if p.is_file()}})
    print('contract_complete',link,control,flush=True)


def run():
    verify()
    assert not (WORK/'sessions').exists(), 'no duplicate/ambiguous restart'
    sys.path.insert(0,str(PACKAGE/'research/probes'))
    import run_resource_357 as frozen
    with frozen.rpc.BridgeContext() as bridge:
        for link in ('plain','probe'):
            for control in ('0','1'):run_session(link,control,frozen.rpc,bridge)
    verify()
    write(WORK/'complete361.json',summary())
    print('complete361',sha(WORK/'complete361.json'),flush=True)


def summary():
    rows=[]
    for link in ('plain','probe'):
        for control in ('0','1'):
            folder=WORK/'sessions'/(link+'-'+control)
            row=load(folder/'case_complete.json')
            for name,h in row['files'].items(): assert sha(folder/name)==h,name
            rows.append(row)
    return {'experiment':ID,'complete':True,'rows':rows,'sessions':4,'new_acks':4,
        'expired_waits':4,'restored_prefix_days':8,'stage_sha256':sha(WORK/'stage361.json'),
        'authority':'synthetic restart and expected expired-window contract only; not score or latency'}


if __name__=='__main__':
    if len(sys.argv)==2 and sys.argv[1]=='check':
        verify();assert load(WORK/'complete361.json')==summary();print('verified361',sha(WORK/'complete361.json'))
    else: run()
