"""Mechanical isolated transport extraction; frozen gameplay is never changed."""
import hashlib
import io
import json
from pathlib import Path
import shutil
import subprocess
import tarfile
from run_pair_score_341 import ROOT, load, digest, write_new, require

ID='ATTR-PORTABLE-RUNTIME-LOSS-REPEAT-352'
DEST=ROOT/'artifacts/research/352/package'
SEEDS=(2026090835090164,2026090835090213,2026090835090127)
PARENT='c76a8eaa4f200e3eeeb1a58ef1d1fb3d0c13579c'
START='\n#ifdef _WIN32\n\n[[nodiscard]] std::wstring utf8_to_wide'
POLICY='[[nodiscard]] bool transient_http_status'
END='\n#else\n\nvoid run_http(const RuntimeOptions&)'

def policy(text):
    a=text.index(POLICY);b=text.index(END,a)
    return text[a:b]

def portable(text,shim):
    a=text.index(START);b=text.index(END,a);c=text.index('\n#endif',b)+len('\n#endif')
    out='#include <cstring>\n'+text[:a]+'\n'+shim+'\n'+policy(text)+text[c:]
    require(out.count('void run_http(')==1,'one runtime entry')
    require(policy(text) in out,'original entire policy lost')
    return out

def prepare():
    require(not DEST.exists(),'preparation already exists')
    require(subprocess.check_output(['git','rev-parse','HEAD'],cwd=ROOT,text=True).strip()==PARENT,'parent drift')
    mpath=ROOT/'research/holdouts/SCORE-CERTIFIED-ANYTIME-PAIR-INCUMBENT-350-execution.json'
    require(digest(mpath)=='A60CC5C0E4438D50BC7ABD50444E7EC9BF5F7AEFD095E3B1B87192B029A53B7B','350 provenance')
    m=load(mpath);split=ROOT/m['splits']['broad-development']['path']
    require(digest(split)==m['splits']['broad-development']['sha256'],'DEV split drift')
    cases={c['seed']:c for c in load(split)['cases']}
    chosen=[cases[s] for s in SEEDS]
    require(sum(c['days'] for c in chosen)*4==96,'day count')
    DEST.mkdir(parents=True)
    paths=['CMakeLists.txt','src','include','strategies','tests','bench']
    raw=subprocess.check_output(['git','archive',PARENT,*paths],cwd=ROOT)
    parent=DEST/'parent';parent.mkdir()
    with tarfile.open(fileobj=io.BytesIO(raw)) as archive:
        for member in archive.getmembers():
            require(not member.name.startswith('/') and '..' not in Path(member.name).parts,'archive path')
            require(member.isfile() or member.isdir(),'no archive links')
        archive.extractall(parent,filter='data')
    candidate=DEST/'candidate';candidate.mkdir()
    src=ROOT/'artifacts/research/350/source'
    for item in paths:
        p=src/item
        if p.is_dir():shutil.copytree(p,candidate/item)
        elif p.is_file():shutil.copy2(p,candidate/item)
    for p in candidate.rglob('*'):
        if p.is_file():
            original=src/p.relative_to(candidate)
            key=original.relative_to(ROOT).as_posix()
            if p.suffix in ('.cpp','.hpp','.inc') or p.name=='CMakeLists.txt':
                require(m['hashes'][key]==digest(original),'candidate source not frozen')
            require(digest(p)==digest(original),'mechanical source copy drift')
    probe_dir=DEST/'research/probes';probe_dir.mkdir(parents=True)
    # Include all Python imports mechanically; no sealed inputs are packaged.
    for p in (ROOT/'research/probes').glob('*.py'):shutil.copy2(p,probe_dir/p.name)
    for name in ('process_rpc_352.hpp','protected_http_bridge_341.cpp','bounded_pair_probe_340.cpp','claim_cap_probe_347.cpp','build_portable_352.sh'):
        shutil.copy2(ROOT/'research/probes'/name,probe_dir/name)
    ev=DEST/'research/evidence';ev.mkdir(parents=True)
    shutil.copy2(ROOT/f'research/evidence/{ID}-preregistration.md',ev/f'{ID}-preregistration.md')
    shim=(probe_dir/'process_rpc_352.hpp').read_text(encoding='utf-8')
    identities={}
    for side in ('parent','candidate'):
        p=DEST/side/'src/btc_main.cpp';text=p.read_text(encoding='utf-8')
        original=p.with_name('btc_main.original352.txt');original.write_bytes(p.read_bytes())
        with p.open('w',encoding='utf-8',newline='\n') as f:f.write(portable(text,shim))
        identities[side]={'original_btc':digest(original),'portable_btc':digest(p),
            'unchanged_policy_sha256':hashlib.sha256(policy(text).encode()).hexdigest().upper()}
    cm=candidate/'CMakeLists.txt';s=cm.read_text()
    require(s.count('../../../../research/probes/')==2,'expected two relocated research targets')
    with cm.open('w',encoding='utf-8',newline='\n') as f:f.write(s.replace('../../../../research/probes/','../research/probes/'))
    write_new(DEST/'input352.json',{'experiment':ID,'parent_commit':PARENT,
        'cases':chosen,'order':['parentA','candidateA','candidateB','parentB'],
        'source_identities':identities,'authority':'Diagnostic development only; full policy process RPC, no real HTTP or promotion.'})
    files={p.relative_to(DEST).as_posix():digest(p) for p in DEST.rglob('*') if p.is_file()}
    write_new(DEST/'package352.manifest.json',{'hashes':files})
    archive=DEST.parent/'package352.tar'
    with tarfile.open(archive,'w') as out:
        for p in sorted(DEST.rglob('*')):
            if p.is_file():out.add(p,arcname=p.relative_to(DEST).as_posix())
    print({'package_sha256':digest(archive),'input_sha256':digest(DEST/'input352.json'),
        'dependencies':len(files),'identities':identities})

if __name__=='__main__':prepare()
