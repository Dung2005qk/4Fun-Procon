"""One emit-observation query plus untouched operation-equivalence control."""
import argparse
import json
import re
import subprocess
from refinement_cost_353 import ROOT,sha,load,write,require
ID='ATTR-TARGET-EMISSION-RETENTION-355'
W=ROOT/'artifacts/research/355'
D=ROOT/f'research/evidence/{ID}'
def marked(s):return '// BEGIN OBS355\n'+s+'\n// END OBS355\n'
def prepare():
    original=ROOT/'artifacts/research/352/package/parent/src/orienteering.cpp'
    require(sha(original)==load(ROOT/'artifacts/research/352/execution352.json')['hashes']['parent/src/orienteering.cpp'],'original source identity')
    source=original.read_text();start=source.index('ExactOrienteeringReachability enumerate_sparse_anytime_resource_routes_impl(');end=source.index('\nExactOrienteeringReachability enumerate_resource_routes(',start)
    body=source[start:end]
    additions=[('    std::vector<SparseRouteChoice> retained;\n',marked('    std::vector<SparseRouteChoice> emittedChoices355;')),
        ('            const SparseRouteRank rank = route_rank(current);\n',marked('            emittedChoices355.push_back(SparseRouteChoice{rank,labelIndex});'))]
    for anchor,extra in additions:
        require(body.count(anchor)==1,'observer anchor');body=body.replace(anchor,anchor+extra)
    anchor='    const auto append_choices = [&reconstruct]('
    extra=marked('    for(const auto& choice:emittedChoices355){\n        std::vector<std::int64_t> rank;\n        std::apply([&](auto... x){(rank.push_back(static_cast<std::int64_t>(x)),...);},choice.rank);\n        udon::observed355.push_back({reconstruct(choice.label),std::move(rank)});\n    }')
    require(body.count(anchor)==1,'reconstruction anchor');body=body.replace(anchor,extra+anchor)
    observed=marked('#include "observer355.hpp"')+source[:start]+body+source[end:]
    require(re.sub(r'// BEGIN OBS355\n.*?// END OBS355\n','',observed,flags=re.S)==source,'strip equivalence')
    W.mkdir(parents=True,exist_ok=True)
    with (W/'orienteering_observed.cpp').open('x',newline='\n')as f:f.write(observed)
    paths=['research/probes/target_retention_355.py','research/probes/target_retention_355.cpp','research/probes/observer355.hpp',
        'research/probes/refinement_cost_353.py',f'research/evidence/{ID}-preregistration.md',
        'artifacts/research/354/input.json','artifacts/research/355/orienteering_observed.cpp']
    write(W/'manifest.json',{'experiment':ID,'original_source_sha256':sha(original),
        'hashes':{p:sha(ROOT/p)for p in paths},'strip_equivalent':True,'order':['control','observed']})
    print('355 manifest frozen',sha(W/'manifest.json'))
def vm():
    require(subprocess.check_output(['hostname'],text=True).strip()=='udon-f0-240-0829','authorized VM')
    require(not D.exists() and not (W/'execution.json').exists(),'existing evidence; no duplication')
    m=load(W/'manifest.json')
    for p,h in m['hashes'].items():require(sha(ROOT/p)==h,'frozen drift '+p)
    require(sha(ROOT/'parent/src/orienteering.cpp')==m['original_source_sha256'],'original source')
    observed=(W/'orienteering_observed.cpp').read_text()
    require(re.sub(r'// BEGIN OBS355\n.*?// END OBS355\n','',observed,flags=re.S)==(ROOT/'parent/src/orienteering.cpp').read_text(),'remote strip equivalence')
    lib='build-parent/libudon_shield.a';require(sha(ROOT/lib)==load(ROOT/'execution352.json')['hashes'][lib],'original library')
    hashes=dict(m['hashes']);hashes[lib]=sha(ROOT/lib)
    for side in m['order']:
        cmd=['g++','-std=c++20','-O3','-DNDEBUG','-pthread','-I',str(ROOT/'parent/include'),'-I',str(ROOT/'research/probes'),str(ROOT/'research/probes/target_retention_355.cpp')]
        if side=='observed':cmd.append(str(W/'orienteering_observed.cpp'))
        cmd.extend([str(ROOT/lib),'-o',str(W/side)]);subprocess.run(cmd,check=True)
        hashes[f'artifacts/research/355/{side}']=sha(W/side)
    write(W/'execution.json',{'hashes':hashes,'manifest_sha256':sha(W/'manifest.json'),
        'compiler':subprocess.check_output(['g++','--version'],text=True)})
    D.mkdir()
    for side in m['order']:
        p=subprocess.run([str(W/side)],input=json.dumps(load(ROOT/'artifacts/research/354/input.json')),capture_output=True,text=True,timeout=45)
        for suffix,data in [('stdout',p.stdout),('stderr',p.stderr)]:
            with (D/f'{side}.{suffix}').open('x')as f:f.write(data)
        require(p.returncode==0 and not p.stderr,'operation failure, evidence preserved')
        r=json.loads(p.stdout);require(r.pop('input_echo')==load(ROOT/'artifacts/research/354/input.json'),'input mutation')
        write(D/f'{side}.json',r)
    a,b=(load(D/f'{x}.json')for x in m['order'])
    require(all(a[k]==b[k] for k in ['supported','complete','settled','retained']) and a['settled']==1250000,'operation equivalence failed')
    require(not a['emitted'] and b['emitted'],'observer scope')
    write(D/'run_complete.json',{'execution_sha256':sha(W/'execution.json'),
        'files':{p.name:sha(p)for p in D.iterdir()if p.is_file()},'operation_equivalent':True})
    check()
def check():
    c=load(D/'run_complete.json');require(sha(W/'execution.json')==c['execution_sha256'],'execution drift')
    for p,h in c['files'].items():require(sha(D/p)==h,'raw drift')
    a,b=(load(D/f'{x}.json')for x in ['control','observed'])
    for side in ['control','observed']:
        raw=load(D/f'{side}.stdout');require(raw.pop('input_echo')==load(ROOT/'artifacts/research/354/input.json'),'raw input');require(raw==load(D/f'{side}.json'),'raw equivalence')
    require(all(a[k]==b[k]for k in ['supported','complete','settled','retained']),'operation equivalence')
    gains=[r for r in b['emitted']if r['safe'] and r['score']>[6,6,64]]
    print('355 complete',sha(D/'observed.json'),'emissions',len(b['emitted']),'safe gains',len(gains),
        'witness mask',b['goal_mask'],'mask_emitted',sum(r['mask']==b['goal_mask']for r in b['emitted']),
        'exact_witness',sum(r['equals_witness']for r in b['emitted']))
    print('gain records',[(r['mask'],r['score'],r['fuel'],r['rank'])for r in gains])
if __name__=='__main__':
    p=argparse.ArgumentParser();p.add_argument('mode',choices=['prepare','vm','check']);a=p.parse_args();{'prepare':prepare,'vm':vm,'check':check}[a.mode]()
