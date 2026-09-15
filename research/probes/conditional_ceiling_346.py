"""All complete344 day2 roots; exact offline oracle, never a timed solver rerun."""
import argparse
from collections import Counter
import copy
import json
from pathlib import Path
import subprocess
import time
import witness_realization_345 as previous
from http_prefix_option_loss_321 import object_hash, state, validate_result
from ack_suffix_consumption_333 import score, check_step

ROOT=previous.ROOT
ID='ATTR-CURRENT-FLOOR-CONDITIONAL-CEILING-346'
M=ROOT/f'research/holdouts/{ID}.json'
D=ROOT/f'research/evidence/{ID}'
S=ROOT/f'research/evidence/{ID}.summary.json'
PROBE=ROOT/'artifacts/research/346/probe.exe'
load,require,digest,verify,write_new=previous.load,previous.require,previous.digest,previous.verify,previous.write_new


def deduplicate(references):
    unique={}
    for ref in references:
        key=object_hash(ref['request'])
        if key in unique:require(unique[key]['request']==ref['request'],'hash collision')
        else:unique[key]={'id':key,'after_day':2,'request':ref['request'],'references':[]}
        unique[key]['references'].append({k:v for k,v in ref.items() if k!='request'})
    return list(unique.values())


def strict_gate(rows):
    wins=[r for r in rows if r['inherited_certificate']>r['conditional_optimum']]
    return len({r['seed'] for r in wins})>=2 and len({r['family'] for r in wins})>=2


def freeze():
    pm=load(previous.M);verify(pm)
    require(digest(previous.S)=='61A804C1625ACBF82CF9FDF1060015A014325FCDB7A8D6BBAE1F8F9843BA536B','345 report drift')
    pr=load(previous.S)
    require(pr['complete'] and pr['boundaries']==288 and pr['dual_days']==864,'345 incomplete')
    _,cases,_=previous.complete344()
    bykey={(r['seed'],r['repeat'],r['side']):r for r in pr['rows'] if r['after_day']==1}
    matches={(r['seed'],r['repeat'],r['side']):r for r in pr['matches']}
    refs=[]
    for c in cases:
        for label,(repeat,side) in previous.source.LABELS.items():
            seed=previous.source.seed(c);p=previous.source.location(c,label,previous.D)
            bodies=[e['body'] for line in p.with_suffix('.replay.jsonl').read_text().splitlines()
                if (e:=json.loads(line))['kind']=='decision']
            b=bodies[2];root={'setup':c['setup'],'state':state(b['state']['agents'],3),'ledger':b['ledger']}
            require(b['state']['day']==3 and not b['state']['others'] and not b['state']['traffics'],'exact day3 domain')
            old=bykey[seed,repeat,side];day2=matches[seed,repeat,side]['days'][1]
            refs.append({'seed':seed,'family':c['spec']['family'],'players':c['setup']['players'],'repeat':repeat,'side':side,
                'request':root,'selected_certificate':day2['certificate'],'selected_upper':day2['valid_upper'],
                'inherited_certificate':old['witness_score'],'actual_final':old['actual_final_score'],
                'source_replay_sha256':old['replay_sha256'],'paired_result':old['paired_result']})
    require(len(refs)==96,'reference coverage')
    paths={Path(__file__),Path(__file__).with_name('test_conditional_ceiling_346.py'),previous.M,previous.S,PROBE,
        ROOT/'research/probes/build_conditional_346.cmd',ROOT/'build-release/udon_shield.lib',
        ROOT/'research/probes/http_prefix_oracle_321.cpp',ROOT/'research/probes/multi_patrol_oracle.cpp',
        ROOT/'research/probes/test_http_prefix_oracle_321.py',ROOT/'research/probes/test_http_prefix_option_loss_321.py',
        ROOT/f'research/evidence/{ID}-preregistration.md'}
    hashes={**pm['hashes'],**{str(p.relative_to(ROOT)).replace('\\','/'):digest(p) for p in paths}}
    write_new(M,{'experiment':ID,'cases':deduplicate(refs),'references':96,'bridge':pm['bridge'],
        'probe':'artifacts/research/346/probe.exe','probe_sha256':digest(PROBE),'hashes':hashes,
        'root_time_normalization':'endsAt=0 for timeless exact oracle only; all gameplay state/ledger retained',
        'authority':'Offline consumed DEVELOPMENT conditional ceiling; no production/holdout/performance authority.'})
    print(json.dumps({'manifest_sha256':digest(M),'unique_requests':len(load(M)['cases']),'references':96,'dependencies':len(hashes)}))


def execute():
    import msvcrt
    m=load(M);mh=digest(M);verify(m);probe=ROOT/m['probe']
    D.mkdir(exist_ok=True)
    with (D/'runner.lock').open('a+b') as lock:
        lock.seek(0);msvcrt.locking(lock.fileno(),msvcrt.LK_NBLCK,1)
        try:
            require(not list(D.glob('*.partial')),'ambiguous partial; manual recovery only')
            names={c['id']+'.result.json' for c in m['cases']}
            require({p.name for p in D.glob('*.result.json')}<=names,'foreign result')
            for i,c in enumerate(m['cases']):
                out=D/(c['id']+'.result.json')
                if out.exists():validate_result(load(out),c,mh,m['probe_sha256']);continue
                partial=D/(c['id']+'.partial');err=D/(c['id']+'.stderr')
                require(not err.exists() and not (D/(c['id']+'.stdout.json')).exists(),'orphan side')
                require(digest(probe)==m['probe_sha256'],'binary drift')
                start=time.monotonic()
                with partial.open('x',encoding='utf-8') as stdout,err.open('x',encoding='utf-8') as stderr:
                    p=subprocess.run([str(probe)],input=json.dumps(c['request'])+'\n',text=True,stdout=stdout,stderr=stderr)
                require(p.returncode==0 and err.stat().st_size==0,'oracle process failure')
                result={'id':c['id'],'manifest_sha256':mh,'probe_sha256':m['probe_sha256'],
                    'request_sha256':object_hash(c['request']),'oracle':load(partial),'elapsed_seconds':time.monotonic()-start}
                validate_result(result,c,mh,m['probe_sha256'])
                commit=D/(c['id']+'.commit.partial');write_new(commit,result);commit.rename(out)
                partial.rename(D/(c['id']+'.stdout.json'))
                print(json.dumps({'event':'case_complete','count':i+1,'total':len(m['cases']),'seconds':result['elapsed_seconds']}),flush=True)
            verify(m)
            marker={'experiment':ID,'manifest_sha256':mh,'cases':len(m['cases']),'references':96,
                'results':{p.name:digest(p) for p in sorted(D.glob('*.result.json'))}}
            require(len(marker['results'])==len(m['cases']),'incomplete set')
            if (D/'run_complete.json').exists():require(load(D/'run_complete.json')==marker,'completion drift')
            else:write_new(D/'run_complete.json',marker)
        finally:
            lock.seek(0);msvcrt.locking(lock.fileno(),msvcrt.LK_UNLCK,1)
    print(json.dumps({'event':'run_complete','cases':len(m['cases']),'references':96}),flush=True)
    summarize()


def summarize():
    m=load(M);mh=digest(M);verify(m);marker=load(D/'run_complete.json')
    require(marker['manifest_sha256']==mh and marker['cases']==len(m['cases']) and marker['references']==96,'completion')
    require(set(marker['results'])=={c['id']+'.result.json' for c in m['cases']},'coverage')
    require(not list(D.glob('*.partial')),'partial evidence')
    rows=[];dual=0;work=Counter();bridge=previous.source.Bridge(ROOT/m['bridge'])
    try:
        for c in m['cases']:
            p=D/(c['id']+'.result.json');require(digest(p)==marker['results'][p.name],'result hash')
            r=load(p);validate_result(r,c,mh,m['probe_sha256']);o=r['oracle']
            st=copy.deepcopy(c['request']['state']);ledger=copy.deepcopy(c['request']['ledger'])
            for d in o['days']:
                require(d['day']==st['day'],'day identity')
                step=check_step(bridge,c['request']['setup'],st,ledger,d['plan']);dual+=1
                require(all(step[k]==d[k] for k in ('agents','ledger','score')),'independent reconstruction')
                st={**st,'day':st['day']+1,'agents':step['agents']};ledger=step['ledger']
            for k in ('memo_states','day_enumerations','joint_transitions'):work[k]+=o[k]
            for ref in c['references']:
                optimum=o['score']
                require(ref['selected_certificate']<=optimum and ref['actual_final']<=optimum<=ref['selected_upper'],'soundness mismatch')
                rows.append({**ref,'conditional_optimum':optimum,'request_id':c['id'],
                    'inherited_strictly_dominates':ref['inherited_certificate']>optimum,
                    'constructive_gap':optimum>ref['selected_certificate'],
                    'upper_gap':ref['selected_upper']>optimum,'oracle_result_sha256':digest(p)})
    finally:
        bridge.close();bridge.process.stdout.close();bridge.process.stderr.close()
    require(len(rows)==96 and dual==2*len(m['cases']),'full coverage')
    def count(rr):return {'references':len(rr),'strict_dominance':sum(x['inherited_strictly_dominates'] for x in rr),
        'constructive_gap':sum(x['constructive_gap'] for x in rr),'upper_gap':sum(x['upper_gap'] for x in rr)}
    report={'experiment':ID,'complete':True,'unique_requests':len(m['cases']),'dual_days':dual,**count(rows),
        'gate_passed':strict_gate(rows),'work':dict(work),'zero_reconstruction_bound_failure':True,'rows':rows,
        'strata':{k:{str(v):count([x for x in rows if x[k]==v]) for v in sorted({x[k] for x in rows})}
            for k in ('family','side','repeat','players','paired_result')},
        'manifest_sha256':mh,'completion_sha256':digest(D/'run_complete.json'),'authority':m['authority']}
    if S.exists():require(load(S)==report,'summary drift')
    else:write_new(S,report)
    print(json.dumps({k:v for k,v in report.items() if k not in ('rows','strata')}),flush=True)
    print('summary_sha256='+digest(S),flush=True)


if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=('freeze','execute','summarize'));a=p.parse_args()
    {'freeze':freeze,'execute':execute,'summarize':summarize}[a.mode]()
