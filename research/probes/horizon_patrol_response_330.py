"""Frozen one-coordinate whole-horizon capability; no production mutations."""
import argparse
import json
from collections import Counter
from pathlib import Path
from http_prefix_option_loss_321 import ROOT, load, digest, require, write_new, close_bridge
from pool_boundary_value_327 import verify_local
from run_http_baseline_314 import Bridge
from day2_pool_value_323 import comparison
ID='ATTR-W1-HORIZON-PATROL-BEST-RESPONSE-330'
M=ROOT/f'research/holdouts/{ID}.json'
S=ROOT/f'research/evidence/{ID}.summary.json'
D=ROOT/f'research/evidence/{ID}'

def freeze():
    previous=ROOT/'research/holdouts/ATTR-W1-CERTIFIED-SUFFIX-REUSE-329.json'
    old=load(previous);verify_local(old)
    cases=[]
    for c in old['cases']:
        for v in c['candidates']:
            node=v['reference']['nodes'][1]
            cases.append({'id':str(c['seed'])+'-'+v['label'],'seed':c['seed'],'label':v['label'],
                'family':c['family'],'players':c['players'],'audit':v['audit'],'baseline':v['reference']['score'],
                'exact_value':v['exact_value'],'selected_exact_value':c['selected_exact_value'],
                'fixed_states':[d['agents'] for d in v['reference']['days'][1:]],
                'request':{'setup':c['setup'],'state':node['state'],'ledger':node['ledger'],'plans':v['plans'][1:]}})
    require(len(cases)==27,'eligible candidate coverage')
    paths=set(old['local_hashes'])|{str(previous.relative_to(ROOT)).replace('\\','/'),
        'research/probes/horizon_patrol_response_330.cpp','research/probes/horizon_patrol_response_330.py',
        'research/probes/test_horizon_patrol_response_330.py','artifacts/research/330/probe.exe'}
    write_new(M,{'experiment':ID,'cases':cases,'local_hashes':{p:digest(ROOT/p) for p in sorted(paths)},
        'probe':'artifacts/research/330/probe.exe','bridge':'artifacts/research/314/bridge.exe',
        'expected_responses':81,'gate':'All27 original eligible W1 controls81 independent single-Patrol complete best responses. Strict gain on2 better-action roots2families permits separate fresh bounded SCORE proposal only.',
        'production_change':False,'holdout_authority':False})
    print(json.dumps({'manifest_sha256':digest(M),'probe_sha256':digest(ROOT/'artifacts/research/330/probe.exe'),'requests':27,'responses':81}))

def validate(out,c,bridge):
    require(out.get('ok') and out.get('complete') and out['baseline']==c['baseline'] and
            [r['agent'] for r in out['responses']]==[0,1,2],'whole original best-response coverage')
    dual=0
    for r in out['responses']:
        require(r['complete'] and [d['day'] for d in r['days']]==[2,3,4] and
                tuple(c['baseline'])<=tuple(r['score'])<=tuple(c['exact_value']),'value/control/ceiling')
        q=c['request'];state=q['state'];ledger=q['ledger'];agent=r['agent']
        for i,d in enumerate(r['days']):
            require(all(d['plan'][a]==q['plans'][i][a] for a in range(3) if a!=agent),'other plan changed')
            checked=bridge.request({'op':'step','setup':q['setup'],'state':state,'ledger':ledger,'plan':d['plan']})
            require(checked.get('ok') and checked.get('agrees') and all(checked[k]==d[k] for k in ('score','agents','ledger')),'dual witness identity')
            require(all(checked['agents'][a]==c['fixed_states'][i][a] for a in range(3) if a!=agent),'other state changed')
            state={**state,'day':state['day']+1,'agents':checked['agents']};ledger=checked['ledger'];dual+=1
        require(r['score']==r['days'][-1]['score'],'final score')
    return dual

def run():
    m=load(M);verify_local(m);D.mkdir(exist_ok=False)
    probe=Bridge(ROOT/m['probe']);validator=Bridge(ROOT/m['bridge']);results=[];dual=0
    try:
        for c in m['cases']:
            out=probe.request(c['request']);dual+=validate(out,c,validator)
            row={'id':c['id'],'request':c['request'],'output':out,'manifest_sha256':digest(M)}
            write_new(D/(c['id']+'.result.json'),row)
            best=max((r['score'] for r in out['responses']),key=tuple)
            results.append({'id':c['id'],'seed':c['seed'],'family':c['family'],'players':c['players'],
                'audit':c['audit'],'control':c['baseline'],'best':best,'exact_value':c['exact_value'],
                'best_vs_control':comparison(best,c['baseline']),
                'action_vs_selected_exact':comparison(c['exact_value'],c['selected_exact_value']),
                'responses':[{**r,'versus_control':comparison(r['score'],c['baseline'])} for r in out['responses']]})
    finally:close_bridge(probe);close_bridge(validator)
    require(dual==243 and len(results)==27,'complete measured coverage');verify_local(m)
    marker={'experiment':ID,'manifest_sha256':digest(M),'requests':27,'responses':81,
        'result_hashes':{p.name:digest(p) for p in sorted(D.glob('*.result.json'))}}
    write_new(D/'run_complete.json',marker)
    qualified=[r for r in results if r['best_vs_control']['result']=='win' and r['action_vs_selected_exact']['result']=='win']
    report={'experiment':ID,'manifest_sha256':digest(M),'run_complete_sha256':digest(D/'run_complete.json'),
        'results':results,'result_hashes':marker['result_hashes'],'complete':True,'dual_days':dual,
        'zero_invalid_or_reconstruction_failure':True,'qualified_roots':sorted({r['seed'] for r in qualified}),
        'design_qualified':len({r['seed'] for r in qualified})>=2 and len({r['family'] for r in qualified})>=2,
        'candidate_wtl':dict(Counter(r['best_vs_control']['result'] for r in results)),
        'response_wtl':dict(Counter(p['versus_control']['result'] for r in results for p in r['responses'])),
        'strata':{f:{str(k):dict(Counter(r['best_vs_control']['result'] for r in results if r[f]==k))
                   for k in sorted({r[f] for r in results})} for f in ('family','players')},
        'production_change':False,'holdout_authority':False,'score_promotion_authority':False}
    write_new(S,report);print(json.dumps({k:v for k,v in report.items() if k not in ('results','result_hashes')}))
    print('summary_sha256='+digest(S))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','run'));a=p.parse_args()
    freeze() if a.mode=='freeze' else run()
