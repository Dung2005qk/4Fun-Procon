"""Read-only local byte and lifecycle cross-check; does not rerun any solver."""
import hashlib
import json
from pathlib import Path

from takeover_resume_364 import prefix_bytes
from takeover_resume_364_v2 import tests, validate_decision

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'artifacts/research/364/completed'
SOURCE=ROOT/'artifacts/research/362/completed/research/evidence/SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362-development'


def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def load(p):return json.loads(p.read_text())


def verify():
    assert sha(SOURCE/'run_complete.json')=='24A5574453D6FD284CABABFBD764479AD65A68AED06E1C298923193DE7CD4791'
    assert sha(WORK/'stage364.json')=='27423C681FB75FEEB81044DD49E09646CE2EFD02F64B174236E45D1D4111B7D9'
    assert sha(WORK/'stage364-v2.json')=='9CC45D351ADA0335F4119EBE046E6C3F2E93B434BA88BA4ABF9E03D72B2496D9'
    for name in ('stage364.json','stage364-v2.json'):
        for path,h in load(WORK/name)['hashes'].items():assert sha(WORK/path)==h,path
    assert sha(WORK/'takeover_resume_364_v2.py')==sha(ROOT/'research/probes/takeover_resume_364_v2.py')
    assert tests()=={'positive':2,'adverse':6}
    m=load(WORK/'input364.json');report=load(WORK/'complete364.json')
    assert report['complete'] and report['no_first_session_rerun']
    assert report['stage_sha256']==sha(WORK/'stage364.json') and report['stage_v2_sha256']==sha(WORK/'stage364-v2.json')
    expected={f"{s['seed']}-{c}" for s in m['sessions'] for c in m['controls']}
    assert {p.name for p in (WORK/'sessions').iterdir()}==expected
    rows=[];terminal_main=[]
    for s in m['sessions']:
        n=s['restored_days'];seed=s['seed']
        for name,h in s['source_hashes'].items():assert sha(SOURCE/name)==h,name
        original=load(SOURCE/f'A/candidate/{seed}.transport.json')
        before=prefix_bytes(SOURCE/f'A/candidate/{seed}.replay.jsonl',n)
        assert len(before)==s['prefix_bytes'] and hashlib.sha256(before).hexdigest().upper()==s['prefix_sha256']
        for control in m['controls']:
            folder=WORK/'sessions'/f'{seed}-{control}'
            row=load(folder/'case_complete.json');rows.append(row)
            assert {p.name for p in folder.iterdir()}==set(row['files'])|{'case_complete.json'}
            for name,h in row['files'].items():assert sha(folder/name)==h,name
            t=load(folder/'transport.json')
            assert t['exit_code']==0 and t['error'] is None and not t['assignment_posts']
            assert t['day']==n+2 and len(t['new_actions'])==1
            assert t['restored']==[a['validated'] for a in original['actions'][:n]]
            expired=t['expired'];final=t['new_actions'][0]
            assert expired['validated']['ok'] and expired['validated']['agrees']
            assert expired['state']['day']==n and expired['state']['agents']==t['restored'][-1]['agents']
            assert expired['plan']==[[-original['case']['setup']['daySteps'][n]] for _ in expired['state']['agents']]
            assert final['wire_day']==n+1 and final['state']['agents']==expired['validated']['agents']
            assert final['validated']['ok'] and final['validated']['agrees'] and final['deadline_margin_ms']>0
            assert t['own']==[r['road_footprint'] for r in t['restored']]+[expired['validated']['road_footprint'],final['validated']['road_footprint']]
            posts=[r for r in t['requests'] if r['query']['method']=='POST']
            assert len(posts)==1 and posts[0]['query']['path'].endswith('/actions') and posts[0]['wire_day']==n+1
            raw=(folder/'replay.jsonl').read_bytes();assert raw.startswith(before)
            events=[json.loads(line) for line in raw[len(before):].splitlines()]
            kinds=[e['kind'] for e in events]
            assert kinds.count('decision')==2 and kinds.count('day_state')==2
            for kind in ('action_result','actions','actions_deadline_skip','actions_server_wait','protected_slack'):assert kinds.count(kind)==1
            assert not set(kinds).intersection(('resource_marginal','actions_fallback','actions_recovery_wait','virtual_parent_dropped'))
            for e in events:
                if e['kind']=='decision':
                    d=e['body']['decision'];validate_decision(d,n)
                    if d['dayNumber']==n+2:terminal_main.append(d['timing']['totalMs'])
                if e['kind']=='protected_slack':assert not any(v for k,v in e['body'].items() if k.endswith('Failure'))
                if e['kind']=='actions_deadline_skip':assert e['body']['day']==n+1
                if e['kind']=='action_result':assert e['body']['valid'] and e['body']['day']==n+2 and e['atUnixMs']<=final['state']['endsAt']
            assert (folder/'stderr').stat().st_size==0
            check=(folder/'replay-check.txt').read_text()
            assert f'summary days={n+2} reconciled_transitions={n+1}' in check
            assert f'resume accepted_days={n+2} last_wire_day={n+1}' in check
    assert report['rows']==rows and len(rows)==4
    for key in ('new_acks','expired_waits','restored_days','full_days','transitions'):
        assert sum(r[key] for r in rows)==report[key]==m['expected_'+key]
    assert (WORK/'runner364-v2.stderr').stat().st_size==0
    assert 'AssertionError' in (WORK/'runner364.stderr').read_text()
    return {'verified':True,'sessions':4,'new_acks':4,'expected_expired_waits':4,'restored_days':10,
        'full_replay_days':18,'transitions':14,'terminal_main_ms':terminal_main,
        'no_new_resource_entry':True,'no_first_session_rerun':True,'source_prefix_byte_identity':True,
        'complete_sha256':sha(WORK/'complete364.json'),'archive_sha256':sha(WORK.parent/'complete.tar.gz')}


if __name__=='__main__':print(json.dumps(verify(),indent=2))
