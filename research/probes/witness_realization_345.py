"""Exact read-only realization attribution on all complete344 development matches."""
import argparse
from collections import Counter
import copy
import json
from pathlib import Path
import complete_pool_score_344 as source
from ack_suffix_consumption_333 import score, plan_id, cache_has, classify, gate, check_step

ROOT=source.ROOT
ID='ATTR-COMPLETE-POOL-WITNESS-REALIZATION-345'
M=ROOT/f'research/holdouts/{ID}.json'
S=ROOT/f'research/evidence/{ID}.summary.json'
R=ROOT/f'research/evidence/{source.ID}-development.summary.json'
D=ROOT/f'research/evidence/{source.ID}-development'
load,verify,require,digest,write_new=source.load,source.verify,source.require,source.digest,source.write_new


def complete344():
    m=load(source.M);verify(m)
    require(digest(R)=='8FDF5694178AC857186FB01E570A02461A675FB618A55D01ED69EECDBCDBAEE3','344 report drift')
    r=load(R)
    require(r['complete'] and r['results']==96 and not r['gate']['passed'],'344 completion/verdict')
    require(digest(D/'run_complete.json')==r['completion_sha256'],'344 completion drift')
    cases=load(ROOT/m['splits']['development']['path'])['cases']
    source.audit(cases,D,'development')
    return m,cases,r


def freeze():
    m,cases,r=complete344()
    paths={source.M,R,D/'run_complete.json',Path(__file__),ROOT/f'research/probes/test_witness_realization_345.py',
        ROOT/f'research/evidence/{ID}-preregistration.md',ROOT/'research/probes/ack_suffix_consumption_333.py',
        ROOT/'research/evidence/ATTR-ACK-W1-SUFFIX-CONSUMPTION-333-closure.md'}
    for c in cases:
        paths.add(D/f'{source.seed(c)}.fixture_complete.json')
        for label in source.LABELS:
            p=source.location(c,label,D)
            paths.update(p.with_suffix(s) for s in source.SUFFIXES)
    hashes={**m['hashes'],**{str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in sorted(paths)}}
    write_new(M,{'experiment':ID,'parent':'c76a8ea','cases':cases,'bridge':m['bridge_binary'],
        'hashes':hashes,'matches':96,'boundaries':288,'dual_days':864,
        'authority':'Complete consumed DEVELOPMENT only. No solver/oracle or promotion.',
        'gate':'Eligible retained same-state witness value loss on2distinctroots2families permits separate consistency proposal.'})
    print(json.dumps({'manifest_sha256':digest(M),'dependencies':len(hashes)}))


def analyze():
    m=load(M);verify(m);_,cases,r=complete344()
    pairs={(x['seed'],x['repeat']):x for x in r['rows']}
    rows=[];matches=[];dual_days=0;bridge=source.Bridge(ROOT/m['bridge'])
    try:
        for c in cases:
            seed=source.seed(c)
            for label,(repeat,side) in source.LABELS.items():
                p=source.location(c,label,D)
                events=[json.loads(line) for line in p.with_suffix('.replay.jsonl').read_text().splitlines()]
                indexed=[(i,e['body']) for i,e in enumerate(events) if e['kind']=='decision']
                t=load(p.with_suffix('.transport.json'));result=load(p.with_suffix('.result.json'))
                require(len(indexed)==4,'four decision coverage')
                match={'seed':seed,'family':c['spec']['family'],'repeat':repeat,'side':side,
                    'paired_result':pairs[seed,repeat]['comparison'],'actual_final_score':result['http_score'],'days':[]}
                for di,(ei,body) in enumerate(indexed):
                    d=body['decision']
                    match['days'].append({'day':di+1,'current_score':score(d['candidate']['scoreAfterToday']),
                        'certificate':score(d['profile']['certifiedLowerBound']),
                        'valid_upper':score(d['profile']['validUpperBound']),
                        'complete_pool_pricing':d['audit'].get('completePoolPricing'),
                        'selected_id':d['candidate']['stableId']})
                    if di==3:continue
                    ni,nb=indexed[di+1];nd=nb['decision']
                    require(body['state']['day']==di+1 and nb['state']['day']==di+2,'day identity')
                    require(not body['state']['traffics'] and not body['state']['others'],'roadless scope')
                    require(d['candidate']['plan']==t['actions'][di]['plan'],'actual submitted plan')
                    require(len(d['profile']['outcomes'])==1,'deterministic witness count')
                    w=d['profile']['outcomes'][0]
                    require(w['certified'] and not w['lowerBoundOnly'] and len(w['futurePlans'])==3-di,'complete W1')
                    current=check_step(bridge,c['setup'],body['state'],body['ledger'],d['candidate']['plan']);dual_days+=1
                    require(current['agents']==nb['state']['agents'] and current['ledger']==nb['ledger'],'ACK state/ledger')
                    checkpoints=[e['body'] for e in events[ei+1:ni] if e['kind']=='session_checkpoint' and e['body']['acceptedDay']==di+1]
                    require(checkpoints,'checkpoint coverage')
                    state,ledger=copy.deepcopy(nb['state']),copy.deepcopy(nb['ledger']);days=[]
                    for plan in w['futurePlans']:
                        step=check_step(bridge,c['setup'],state,ledger,plan);dual_days+=1
                        days.append({'day':state['day'],'plan':plan,**step})
                        state={**state,'day':state['day']+1,'agents':step['agents']};ledger=step['ledger']
                    require(days[-1]['score']==score(w['witnessScore'])==score(w['score']),'suffix certificate score')
                    wid=plan_id(w['futurePlans'][0]);audit=[a for a in nd['audit']['candidates'] if a['stableId']==wid]
                    require(len(audit)<=1,'duplicate candidate')
                    row={**c['spec'],'repeat':repeat,'side':side,'after_day':di+1,
                        'paired_result':pairs[seed,repeat]['comparison'],'replay_sha256':result['replay_sha256'],
                        'witness_score':days[-1]['score'],'witness_first_day_score':days[0]['score'],
                        'next_certificate':score(nd['profile']['certifiedLowerBound']),
                        'next_current_score':score(nd['candidate']['scoreAfterToday']),
                        'actual_final_score':result['http_score'],
                        'selected_pricing_improvements':d['profile'].get('horizonPricing',{}).get('improvements',0),
                        'cache_at_ack':cache_has(checkpoints[0],w),'cache_before_next':cache_has(checkpoints[-1],w),
                        'checkpoint_count':len(checkpoints),'cache_repair':nd['cacheRepair'],
                        'plan_matches':wid==nd['candidate']['stableId'],'audit_record':audit[0] if audit else None,
                        'current_floor_eligible':tuple(days[0]['score'])>=tuple(score(nd['candidate']['scoreAfterToday'])),
                        'validated_suffix':days}
                    row['classification']=classify(row)
                    row['realized_below_witness']=tuple(row['actual_final_score'])<tuple(row['witness_score'])
                    rows.append(row)
                matches.append(match)
    finally:
        bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    require(len(matches)==96 and len(rows)==288 and dual_days==864,'exact coverage')
    verify(m)
    report={'experiment':ID,'complete':True,'boundaries':len(rows),'dual_days':dual_days,'gate_passed':gate(rows),
        'classifications':dict(Counter(x['classification'] for x in rows)),
        'realized_below_witness':sum(x['realized_below_witness'] for x in rows),
        'missing_ack_cache':sum(not x['cache_at_ack'] for x in rows),
        'missing_next_cache':sum(not x['cache_before_next'] for x in rows),
        'strata':{k:{str(v):dict(Counter(x['classification'] for x in rows if x[k]==v))
            for v in sorted({x[k] for x in rows})} for k in ('repeat','side','family','players','after_day','paired_result')},
        'zero_state_ledger_validation_failure':True,'rows':rows,'matches':matches,'manifest_sha256':digest(M),
        'source_summary_sha256':digest(R),'authority':'Read-only consumed DEVELOPMENT exact-plan attribution, no promotion.'}
    write_new(S,report)
    print(json.dumps({k:v for k,v in report.items() if k not in ('rows','matches','strata')}))
    print('summary_sha256='+digest(S))


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','analyze'));a=p.parse_args()
    freeze() if a.mode=='freeze' else analyze()
