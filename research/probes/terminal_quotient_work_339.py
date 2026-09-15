"""Consumed-root attribution only; no live actions, holdout or promotion authority."""
import argparse
from collections import Counter
import json
from pathlib import Path
import run_pair_score_338 as frozen
from run_http_baseline_314 import ROOT, Bridge, digest
from selected_horizon_pricing_331 import selected_context
from summarize_http_baseline_314 import require

ID = 'ATTR-PAIR-TERMINAL-QUOTIENT-WORK-339'
M = ROOT/f'research/holdouts/{ID}.json'
E = ROOT/f'research/holdouts/{ID}-execution.json'
D = ROOT/f'research/evidence/{ID}'
S = ROOT/f'research/evidence/{ID}.summary.json'
load = frozen.load
write_new = frozen.write_new


def verify(m):
    for p,h in m['hashes'].items(): require(digest(ROOT/p)==h, 'frozen drift: '+p)


def freeze_inputs():
    m=load(frozen.M); frozen.verify(m); summary=load(frozen.S)
    require(digest(frozen.S)=='54B87EE30BCDE3AE1D93E6E705BCC6A706862376FC30D7783EE181F192C50B17', '338 summary drift')
    require(summary['complete'] and not summary['gate_passed'], '338 gate not closed')
    inputs=load(ROOT/m['development'])['cases']; cases=[]
    hashes={str(frozen.M.relative_to(ROOT)):digest(frozen.M),str(frozen.S.relative_to(ROOT)):digest(frozen.S)}
    judge=Bridge(ROOT/m['bridge_binary'])
    try:
        for c in inputs:
            frozen.validate_side(c,'parent')
            p=(frozen.D/'parent'/str(c['spec']['seed'])).with_suffix('.replay.jsonl')
            body=next(e['body'] for e in map(json.loads,p.read_text().splitlines()) if e['kind']=='decision')
            ctx=selected_context(body,c['setup'],judge)
            cases.append({'spec':c['spec'],'context':ctx})
            hashes[str(p.relative_to(ROOT))]=digest(p)
    finally:
        judge.close();judge.process.stdout.close();judge.process.stderr.close()
    require(len(cases)==24, 'all24 roots required')
    write_new(M,{'experiment':ID,'cases':cases,'hashes':hashes,'holdout_authority':False,
        'selection':'ALL24 parent-selected day1 roots from completed338, no score filter',
        'gate':'zero correctness/plan-equivalence/rollback failure; no338 complete regresses; >=4new completions2families and >=4strict witness gains2families; fresh SCORE only'})
    print('input_sha256='+digest(M))


def freeze_execution():
    m=load(M);verify(m)
    paths=list(load(frozen.M)['hashes'])+list(m['hashes'])+[str(M.relative_to(ROOT)),
        'research/probes/terminal_quotient_work_339.py','research/probes/test_terminal_quotient_339.py',
        'research/probes/bounded_pair_probe_338.cpp','artifacts/research/339/horizon_pricing.cpp',
        'artifacts/research/338/source/include/udon/horizon_pricing.hpp',
        'artifacts/research/338/build/udon_shield.lib','artifacts/research/338/probe.exe',
        'artifacts/research/339/probe.exe','artifacts/research/314/bridge.exe',
        'research/probes/run_pair_score_338.py','research/probes/run_http_baseline_314.py',
        'research/probes/summarize_http_baseline_314.py','research/probes/selected_horizon_pricing_331.py']
    write_new(E,{'experiment':ID,'input':str(M.relative_to(ROOT)),
        'hashes':{p:digest(ROOT/p) for p in sorted(set(paths))},'caps':'unchanged338',
        'parent':'artifacts/research/338/probe.exe','candidate':'artifacts/research/339/probe.exe',
        'bridge':'artifacts/research/314/bridge.exe'})
    print('execution_sha256='+digest(E))


def validate(c,out,judge):
    q=c['context']['request']
    require(out.get('ok') and out['originalUnchanged'] and out['failures']==0,'probe safety')
    require(out['baseline']==c['context']['baseline'] and out['supported']==1,'root identity')
    require(out['completed']+out['exhausted']==1,'completion identity')
    for k,n in {'settled':32768,'created':196609,'actions':262144,'memo':8192,'transitions':262144,'queries':1024}.items():
        require(0<=out[k]<=n,'cap exceeded: '+k)
    if not out['completed']: require(out['plans']==q['plans'] and out['score']==out['baseline'],'partial replacement')
    require(tuple(out['score'])>=tuple(out['baseline']),'nonregression certificate')
    require(len(out['plans'])==3,'suffix length')
    fixed=[a for a in range(3) if all(out['plans'][i][a]==q['plans'][i][a] for i in range(3))]
    require(fixed,'pair identity changed during horizon')
    state,ledger=q['state'],q['ledger']
    for i,plan in enumerate(out['plans']):
        s=judge.request({'op':'step','setup':q['setup'],'state':state,'ledger':ledger,'plan':plan})
        require(s.get('ok') and s['agrees'],'dual validation')
        require(all(s['agents'][a]==c['context']['original_days'][i]['agents'][a] for a in fixed),'fixed state drift')
        state={**state,'day':state['day']+1,'agents':s['agents']};ledger=s['ledger']
    require(s['score']==out['score'],'final ledger mismatch')


def run():
    e=load(E);verify(e);m=load(M);verify(m);D.mkdir(exist_ok=False)
    probes={s:Bridge(ROOT/e[s]) for s in ('parent','candidate')};judge=Bridge(ROOT/e['bridge'])
    try:
        for i,c in enumerate(m['cases']):
            outputs={}
            for side in (('parent','candidate') if i%2==0 else ('candidate','parent')):
                q={**c['context']['request'],'op':'price','expired':False,'corrupt':False}
                out=probes[side].request(q);validate(c,out,judge);outputs[side]=out
            write_new(D/(str(c['spec']['seed'])+'.result.json'),{'spec':c['spec'],'outputs':outputs,'execution_sha256':digest(E)})
    finally:
        for b in [*probes.values(),judge]: b.close();b.process.stdout.close();b.process.stderr.close()
    verify(e)
    write_new(D/'run_complete.json',{'cases':24,'responses':48,'dual_days':144,'execution_sha256':digest(E),
        'hashes':{p.name:digest(p) for p in sorted(D.glob('*.result.json'))}})
    summarize()


def summarize():
    e=load(E);verify(e);m=load(M);marker=load(D/'run_complete.json')
    require(marker['cases']==24 and marker['responses']==48 and marker['execution_sha256']==digest(E),'closure identity')
    require(marker['hashes']=={p.name:digest(p) for p in sorted(D.glob('*.result.json'))} and len(marker['hashes'])==24,'result drift')
    rows=[];new=[];gains=[];equal=0;regress=[];totals={s:Counter() for s in ('parent','candidate')}
    judge=Bridge(ROOT/e['bridge'])
    try:
        for c in m['cases']:
            r=load(D/(str(c['spec']['seed'])+'.result.json')); require(r['spec']==c['spec'],'spec mismatch')
            a,b=r['outputs']['parent'],r['outputs']['candidate']
            for side,out in r['outputs'].items():
                validate(c,out,judge)
                totals[side].update({k:out[k] for k in ('completed','exhausted','improvements','settled','created','actions','memo','transitions','queries')})
            if a['completed'] and b['completed']:
                require(a['score']==b['score'] and a['plans']==b['plans'],'complete equivalence failed');equal+=1
            if a['completed'] and not b['completed']:regress.append(c['spec'])
            if not a['completed'] and b['completed']:new.append(c['spec'])
            if b['completed'] and tuple(b['score'])>tuple(b['baseline']):gains.append(c['spec'])
            rows.append(r)
    finally:judge.close();judge.process.stdout.close();judge.process.stderr.close()
    gate=not regress and len(new)>=4 and len({x['family'] for x in new})>=2 and len(gains)>=4 and len({x['family'] for x in gains})>=2
    result={'experiment':ID,'complete':True,'cases':24,'responses':48,'dual_days':144,
        'both_complete_exact_score_and_plan':equal,'new_completions':new,'completion_regressions':regress,
        'strict_witness_gains':gains,'totals':totals,'zero_safety_equivalence_failure':True,
        'gate_passed':gate,'rows':rows,'execution_sha256':digest(E),'completion_sha256':digest(D/'run_complete.json'),
        'authority':'consumed-root work/certificate attribution only; no played-score, latency or promotion authority'}
    write_new(S,result)
    print(json.dumps({k:v for k,v in result.items() if k!='rows'},indent=2));print('summary_sha256='+digest(S))


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze-inputs','freeze-execution','run','summarize'))
    {'freeze-inputs':freeze_inputs,'freeze-execution':freeze_execution,'run':run,'summarize':summarize}[p.parse_args().mode]()
