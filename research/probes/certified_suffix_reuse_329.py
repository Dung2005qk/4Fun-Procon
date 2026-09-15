"""One-hop reuse of existing W1 witnesses; no optimizer or product call."""
import argparse
import copy
import itertools
import json
from collections import Counter
from pathlib import Path
from http_prefix_option_loss_321 import ROOT, load, digest, require, write_new, state, close_bridge
from pool_boundary_value_327 import verify_local
from day2_pool_value_323 import comparison, scores
from run_http_baseline_314 import Bridge

ID = 'ATTR-W1-CERTIFIED-SUFFIX-REUSE-329'
M = ROOT / f'research/holdouts/{ID}.json'
S = ROOT / f'research/evidence/{ID}.summary.json'

def mappings(recipient, donor):
    require(len(recipient) == len(donor) == 3 and all(a['kind'] == 0 for a in recipient+donor), 'Patrol domain')
    return [list(p) for p in itertools.permutations(range(3))
            if all(recipient[i]['pos'] == donor[p[i]]['pos'] and
                   recipient[i]['fuel'] >= donor[p[i]]['fuel'] for i in range(3))]

def splice(recipient_plans, donor_plans, boundary, mapping):
    require(boundary in (1, 2, 3) and sorted(mapping) == [0,1,2] and
            len(recipient_plans) == len(donor_plans) == 4, 'splice identity')
    return copy.deepcopy(recipient_plans[:boundary]) + [
        [copy.deepcopy(day[mapping[i]]) for i in range(3)] for day in donor_plans[boundary:]]

def replay(bridge, setup, root_state, root_ledger, plans):
    agents = root_state['agents']; ledger = root_ledger; nodes = []; days = []
    require(root_state['day'] == 1 and len(plans) == 4, 'full four days')
    for index, plan in enumerate(plans):
        q = state(agents, index+1)
        nodes.append({'state':q,'ledger':copy.deepcopy(ledger)})
        r = bridge.request({'op':'step','setup':setup,'state':q,'ledger':ledger,'plan':plan})
        require(r.get('ok') and r.get('agrees'), 'eligible suffix invalid or simulator disagreement')
        days.append({'day':index+1,'plan':plan,**r})
        agents, ledger = r['agents'], r['ledger']
    return {'nodes':nodes,'days':days,'score':days[-1]['score']}

def freeze():
    m328 = ROOT/'research/holdouts/ATTR-DAY1-AUDIT-EXACT-OPTION-328.json'
    s328 = ROOT/'research/evidence/ATTR-DAY1-AUDIT-EXACT-OPTION-328.summary.json'
    require(digest(s328) == '78A7337894BA8DDBA038A6191117872280E6F6D0C0A13BBDD34A750A26C12343', '328 summary')
    old = load(m328); verify_local(old)
    values = {r['seed']:r for r in load(s328)['matches']}
    m316 = ROOT/'research/holdouts/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316.json'
    s316 = ROOT/'research/evidence/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316.summary.json'
    old316 = load(m316)
    for p,h in old316['hashes'].items(): require(digest(ROOT/p) == h,'316 dependency drift:'+p)
    require(digest(s316) == '318184A0BA2B676EDF4D9CC55B758E46A1602991D48FF40832C7D0F13AAF4F61','316 summary')
    references = {r['seed']:r for r in load(s316)['results']}
    paths = set(old['local_hashes']) | set(old316['hashes'])
    cases = []; bridge = Bridge(ROOT/'artifacts/research/314/bridge.exe')
    try:
        for c in old['cases']:
            seed = c['seed']; q = c['request']
            require(all(v != 1 for row in q['setup']['map']['cells'] for v in row) and
                    q['state']['others'] == [] and len(q['setup']['daySteps']) == 4,'roadless scope')
            path = ROOT/f'research/evidence/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316/{seed}.result.jsonl'
            require(digest(path) == references[seed]['result_sha256'],'316 immutable output')
            paths.add(str(path.relative_to(ROOT)).replace('\\','/'))
            rows = [json.loads(line) for line in path.read_text().splitlines()]
            require([r['index'] for r in rows] == list(range(16)),'316 coverage')
            candidates = []
            for i,row in enumerate(rows):
                label = f'p{i:02d}'; a = c['plans'][label]['audit']; o = row['output']
                if not a['certified']: continue
                require(a['w1Role'] and a['disposition'] in ('selected','certified-not-selected','certified-dominated'), 'actual eligible W1')
                require(o['ok'] and o['certified'] and not o['lower_bound_only'] and
                        len(o['future_plans']) == 3 and o['w1_score'] == scores(a['finalCertifiedLowerBound']), 'recorded certificate identity')
                plans = [json.loads(a['stableId'])] + o['future_plans']
                ref = replay(bridge,q['setup'],q['state'],q['ledger'],plans)
                require(ref['score'] == o['w1_score'] and all(ref['days'][0][k] == o['current_'+k]
                        for k in ('agents','ledger','score')),'316 root/witness identity')
                value = next(r['value'] for r in values[seed]['plans'] if r['label'] == label)
                candidates.append({'label':label,'audit':a,'plans':plans,'reference':ref,'exact_value':value})
            require(sum(v['audit']['selected'] for v in candidates) == 1,'selected included')
            cases.append({'seed':seed,'family':c['family'],'players':c['players'],
                'setup':q['setup'],'state':q['state'],'ledger':q['ledger'],
                'selected_exact_value':c['selected_value'],'candidates':candidates})
    finally: close_bridge(bridge)
    paths.update(str(p.relative_to(ROOT)).replace('\\','/') for p in
                 (m328,s328,m316,s316,Path(__file__),Path(__file__).with_name('test_certified_suffix_reuse_329.py')))
    require(len(cases) == 12,'all12')
    write_new(M,{'experiment':ID,'cases':cases,'local_hashes':{p:digest(ROOT/p) for p in sorted(paths)},
        'bridge':'artifacts/research/314/bridge.exe','gate':'All eligible queue only; every pair/boundary/mapping; zero invalidity;2 better-action roots2families certificate gains for separate fresh design only.',
        'production_change':False,'holdout_authority':False})
    print(json.dumps({'manifest_sha256':digest(M),'cases':12,'eligible_candidates':sum(len(c['candidates']) for c in cases)}))

def run():
    m = load(M); verify_local(m); bridge = Bridge(ROOT/m['bridge']); cases = []
    try:
        for c in m['cases']:
            candidates = c['candidates']; results = []; pair_boundaries = 0; tried_mappings = 0; dual_days = 0
            for r in candidates:
                baseline = replay(bridge,c['setup'],c['state'],c['ledger'],r['plans']); dual_days += 4
                require(baseline == r['reference'],'baseline identity')
                best = baseline['score']; best_plan = r['plans']; attempts = []
                for donor in candidates:
                    for boundary in (1,2,3):
                        pair_boundaries += 1; tried_mappings += 6
                        rn = r['reference']['nodes'][boundary]['state']['agents']
                        dn = donor['reference']['nodes'][boundary]['state']['agents']
                        for mapping in mappings(rn,dn):
                            plans = splice(r['plans'],donor['plans'],boundary,mapping)
                            output = replay(bridge,c['setup'],c['state'],c['ledger'],plans); dual_days += 4
                            require(tuple(output['score']) <= tuple(r['exact_value']),'328 optimum contradicted')
                            delta = comparison(output['score'],baseline['score'])
                            attempts.append({'donor':donor['label'],'boundary_day':boundary+1,'mapping':mapping,
                                             'score':output['score'],'versus_control':delta,'plan':plans})
                            if tuple(output['score']) > tuple(best): best,best_plan = output['score'],plans
                results.append({'label':r['label'],'audit':r['audit'],'control':baseline['score'],'best':best,
                    'best_plan':best_plan,'exact_value':r['exact_value'],'versus_control':comparison(best,baseline['score']),
                    'action_vs_selected_exact':comparison(r['exact_value'],c['selected_exact_value']),
                    'eligible_attempts':attempts})
            cases.append({'seed':c['seed'],'family':c['family'],'players':c['players'],'candidates':results,
                'pair_boundaries':pair_boundaries,'attempted_mappings':tried_mappings,'dual_days':dual_days})
    finally: close_bridge(bridge)
    verify_local(m)
    qualified = [c for c in cases if any(r['versus_control']['result']=='win' and
                 r['action_vs_selected_exact']['result']=='win' for r in c['candidates'])]
    report = {'experiment':ID,'manifest_sha256':digest(M),'cases':cases,'complete':len(cases)==12,
        'eligible_candidates':sum(len(c['candidates']) for c in cases),'zero_invalid_or_dual_mismatch':True,
        'pair_boundaries':sum(c['pair_boundaries'] for c in cases),'attempted_mappings':sum(c['attempted_mappings'] for c in cases),
        'eligible_splices':sum(len(r['eligible_attempts']) for c in cases for r in c['candidates']),
        'dual_days':sum(c['dual_days'] for c in cases),'qualified_roots':[c['seed'] for c in qualified],
        'design_qualified':len(qualified)>=2 and len({c['family'] for c in qualified})>=2,
        'candidate_wtl':dict(Counter(r['versus_control']['result'] for c in cases for r in c['candidates'])),
        'strata':{field:{str(k):dict(Counter(r['versus_control']['result'] for c in cases if c[field]==k for r in c['candidates']))
                         for k in sorted({c[field] for c in cases})} for field in ('family','players')},
        'production_change':False,'holdout_authority':False,'score_promotion_authority':False}
    write_new(S,report)
    print(json.dumps({k:v for k,v in report.items() if k!='cases'}))
    print('summary_sha256='+digest(S))

if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','run'));a=p.parse_args()
    freeze() if a.mode=='freeze' else run()
