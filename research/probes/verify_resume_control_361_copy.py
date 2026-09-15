"""Independent read-only integrity/lifecycle audit of the downloaded 361 evidence."""
import hashlib
import json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
WORK=ROOT/'artifacts/research/361/completed'
def sha(p):return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def load(p):return json.loads(p.read_text())

def verify():
    assert sha(WORK.parent/'complete.tar.gz')=='C8AD766D85DCE3E182E0CD8D901DE0006FFE6104D1555C70BD9C96F725BB0CCE'
    assert sha(WORK/'complete361.json')=='6A9478EA267C5220A69376DC4F584AB40915E67F4ABD385F48744772398A3184'
    summary=load(WORK/'complete361.json')
    assert summary['stage_sha256']==sha(WORK/'stage361.json')
    for name,h in load(WORK/'stage361.json')['hashes'].items():assert sha(WORK/name)==h,name
    original=ROOT/'artifacts/research/360/completed/contracts/mixed/candidate/3419001.replay.jsonl'
    prefix=[]
    for line in original.read_bytes().splitlines(keepends=True):
        e=json.loads(line)
        if e['kind']=='day_state' and e['body']['day']==2:break
        prefix.append(line)
    prefix=b''.join(prefix)
    assert len(summary['rows'])==4 and len(list((WORK/'sessions').glob('*/case_complete.json')))==4
    for row in summary['rows']:
        p=WORK/'sessions'/(row['link']+'-'+row['control'])
        assert row==load(p/'case_complete.json')
        for name,h in row['files'].items():assert sha(p/name)==h,name
        t=load(p/'transport.json')
        assert not t['error'] and t['exit_code']==0 and t['day']==4
        assert not t['assignment_posts'] and len(t['new_actions'])==1
        assert t['new_actions'][0]['wire_day']==3 and len(t['restored'])==2
        assert len(t['own'])==4 and t['expired']['validated']['ok'] and t['expired']['validated']['agrees']
        assert t['prefix_bytes']==len(prefix) and t['prefix_sha256']==hashlib.sha256(prefix).hexdigest().upper()
        assert (p/'replay.jsonl').read_bytes().startswith(prefix)
        new=(p/'replay.jsonl').read_bytes()[len(prefix):]
        e=[json.loads(line) for line in new.splitlines()]
        assert [x['kind'] for x in e].count('actions_deadline_skip')==1
        assert [x['kind'] for x in e].count('actions_server_wait')==1
        assert [x['kind'] for x in e].count('action_result')==1
        assert not any(x['kind']=='resource_marginal' for x in e)
        assert not (p/'control.txt').exists() and (p/'stderr').stat().st_size==0
        posts=[x['query'] for x in t['requests'] if x['query']['method']=='POST']
        assert len(posts)==1 and posts[0]['path'].endswith('/actions')
        check=(p/'replay-check.txt').read_text()
        assert 'summary days=4 reconciled_transitions=3' in check
        assert 'resume accepted_days=4 last_wire_day=3' in check
    return dict(verified=True,sessions=4,new_acks=4,expired_waits=4,duplicate_prefix_posts=0,
                resumed_treatment_reads=0,full_replay_days=16,full_replay_transitions=12)

if __name__=='__main__':print(json.dumps(verify(),indent=2))
