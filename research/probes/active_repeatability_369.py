"""Four fixed consumed-case attribution sessions; never a promotion rerun."""
import argparse
import hashlib
import json
import os
from pathlib import Path
import subprocess
import sys

WORK = Path('/home/LMC/udon369-0910')
UP = Path('/home/LMC/udon366-0909')
BASE = Path('/home/LMC/udon360-0909')
PKG = Path('/home/LMC/udon357-0909')
DATA = WORK/'data'
EXEC = WORK/'execution369.json'
ID = 'ATTR-ACTIVE-MAIN-REPEATABILITY-369'
SEED = 202609093860641
ORDER = ('parentA', 'candidateA', 'candidateB', 'parentB')
BINARY = BASE/'btc360-plain'
BINARY_SHA = 'D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678'
SETUP_SHA = '02E4C55AD2C8C506239E0B947BB5E2DA955D329C8BADC2B5979135F93A52DEB7'


def sha(p):
    return hashlib.sha256(Path(p).read_bytes()).hexdigest().upper()


def load(p):
    return json.loads(Path(p).read_text(encoding='utf-8'))


def write(p, value):
    p.parent.mkdir(parents=True, exist_ok=True)
    with p.open('x', encoding='utf-8', newline='\n') as stream:
        json.dump(value, stream, indent=2, sort_keys=True)
        stream.write('\n')


def harness():
    assert subprocess.check_output(['hostname'], text=True).strip() == 'udon-f0-240-0829'
    sys.path.insert(0, str(UP))
    import score_resource_366 as upstream
    upstream.verify()
    assert sha(BINARY) == BINARY_SHA
    f = upstream.f
    f.rpc.ID = ID
    return upstream, f


def freeze():
    assert not EXEC.exists() and not DATA.exists()
    upstream, f = harness()
    meta = load(UP/'input366.json')['splits']['protected']
    setup = UP/meta['path']
    assert meta['sha256'] == sha(setup) == SETUP_SHA
    case = next(c for c in load(setup)['cases'] if c['seed'] == SEED)
    assert case['days'] == 10 and case['window_ms'] == 10000
    assert case['role'] == 'fixed-all-Patrol' and case['setup']['daySeconds'] == [10]*10
    paths = {Path(__file__).resolve(), WORK/'preregistration.md', BINARY,
             PKG/'bridge352', UP/'execution366.json', setup}
    for module in tuple(sys.modules.values()):
        file = getattr(module, '__file__', None)
        if file:
            p = Path(file).resolve()
            if p.suffix == '.py' and str(p).startswith('/home/LMC/udon'):
                paths.add(p)
    write(EXEC, {'experiment': ID, 'order': ORDER, 'case': case,
                 'hashes': {str(p): sha(p) for p in sorted(paths)},
                 'authority': 'Consumed-case attribution only; no366 gate change or promotion.'})
    print('execution_frozen', sha(EXEC), flush=True)


def verify():
    doc = load(EXEC)
    assert doc['order'] == list(ORDER) and doc['case']['seed'] == SEED
    for p, h in doc['hashes'].items():
        assert sha(p) == h, 'frozen drift: '+p
    return doc


def prefix(label):
    assert label in ORDER
    return DATA/label[-1]/label[:-1]/str(SEED)


def check_atomic(f, case):
    for label in ORDER:
        p = prefix(label)
        files = list(p.parent.glob(p.name+'.*'))
        if p.with_suffix('.result.json').exists():
            assert p.with_suffix('.certificate.json').exists(), 'result without certificate; no rerun'
            f.validate_side(case, label[:-1], DATA/label[-1])
            assert load(p.with_suffix('.certificate.json'))['independently_checked']
        else:
            assert not files, 'ambiguous partial side; no automatic recovery'


def summarize(f, case):
    transports = {l: load(prefix(l).with_suffix('.transport.json')) for l in ORDER}
    assert all(not t['failure'] and len(t['actions']) == 10 for t in transports.values())
    report = {'experiment': ID, 'results': 4, 'actions': 40, 'transitions': 36,
              'scores': {l: t['score'] for l, t in transports.items()},
              'AB': {r: f.compare_runs(transports['parent'+r], transports['candidate'+r]) for r in ('A','B')},
              'AA': {s: f.compare_runs(transports[s+'A'], transports[s+'B']) for s in ('parent','candidate')},
              'takeovers': {l: [r['day'] for r in load(prefix(l).with_suffix('.certificate.json'))['frames'] if r['takeover']] for l in ORDER},
              'zero_safety_failure': True, 'promotion_authority': False,
              'execution_sha256': sha(EXEC)}
    write(WORK/'summary369.json', report)
    return report


def run():
    _, f = harness()
    doc = verify()
    case = doc['case']
    assert not (DATA/'run_complete.json').exists()
    for label in ORDER:
        prefix(label).parent.mkdir(parents=True, exist_ok=True)
    check_atomic(f, case)
    with f.rpc.BridgeContext() as bridge:
        for label in ORDER:
            p = prefix(label)
            if p.with_suffix('.result.json').exists():
                continue
            f.configure_side(label[:-1])
            f.rpc.run_one(case, BINARY, p, bridge, label[:-1])
            cert = f.certificates(case, p, bridge)
            assert label.startswith('candidate') or not cert['frames']
            write(p.with_suffix('.certificate.json'), cert)
            print('atomic_complete', label, flush=True)
    check_atomic(f, case)
    verify()
    assert len(list(DATA.glob('*/*/*.result.json'))) == 4
    assert len(list(DATA.glob('*/*/*.certificate.json'))) == 4
    report = summarize(f, case)
    write(DATA/'run_complete.json', {'experiment': ID, 'results': 4, 'actions': 40,
          'transitions': 36, 'execution_sha256': sha(EXEC),
          'summary_sha256': sha(WORK/'summary369.json'),
          'files': {p.relative_to(DATA).as_posix(): sha(p) for p in DATA.rglob('*') if p.is_file()}})
    print('run_complete', sha(DATA/'run_complete.json'), flush=True)


def launch():
    assert not (WORK/'runner369.json').exists()
    freeze()
    # Read-only host inventory. Do not kill any competing or unrelated work.
    processes = subprocess.check_output(['ps', '-eo', 'pid,comm,args'], text=True)
    assert not any('btc360-plain' in line or ('python' in line and 'score_resource_' in line)
                   for line in processes.splitlines()), 'another timed experiment is active'
    with (WORK/'runner369.stdout').open('x') as out, (WORK/'runner369.stderr').open('x') as err:
        p = subprocess.Popen([sys.executable, str(Path(__file__).resolve()), 'run'], cwd=WORK,
              stdin=subprocess.DEVNULL, stdout=out, stderr=err, start_new_session=True)
    write(WORK/'runner369.json', {'pid': p.pid, 'execution_sha256': sha(EXEC), 'order': ORDER})
    print('detached369', p.pid, flush=True)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=('freeze','run','launch','status'))
    mode = parser.parse_args().mode
    if mode == 'launch':
        launch()
    elif mode == 'freeze':
        freeze()
    elif mode == 'run':
        run()
    else:
        print(json.dumps({'runner': load(WORK/'runner369.json'),
            'results': len(list(DATA.glob('*/*/*.result.json'))),
            'certificates': len(list(DATA.glob('*/*/*.certificate.json'))),
            'complete': (DATA/'run_complete.json').exists(),
            'stderr_bytes': (WORK/'runner369.stderr').stat().st_size}))
