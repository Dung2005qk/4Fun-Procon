"""Correct registered expired-day verification; retain frozen 364 live path."""
import copy
import hashlib
import json
from pathlib import Path
import subprocess
import sys
import shutil

import takeover_resume_364 as old

V1_HASH = 'F7AA82A6D67E79C882B609F68A0E4A62C00EAE8250458937235875CFB31B953C'


def validate_decision(d, restored_days):
    assert d['candidate']['simulation']['valid'] and d['timing']['totalMs'] <= 5000
    if d['dayNumber'] == restored_days+1:
        assert d['emergency'] and d['deadline']['totalMs'] == 0
        assert d['audit']['selectionReason'] == 'deadline-emergency'
    else:
        assert d['dayNumber'] == restored_days+2 and not d['emergency']
        assert 0 < d['deadline']['totalMs'] <= 5000


def tests():
    expired = {'dayNumber': 4, 'emergency': True, 'candidate': {'simulation': {'valid': True}},
        'timing': {'totalMs': 34}, 'deadline': {'totalMs': 0}, 'audit': {'selectionReason': 'deadline-emergency'}}
    terminal = copy.deepcopy(expired)
    terminal.update(dayNumber=5, emergency=False, deadline={'totalMs': 5000})
    validate_decision(expired, 3); validate_decision(terminal, 3)
    invalid = []
    x=copy.deepcopy(expired);x['deadline']['totalMs']=1;invalid.append(x)
    x=copy.deepcopy(expired);x['candidate']['simulation']['valid']=False;invalid.append(x)
    x=copy.deepcopy(expired);x['audit']['selectionReason']='other';invalid.append(x)
    x=copy.deepcopy(terminal);x['emergency']=True;invalid.append(x)
    x=copy.deepcopy(terminal);x['dayNumber']=3;invalid.append(x)
    x=copy.deepcopy(terminal);x['timing']['totalMs']=5001;invalid.append(x)
    for x in invalid:
        try: validate_decision(x, 3)
        except AssertionError: pass
        else: raise AssertionError('negative verification control admitted')
    return {'positive': 2, 'adverse': 6}


def verify():
    old.verify()
    assert old.sha(old.WORK/'takeover_resume_364.py') == V1_HASH
    for name,h in old.load(old.WORK/'stage364-v2.json')['hashes'].items():
        assert old.sha(old.WORK/name) == h, name
    assert tests() == {'positive': 2, 'adverse': 6}


def stage():
    assert tests() == {'positive': 2, 'adverse': 6}
    root=Path(__file__).resolve().parents[2]
    folder=root/'artifacts/research/364/stage-v2'
    folder.mkdir(parents=True,exist_ok=False)
    shutil.copyfile(Path(__file__),folder/'takeover_resume_364_v2.py')
    shutil.copyfile(root/'research/evidence/CONTRACT-CERTIFIED-TAKEOVER-RESUME-364-checker-correction.md',folder/'checker-correction.md')
    hashes={p.name:old.sha(p) for p in folder.iterdir() if p.is_file()}
    for name in ('takeover_resume_364.py','input364.json','stage364.json'):
        hashes[name]=old.sha(root/'artifacts/research/364/stage'/name)
    old.write(folder/'stage364-v2.json',{'hashes':hashes,'checker_tests':tests(),'no_first_session_rerun':True})
    print('stage364-v2',old.sha(folder/'stage364-v2.json'))


def validate(session, control, rpc, bridge):
    seed,count=session['seed'],session['restored_days']
    folder=old.WORK/'sessions'/f'{seed}-{control}'
    t=old.load(folder/'transport.json')
    assert not t['error'] and t['exit_code']==0 and t['day']==count+2
    assert not t['assignment_posts'] and len(t['new_actions'])==1
    assert len(t['restored'])==count and len(t['own'])==count+2
    original=old.load(old.OLD_DATA/f'A/candidate/{seed}.transport.json')
    restored=old.server_class(rpc)(original,bridge,count)
    assert t['restored']==restored.restored
    expired=t['expired'];es=expired['state']
    assert es['day']==count and es['agents']==restored.agents
    assert es['traffics']==rpc.traffic_for(restored.setup,count,restored.own,original['case']['external_road_footprints'])
    assert expired['plan']==[[-restored.setup['daySteps'][count]] for _ in restored.agents]
    checked=bridge.request({'op':'step','setup':restored.setup,'state':dict(es,day=count+1,endsAt=es['endsAt']//1000),
        'ledger':restored.ledger,'plan':expired['plan']})
    assert checked==expired['validated'] and checked.get('ok') and checked.get('agrees')
    action=t['new_actions'][0];ns=action['state']
    assert action['wire_day']==count+1 and ns['agents']==checked['agents']
    own=restored.own+[checked['road_footprint']]
    assert ns['traffics']==rpc.traffic_for(restored.setup,count+1,own,original['case']['external_road_footprints'])
    final=bridge.request({'op':'step','setup':restored.setup,'state':dict(ns,day=count+2,endsAt=ns['endsAt']//1000),
        'ledger':checked['ledger'],'plan':action['plan']})
    assert final==action['validated'] and final.get('ok') and final.get('agrees')
    assert t['own']==own+[final['road_footprint']] and action['deadline_margin_ms']>0
    posts=[x for x in t['requests'] if x['query']['method']=='POST']
    assert len(posts)==1 and posts[0]['query']['path'].endswith('/actions') and posts[0]['wire_day']==count+1
    assert posts[0]['response']['status']==200
    before=old.prefix_bytes(old.OLD_DATA/f'A/candidate/{seed}.replay.jsonl',count)
    assert t['prefix_sha256']==session['prefix_sha256']==hashlib.sha256(before).hexdigest().upper()
    assert t['prefix_bytes']==len(before)
    replay=folder/'replay.jsonl';raw=replay.read_bytes()
    assert raw.startswith(before) and (folder/'stderr').stat().st_size==0
    events=[json.loads(line) for line in raw[len(before):].splitlines()]
    kinds=[e['kind'] for e in events]
    for kind in ('action_result','actions','actions_deadline_skip','actions_server_wait','protected_slack'):
        assert kinds.count(kind)==1,kind
    assert kinds.count('decision')==2 and kinds.count('day_state')==2
    assert not set(kinds).intersection(('resource_marginal','actions_fallback','actions_recovery_wait','virtual_parent_dropped'))
    for e in events:
        if e['kind']=='decision':validate_decision(e['body']['decision'],count)
        if e['kind']=='actions_deadline_skip':assert e['body']['day']==count+1
        if e['kind']=='day_state' and e['body']['day']==count:assert e['body']['endsAt']<e['atUnixMs']
        if e['kind']=='protected_slack':assert not any(v for k,v in e['body'].items() if k.endswith('Failure'))
        if e['kind']=='action_result':assert e['body']['valid'] and e['body']['day']==count+2 and e['atUnixMs']<=ns['endsAt']
    checkpath=folder/'replay-check.txt'
    if not checkpath.exists():
        p=subprocess.run([str(old.BASE/'btc360-plain'),'replay-check','--replay',str(replay)],capture_output=True,text=True,timeout=60)
        with checkpath.open('x') as f:f.write(p.stdout+p.stderr)
        assert p.returncode==0 and not p.stderr
    check=checkpath.read_text()
    assert f'summary days={count+2} reconciled_transitions={count+1}' in check
    assert f'resume accepted_days={count+2} last_wire_day={count+1}' in check
    row={'seed':seed,'control':control,'new_acks':1,'expired_waits':1,'restored_days':count,
        'full_days':count+2,'transitions':count+1,'checker':'v2-explicit-expired-negative',
        'files':{p.name:old.sha(p) for p in sorted(folder.iterdir()) if p.is_file() and p.name!='case_complete.json'}}
    marker=folder/'case_complete.json'
    if marker.exists():assert old.load(marker)==row
    else:old.write(marker,row)


def run(check=False):
    verify();m=old.load(old.WORK/'input364.json')
    sys.path.insert(0,str(old.PACKAGE/'research/probes'))
    import run_resource_357 as frozen
    with frozen.rpc.BridgeContext() as bridge:
        for s in m['sessions']:
            for control in m['controls']:
                folder=old.WORK/'sessions'/f"{s['seed']}-{control}"
                if not folder.exists():
                    assert not check,'missing completed session'
                    try:old.run_session(s,control,frozen.rpc,bridge)
                    except AssertionError:
                        # Only a complete successful transport can reach the corrected
                        # strict verifier. Any other partial/failure remains a blocker.
                        assert (folder/'transport.json').exists()
                validate(s,control,frozen.rpc,bridge)
                print('verified_session364',s['seed'],control,flush=True)
    verify()
    report={**old.summary(),'stage_v2_sha256':old.sha(old.WORK/'stage364-v2.json'),
        'checker_tests':tests(),'old_checker_incident_preserved':True,'no_first_session_rerun':True}
    p=old.WORK/'complete364.json'
    if check:assert old.load(p)==report
    else:old.write(p,report)
    print('complete364',old.sha(p),flush=True)


if __name__=='__main__':
    if len(sys.argv)>1 and sys.argv[1]=='test':print(json.dumps(tests()))
    elif len(sys.argv)>1 and sys.argv[1]=='stage':stage()
    else:run(len(sys.argv)>1 and sys.argv[1]=='check')
