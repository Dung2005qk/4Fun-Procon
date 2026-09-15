"""Isolated mechanical VM transport/package staging, after input freeze."""
import hashlib
import argparse
import shutil
import tarfile
from refinement_cost_353 import ROOT,sha,load,write,require
from prepare_portable_352 import portable,policy
ID='SCORE-PROTECTED-RESOURCE-MARGINAL-357'
W=ROOT/'artifacts/research/357'
def main():
    parser=argparse.ArgumentParser();parser.add_argument('--revision',default='');args=parser.parse_args()
    require(args.revision in ('','preflight2'),'explicit packaging revision')
    suffix=('-'+args.revision)if args.revision else ''
    inputs=ROOT/f'research/holdouts/{ID}.json'
    require(sha(inputs)=='66131A8A13A9CD1F32A7184A7E0EB6D8EC96EF3604166721827E7C2C87F5C001','input identity')
    m=load(inputs)
    for p,h in m['hashes'].items():require(sha(ROOT/p)==h,'input dependency drift')
    for x in m['splits'].values():require(sha(ROOT/x['path'])==x['sha256'],'sealed identity')
    dest=W/('package'+suffix);require(not dest.exists(),'package already exists');dest.mkdir()
    shutil.copytree(W/'source',dest/'source')
    (dest/'research/probes').mkdir(parents=True)
    for dependency in (ROOT/'research/probes').glob('*.py'):
        shutil.copy2(dependency,dest/'research/probes'/dependency.name)
    p=dest/'source/src/btc_main.cpp';original=p.read_text()
    shim=(ROOT/'research/probes/process_rpc_352.hpp').read_text()
    transformed=portable(original,shim);require(policy(original)in transformed,'full daily policy preservation')
    p.write_text(transformed,encoding='utf-8',newline='\n')
    paths=['research/probes/resource_contract_357.cpp','research/probes/run_resource_357.py',
        'research/probes/build_resource_357.sh',f'research/evidence/{ID}-preregistration.md',
        'artifacts/research/354/input.json',f'research/holdouts/{ID}.json']
    # Upload development only. Sealed grids remain local until their gates qualify.
    paths.append(m['splits']['development']['path'])
    for name in paths:
        dst=dest/name;dst.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(ROOT/name,dst)
    source_hashes={p.relative_to(W/'source').as_posix():sha(p)for p in (W/'source').rglob('*')if p.is_file()}
    write(dest/'source357.manifest.json',{'source_hashes':source_hashes,'input_manifest_sha256':sha(inputs),
        'daily_policy_sha256':hashlib.sha256(policy(original).encode()).hexdigest().upper(),
        'original_btc_sha256':sha(W/'source/src/btc_main.cpp'),'portable_btc_sha256':sha(dest/'source/src/btc_main.cpp')})
    write(dest/'package357.manifest.json',{'hashes':{p.relative_to(dest).as_posix():sha(p)for p in dest.rglob('*')if p.is_file()}})
    with tarfile.open(W/('package'+suffix+'.tar.gz'),'w:gz')as archive:
        for p in sorted(dest.rglob('*')):
            if p.is_file():archive.add(p,arcname=p.relative_to(dest).as_posix())
    print('357 package',sha(W/('package'+suffix+'.tar.gz')),'manifest',sha(dest/'package357.manifest.json'))
if __name__=='__main__':main()
