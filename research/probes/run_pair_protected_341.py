"""Frozen whole-matrix HTTP screen; no partial score analysis or auto-promotion."""
import argparse
from collections import Counter
from pathlib import Path
from protected_http_transport_341 import ROOT,Bridge,load,digest,write_new,require,run_case,validate_side as basic_validate,operational,traffic_for
from run_pair_score_341 import verify,memory_available,decisions,measured_day,FIELDS
from summarize_http_baseline_314 import compare

ID='SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341'
M=ROOT/f'research/holdouts/{ID}-protected-execution.json'
D=ROOT/f'research/evidence/{ID}-protected'
S=ROOT/f'research/evidence/{ID}-protected.summary.json'
SIDES=('parent','candidate')
STRATA=('family','fuel','side','role','days','players','window_ms','roadless','agent_count','brand_count','stock_vector','step_vector')


def coverage(cases):
    require(len(cases)==108 and len({c['seed'] for c in cases})==108,'protected identity/coverage')
    wanted={'side':{8:54,32:54},'role':{'fixed-all-Patrol':54,'native':54},
            'days':{4:36,5:36,10:36},'players':{8:36,9:36,10:36},
            'fuel':{'low':36,'default':36,'high':36},'window_ms':{5000:36,10000:36,15000:36}}
    for k,v in wanted.items():require(dict(Counter(c[k] for c in cases))==v,'coverage '+k)
    require(len(Counter(c['family'] for c in cases))==6 and
            set(Counter(c['family'] for c in cases).values())=={18},'six traffic families')
    for c in cases:
        require(c['days']==len(c['setup']['daySteps'])==len(c['setup']['daySeconds']),'day count')
        require(sorted(c['order'])==sorted(SIDES),'paired order')
    require(sum(c['days'] for c in cases)==684,'total days')
    return {'pairs':108,'results':216,'actions':1368,'transitions':1152}


def validate_side(c,side,directory):
    row=basic_validate(c,side,directory)
    prefix=directory/side/str(c['seed']);t=load(prefix.with_suffix('.transport.json'))
    require(t['case']==c and t['synthetic'] is True and t['failure'] is None,'transport provenance/failure')
    require(len(t['actions'])==len(t['own_road_footprints'])==c['days'],'committed day count')
    require(t['score']==row['http_score']==t['actions'][-1]['validated']['score'],'final ledger score identity')
    require((len(t['assignment_posts'])==1)==(c['role']=='native'),'assignment contract')
    if c['role']=='fixed-all-Patrol':require(t['roles']==[0]*len(c['setup']['agents']),'fixed roles drift')
    for i,a in enumerate(t['actions']):
        require(a['wire_day']==a['state']['day']==i and a['deadline_margin_ms']>=0,'action day/deadline')
        require(a['validated']['ok'] and a['validated']['agrees'],'dual validator failure')
        require(a['state']['traffics']==traffic_for(c['setup'],i,t['own_road_footprints'][:i],c['external_road_footprints']),'own/external traffic drift')
        require(a['validated']['road_footprint']==t['own_road_footprints'][i],'own footprint identity')
        if i:require(a['state']['agents']==t['actions'][i-1]['validated']['agents'],'state transition identity')
    text=prefix.with_suffix('.replay-check.txt').read_text()
    require(f"summary days={c['days']} reconciled_transitions={c['days']-1}" in text,'replay-check closure missing')
    return row


def pair_row(c,directory):
    return {'seed':c['seed'],'order':c['order'],'results':{
        s:digest((directory/s/str(c['seed'])).with_suffix('.result.json')) for s in SIDES}}


def audit_resume(cases,directory):
    """Never turn a prefix containing accepted days into a fresh side."""
    expected={str(c['seed']) for c in cases}
    allowed={str(c['seed'])+'.pair_complete.json' for c in cases}
    require(all(p.name in allowed|set(SIDES)|{'run_complete.json'} for p in directory.iterdir()),'foreign run evidence')
    for s in SIDES:
        require((directory/s).is_dir(),'missing side directory')
        require(all(p.name.split('.')[0] in expected for p in (directory/s).iterdir()),'foreign side evidence')
        for c in cases:
            p=directory/s/str(c['seed'])
            if p.with_suffix('.result.json').exists():validate_side(c,s,directory)
            else:require(not list((directory/s).glob(p.name+'.*')),'ambiguous partial side; no blind resume')
    for c in cases:
        marker=directory/(str(c['seed'])+'.pair_complete.json')
        if marker.exists():require(load(marker)==pair_row(c,directory),'pair marker mismatch')


def complete_audit(m,cases,directory=D):
    counts=coverage(cases);marker=load(directory/'run_complete.json')
    require(all(marker[k]==v for k,v in counts.items()),'completion counts')
    require(marker['execution_sha256']==digest(M),'completion execution identity')
    require(len(list(directory.glob('*.pair_complete.json')))==108 and
            all(len(list((directory/s).glob('*.result.json')))==108 for s in SIDES),'exact completion file counts')
    audit_resume(cases,directory)
    require(marker['pair_hashes']=={p.name:digest(p) for p in sorted(directory.glob('*.pair_complete.json'))},'completion pair hashes')
    return marker


def run(resume=False):
    m=load(M);verify(m);cases=load(ROOT/m['protected'])['cases'];counts=coverage(cases)
    if D.exists():require(resume and not (D/'run_complete.json').exists(),'existing run; no duplication')
    else:
        D.mkdir()
        for s in SIDES:(D/s).mkdir()
    audit_resume(cases,D)
    bridge=Bridge(ROOT/m['bridge_binary'])
    try:
        for i,c in enumerate(cases):
            for side in c['order']:
                p=(D/side/str(c['seed'])).with_suffix('.result.json')
                if p.exists():continue
                require(memory_available()>=m['resource_floor_bytes'],'available RAM below frozen 1024MiB safety floor')
                run_case(c,m[side+'_binary'],D/side,bridge,ID,side)
                validate_side(c,side,D)
                print(f'side_complete pair={i+1} side={side}',flush=True)
            marker=D/(str(c['seed'])+'.pair_complete.json');row=pair_row(c,D)
            if marker.exists():require(load(marker)==row,'existing pair marker drift')
            else:write_new(marker,row)
            print(f'pair_complete count={i+1}',flush=True)
    finally:bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    verify(m)
    write_new(D/'run_complete.json',{**counts,'execution_sha256':digest(M),
        'pair_hashes':{p.name:digest(p) for p in sorted(D.glob('*.pair_complete.json'))}})
    print('run_complete pairs=108 results=216',flush=True)
    summarize()


def aggregate(rows):
    return {'pairs':len(rows),'wtl':{k:sum(r['comparison']==k for r in rows) for k in ('win','tie','loss')},
        'delta':[sum(r['delta'][i] for r in rows) for i in range(3)],
        'first_tiers':dict(Counter(str(r['first_tier']) for r in rows)),
        'gain_tail':[{'seed':r['seed'],'delta':r['delta']} for r in rows if r['comparison']=='win'],
        'loss_tail':[{'seed':r['seed'],'delta':r['delta']} for r in rows if r['comparison']=='loss']}


def protected_checks(rows):
    """Conservative predeclared screen; benefit is independently held-out already.

    These checks never waive real BTC, integration or lane-champion review. No
    fresh win quota is imposed on unaffected lanes, and a small -1 serving loss
    does not automatically reject an otherwise globally beneficial candidate.
    """
    nets={key:{str(v):sum(r['delta'][2] for r in rows if r[key]==v)
               for v in {r[key] for r in rows}} for key in ('family','fuel')}
    checks={'no_lifetime_or_daily_loss':all(r['delta'][0]>=0 and r['delta'][1]>=0 for r in rows),
        'bounded_serving_loss':all(r['delta'][2]>=-1 for r in rows),
        'no_negative_family_or_fuel_net':all(v>=0 for d in nets.values() for v in d.values()),
        'no_inactive_score_difference':all(r['comparison']=='tie' or r['activated_before_divergence'] for r in rows),
        'inactive_exact_plan_state_ledger_roles':all(r['trajectory_equal'] for r in rows if r['first_certificate_gain_day'] is None),
        'zero_operational_failure':all(all(r['safety'][s]['safety_pass'] for s in SIDES) for r in rows)}
    return {'checks':checks,'passed':all(checks.values()),'family_fuel_net':nets,
            'promotion_authorized':False,'remaining':'Integration equivalence, relevant lane champions and genuine BTC target-host gates.'}


def summarize():
    m=load(M);verify(m);cases=load(ROOT/m['protected'])['cases'];complete_audit(m,cases)
    rows=[]
    for c in cases:
        prefix={s:D/s/str(c['seed']) for s in SIDES}
        result={s:validate_side(c,s,D) for s in SIDES}
        transport={s:load(prefix[s].with_suffix('.transport.json')) for s in SIDES}
        plans={s:[a['plan'] for a in transport[s]['actions']] for s in SIDES}
        replay={s:prefix[s].with_suffix('.replay.jsonl') for s in SIDES}
        dd=decisions(replay['candidate'])
        telem=[measured_day(d,plans['candidate'][i+1] if i+1<c['days'] else None) for i,d in enumerate(dd)]
        roles_equal=transport['parent']['roles']==transport['candidate']['roles']
        divergence=0 if not roles_equal else next((i+1 for i in range(c['days']) if plans['parent'][i]!=plans['candidate'][i]),None)
        activation=next((d['day'] for d in telem if d['all_candidates']['improvements']),None)
        a,b=result['candidate']['http_score'],result['parent']['http_score'];outcome,tier,diff=compare(a,b)
        equal={k:all(transport['parent']['actions'][i]['validated'][k]==transport['candidate']['actions'][i]['validated'][k]
                        for i in range(c['days'])) for k in ('agents','ledger','road_footprint')}
        row={k:c[k] for k in ('seed','family','fuel','side','role','days','players','window_ms','roadless','order')}
        row.update({'agent_count':len(c['setup']['agents']),'brand_count':len({s['brand'] for s in c['setup']['spots']}),
            'stock_vector':str([s['stocks'] for s in c['setup']['spots']]),'step_vector':str(c['setup']['daySteps']),
            'parent':b,'candidate':a,'comparison':outcome,'first_tier':tier,'first_tier_difference':diff,
            'delta':[x-y for x,y in zip(a,b,strict=True)],'first_plan_divergence_day':divergence,
            'first_certificate_gain_day':activation,'activated_before_divergence':activation is not None and divergence is not None and activation<=divergence,
            'roles_equal':roles_equal,'plans_equal':plans['parent']==plans['candidate'],'transition_equality':equal,
            'trajectory_equal':roles_equal and plans['parent']==plans['candidate'] and all(equal.values()),
            'plan_sha256':{s:__import__('hashlib').sha256(__import__('json').dumps(plans[s],separators=(',',':')).encode()).hexdigest().upper() for s in SIDES},
            'roles':{s:transport[s]['roles'] for s in SIDES},'pricing_days':telem,
            'work':{k:sum(d['all_candidates'][k] for d in telem) for k in FIELDS},
            'safety':{s:operational(replay[s],c,s) for s in SIDES},
            'result_hashes':{s:digest(prefix[s].with_suffix('.result.json')) for s in SIDES}})
        rows.append(row)
    report={'experiment':ID,'phase':'protected','complete':True,**coverage(cases),**aggregate(rows),
        'rows':rows,'strata':{k:{str(v):aggregate([r for r in rows if r[k]==v]) for v in sorted({r[k] for r in rows})} for k in STRATA},
        'work':{k:sum(r['work'][k] for r in rows) for k in FIELDS},'protected_gate':protected_checks(rows),
        'execution_sha256':digest(M),'run_complete_sha256':digest(D/'run_complete.json'),
        'authority':'Synthetic Windows HTTP parent-paired protected screen; frozen aggregate external traffic, not opponent policies. No BTC latency, competition readiness or automatic promotion authority.'}
    write_new(S,report)
    print({k:report[k] for k in ('pairs','wtl','delta','protected_gate')},flush=True)
    print('summary_sha256='+digest(S),flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('run','summarize'));p.add_argument('--resume',action='store_true')
    args=p.parse_args();run(args.resume) if args.mode=='run' else summarize()
