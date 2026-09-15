"""Complete-only analysis of four fixed369 sessions and immutable original pair."""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
COPY = ROOT/'artifacts/research/369/completed'
OLD = ROOT/'artifacts/research/366/completed-protected-causal/research/evidence/SCORE-CAUSAL-RESOURCE-QUALIFICATION-366-protected-causal-single'
EXEC_HASH = '3D07CEE221B01C6C198C73D704F523055F9B3EDA078F736AEF3EAD012E78F4B6'
ID = 'ATTR-ACTIVE-MAIN-REPEATABILITY-369'
SEED = 202609093860641
LABELS = ('parentA','candidateA','candidateB','parentB')


def sha(p):
    return hashlib.sha256(p.read_bytes()).hexdigest().upper()


def identity(x):
    return hashlib.sha256(json.dumps(x, sort_keys=True, separators=(',',':')).encode()).hexdigest().upper()


def load(p):
    return json.loads(p.read_text(encoding='utf-8'))


def semantic(state):
    return {k:v for k,v in state.items() if k != 'endsAt'}


def first_difference(left, right):
    assert len(left) == len(right)
    return next((i+1 for i,(a,b) in enumerate(zip(left,right)) if a != b), None)


def score_compare(a, b):
    assert len(a) == len(b) == 3 and all(type(v) is int for v in a+b)
    delta = [y-x for x,y in zip(a,b)]
    return {'A': a, 'B': b, 'delta': delta,
            'first_tier': next((i+1 for i,v in enumerate(delta) if v), None),
            'outcome': 'win' if b>a else 'loss' if b<a else 'tie'}


def session(p):
    events = [json.loads(line) for line in p.with_suffix('.replay.jsonl').read_text(encoding='utf-8').splitlines()]
    t = load(p.with_suffix('.transport.json'))
    cert = load(p.with_suffix('.certificate.json'))
    docs = [e['body'] for e in events if e['kind']=='decision']
    assert len(docs) == len(t['actions']) == 10 and not t['failure']
    assert cert['independently_checked']
    rows = []
    prev = [0,0,0]
    for i,(doc,action) in enumerate(zip(docs,t['actions'])):
        d = doc['decision']
        checkpoints = [e['body'] for e in events if e['kind']=='session_checkpoint' and e['body']['acceptedDay']==i+1]
        last = checkpoints[-1] if checkpoints else None
        score = action['validated']['score']
        rows.append({'day': i+1, 'score': score, 'increment': [x-y for x,y in zip(score,prev)],
            'main_state': semantic(doc['state']), 'main_ledger': doc['ledger'],
            'main_plan_hash': identity(d['candidate']['plan']),
            'submitted_plan_hash': identity(action['plan']), 'manifest_hash': identity(d['manifest']),
            'deadline': d['deadline'], 'timing': d['timing'], 'master': d['masterDiagnostics'],
            'audit': {k:v for k,v in d['audit'].items() if k!='candidates'},
            'pool_hashes': [identity(x['stableId']) for x in d['audit']['candidates']],
            'arrival_ms': action['arrival_ms'], 'deadline_margin_ms': action['deadline_margin_ms'],
            'checkpoint_records': len(checkpoints),
            'cached_contingencies_hash': identity(last['cachedContingencies']) if last else None,
            'proof_semantics_hash': identity([{k:v for k,v in x.items() if k not in ('combinationsVisited','branchesPruned')}
                                            for x in last['strongProofs']]) if last else None,
            'last_proofs': last['strongProofs'] if last else None})
        prev = score
    assert [sum(r['increment'][k] for r in rows) for k in range(3)] == t['score']
    return {'score': t['score'], 'roles': t['roles'], 'days': rows,
            'takeovers': [f['day'] for f in cert['frames'] if f['takeover']],
            'resource_frames': [{k:v for k,v in f.items() if k not in ('parentPlan','outputPlan')} for f in cert['frames']]}


def compare(a,b):
    da,db = a['days'], b['days']
    row = score_compare(a['score'], b['score'])
    row['roles_equal'] = a['roles'] == b['roles']
    fields = ('main_state','main_ledger','main_plan_hash','submitted_plan_hash','manifest_hash',
              'deadline','cached_contingencies_hash','proof_semantics_hash')
    row['first_difference'] = {k:first_difference([d[k] for d in da],[d[k] for d in db]) for k in fields}
    row['daily_delta'] = [[y-x for x,y in zip(xd['increment'],yd['increment'])] for xd,yd in zip(da,db)]
    assert [sum(d[k] for d in row['daily_delta']) for k in range(3)] == row['delta']
    return row


def main():
    assert sha(COPY/'execution369.json') == EXEC_HASH
    complete = load(COPY/'data/run_complete.json')
    assert complete['execution_sha256'] == EXEC_HASH
    assert (complete['results'],complete['actions'],complete['transitions']) == (4,40,36)
    assert sha(COPY/'summary369.json') == complete['summary_sha256']
    assert len(complete['files']) == 28
    for name,h in complete['files'].items():
        p = Path(name)
        assert not p.is_absolute() and '..' not in p.parts
        assert sha(COPY/'data'/p) == h, name
    expected_old = load(OLD/'run_complete.json')['files']
    for side in ('parent','candidate'):
        for suffix in ('.replay.jsonl','.transport.json','.certificate.json'):
            p = OLD/'A'/side/(str(SEED)+suffix)
            assert sha(p) == expected_old[p.relative_to(OLD).as_posix()]
    runs = {l: session(COPY/'data'/l[-1]/l[:-1]/str(SEED)) for l in LABELS}
    runs.update({s+'Original':session(OLD/'A'/s/str(SEED)) for s in ('parent','candidate')})
    planned = {'AB-A':('parentA','candidateA'), 'AB-B':('parentB','candidateB'),
               'AA-parent':('parentA','parentB'), 'AA-candidate':('candidateA','candidateB')}
    comparisons = {k:compare(runs[a],runs[b]) for k,(a,b) in planned.items()}
    historical = {l:compare(runs[l[:-1]+'Original'],runs[l]) for l in LABELS}
    result = {'experiment': ID, 'execution_sha256': EXEC_HASH, 'script_sha256': sha(Path(__file__)),
              'complete_sha256': sha(COPY/'data/run_complete.json'), 'verified_files': 28,
              'runs': runs, 'planned_comparisons': comparisons, 'same_label_vs_original': historical,
              'promotion_authority': False, 'null_treatment_effect_proven': False,
              'historical_OS_cause_proven': False}
    out = ROOT/'research/evidence'/(ID+'-explanation.json')
    if out.exists():
        assert load(out) == result
    else:
        with out.open('x',encoding='utf-8',newline='\n') as stream:
            json.dump(result,stream,indent=2,sort_keys=True)
            stream.write('\n')
    print('scores',json.dumps({k:v['score'] for k,v in runs.items()}))
    print('comparisons',json.dumps(comparisons))
    print('same_label_vs_original',json.dumps(historical))
    print('explanation_sha256',sha(out))


if __name__ == '__main__':
    main()
