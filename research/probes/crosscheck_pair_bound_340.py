"""Independent consumed-root optimum check; never supplies oracle input to340."""
import json
from pathlib import Path
from run_http_baseline_314 import ROOT,Bridge,digest
from run_pair_score_338 import load,write_new
from summarize_http_baseline_314 import require
import pair_stock_bound_340 as task


def main():
    m=load(task.base.M);task.base.verify(m);e=load(task.base.E);task.base.verify(e)
    s=load(task.base.S);require(s['complete'] and s['gate_passed'],'340 incomplete')
    old=load(ROOT/'research/holdouts/ATTR-W1-PAIR-RESOURCE-RESPONSE-336.json')
    for p,h in old['local_hashes'].items():require(digest(ROOT/p)==h,'336 drift')
    d=ROOT/'research/evidence/ATTR-PAIR-STOCK-RELAXATION-BOUND-340-crosscheck'
    manifest=ROOT/'research/holdouts/ATTR-PAIR-STOCK-RELAXATION-BOUND-340-crosscheck.json'
    hashes={p:digest(ROOT/p) for p in (str(task.base.M.relative_to(ROOT)),str(task.base.S.relative_to(ROOT)),
        str(Path(__file__).relative_to(ROOT)),'research/probes/pair_resource_response_336.cpp',old['probe'],e['bridge'])}
    write_new(manifest,{'cases':24,'responses':72,'memo_limit':250000,'transition_limit':30000000,
        'hashes':hashes,'authority':'Independent336 conditional optima after340 frozen result; not oracle guidance, fresh score or performance.'})
    d.mkdir(exist_ok=False);p=Bridge(ROOT/old['probe']);judge=Bridge(ROOT/e['bridge']);dual=0
    try:
        for c,r in zip(m['cases'],s['rows'],strict=True):
            require(c['spec']==r['spec'],'case identity')
            q={**c['context']['request'],'memo_limit':250000,'transition_limit':30000000}
            o=p.request(q);require(o.get('ok') and o['complete'],'oracle incomplete')
            require([v['pair'] for v in o['responses']]==[[0,1],[0,2],[1,2]],'coverage')
            require(max(v['score'] for v in o['responses'])==r['outputs']['candidate']['score'],'bounded optimum mismatch')
            for response in o['responses']:
                state,ledger=q['state'],q['ledger'];fixed=next(a for a in range(3) if a not in response['pair'])
                for i,day in enumerate(response['days']):
                    require(day['plan'][fixed]==q['plans'][i][fixed],'fixed route changed')
                    checked=judge.request({'op':'step','setup':q['setup'],'state':state,'ledger':ledger,'plan':day['plan']})
                    require(checked.get('ok') and checked['agrees'] and all(checked[k]==day[k] for k in ('score','agents','ledger')),'dual mismatch')
                    state={**state,'day':state['day']+1,'agents':checked['agents']};ledger=checked['ledger'];dual+=1
            write_new(d/(str(c['spec']['seed'])+'.result.json'),{'spec':c['spec'],'oracle':o,'bounded_score':r['outputs']['candidate']['score'],'equal':True})
    finally:
        for b in (p,judge):b.close();b.process.stdout.close();b.process.stderr.close()
    require(dual==216,'coverage mismatch')
    for path,h in hashes.items():require(digest(ROOT/path)==h,'dependency changed')
    out=ROOT/'research/evidence/ATTR-PAIR-STOCK-RELAXATION-BOUND-340-crosscheck.json'
    write_new(out,{'complete':True,'cases':24,'responses':72,'dual_days':dual,'equal_optima':24,
        'manifest_sha256':digest(manifest),'results':{x.name:digest(x) for x in sorted(d.glob('*.result.json'))}})
    print('independent_optima_equal=24 dual_days=216 hash='+digest(out))


if __name__=='__main__':main()
