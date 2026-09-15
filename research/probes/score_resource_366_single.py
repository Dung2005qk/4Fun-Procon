"""One-pass measurement adapter; inherited full runtime and validators unchanged.

Local import/tests do not load a sealed setup or execute a solver. VM-only commands
verify the immutable package and original366 dependencies before reading setups.
"""
import argparse
import hashlib
import json
from pathlib import Path
import sys

WORK = Path('/home/LMC/udon366-0909')
PACKAGE = Path(__file__).resolve().parent
EXEC = PACKAGE/'execution366-single.json'
ID = 'SCORE-CAUSAL-RESOURCE-QUALIFICATION-366'
DATA = WORK/'research/evidence'/(ID+'-protected-single')
SUMMARY = WORK/'research/evidence'/(ID+'-protected-single.summary.json')
SELECTED = ('parentA','candidateA')
ALL_LABELS = {'parentA','candidateA','parentB','candidateB'}
SPLIT_HASH = '02E4C55AD2C8C506239E0B947BB5E2DA955D329C8BADC2B5979135F93A52DEB7'
ORIGINAL_EXEC_HASH = '94CF0B0FC04DE3277BD0584C2E54898D510F0B076923E8EECC9FBB7F02D648AC'
SUFFIXES = ('.result.json','.replay.jsonl','.transport.json','.replay-check.txt','.stdout','.stderr','.certificate.json')


def require(value, message):
    if not value:
        raise RuntimeError(message)


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest().upper()


def load(path):
    return json.loads(path.read_text(encoding='utf-8'))


def write(path, obj):
    path.parent.mkdir(parents=True,exist_ok=True)
    with path.open('x',encoding='utf-8',newline='\n') as out:
        json.dump(obj,out,indent=2,sort_keys=True)
        out.write('\n')


def selected_order(case):
    order = case['run_order']
    require(len(order) == 4 and set(order) == ALL_LABELS,'invalid frozen four-side order')
    return [label for label in order if label in SELECTED]


def prefix(case,label,directory):
    require(label in SELECTED,'B repetition forbidden')
    return directory/'A'/label[:-1]/str(case['seed'])


def require_previous(summary,completion):
    require(summary['phase'] == 'holdout' and summary['complete'] is True,'complete holdout required')
    require(summary['gate']['passed'] is True,'original holdout gate failed')
    require(completion['fixtures'] == 54 and completion['results'] == 216
            and completion['actions'] == 1368,'holdout completion counts')


def verify_certificate(certificate, side, case):
    require(certificate['independently_checked'] is True,'certificate checker missing')
    frames = certificate['frames']
    checked = certificate['certificates']
    require(len({f['day'] for f in frames}) == len(frames),'duplicate resource frame')
    require(side == 'candidate' or not frames,'parent resource activation')
    require({x['day'] for x in checked} == {x['day'] for x in frames if x['entered']}
            and len({x['day'] for x in checked}) == len(checked),'entered certificate coverage')
    for frame in frames:
        require(1 <= frame['day'] < case['days'] and case['setup']['daySeconds'][frame['day']-1] > 5,
                'illegal resource window/day')
        require(not frame['failure'],'resource failure')
        if frame['takeover']:
            require(frame['entered'] and not frame['deadline'] and frame['certified'] > 0
                    and frame['outputScore'] > frame['parentScore'],'unsafe takeover')
        else:
            require(frame['outputScore'] == frame['parentScore'] and frame['outputPlan'] == frame['parentPlan'],
                    'no-takeover mutation')
    by_day = {x['day']:x for x in frames}
    for row in checked:
        frame = by_day[row['day']]
        before,after = row['before'],row['after']
        require(row['work'] == frame and row['takeover'] == frame['takeover'],'certificate frame ownership')
        require(before['ok'] and before['agrees'] and after['ok'] and after['agrees'],'independent invalidity')
        require(before['score'] == frame['parentScore'] and after['score'] == frame['outputScore'],'certificate score ownership')
        require(before['road_footprint'] == after['road_footprint'],'road certificate')
        require(len(before['agents']) == len(after['agents']) and all(
            a['kind'] == b['kind'] and a['pos'] == b['pos'] and a['fuel'] <= b['fuel']
            for a,b in zip(before['agents'],after['agents'])),'agent certificate')
        require(set(before['ledger']['brands']) <= set(after['ledger']['brands']) and
                all(a <= b for a,b in zip(before['score'],after['score'])),'ledger certificate')


def fixture_marker(case,directory):
    return {'seed':case['seed'],'selected_order':selected_order(case),
        'results':{label:sha(prefix(case,label,directory).with_suffix('.result.json')) for label in SELECTED},
        'certificates':{label:sha(prefix(case,label,directory).with_suffix('.certificate.json')) for label in SELECTED}}


def audit(cases,directory,validate):
    require(len({c['seed'] for c in cases}) == len(cases),'duplicate fixture')
    expected = {str(c['seed']) for c in cases}
    allowed_top = {'A','run_complete.json'} | {seed+'.fixture_complete.json' for seed in expected}
    require(all(p.name in allowed_top for p in directory.iterdir()),'foreign repetition or fixture evidence')
    require({p.name for p in (directory/'A').iterdir()} == {'parent','candidate'},'side directories')
    for side in ('parent','candidate'):
        folder = directory/'A'/side
        allowed_files = {seed+suffix for seed in expected for suffix in SUFFIXES}
        require(all(p.is_file() and not p.is_symlink() and p.name in allowed_files for p in folder.iterdir()),
                'foreign side evidence')
        for case in cases:
            p = prefix(case,side+'A',directory)
            files = list(folder.glob(p.name+'.*'))
            if p.with_suffix('.result.json').exists():
                require(p.with_suffix('.certificate.json').exists(),'result without atomic certificate; no replay')
                validate(case,side,directory/'A')
                verify_certificate(load(p.with_suffix('.certificate.json')),side,case)
            else:
                require(not files,'ambiguous partial side; never duplicate accepted days')
    for case in cases:
        marker = directory/(str(case['seed'])+'.fixture_complete.json')
        if marker.exists():
            require(load(marker) == fixture_marker(case,directory),'fixture marker drift')


def execute(cases,directory,binary,bridge,engine):
    """Same original side lifecycle/certificate calls, filtering only repetition B."""
    for index,case in enumerate(cases):
        for label in selected_order(case):
            p = prefix(case,label,directory)
            if p.with_suffix('.result.json').exists():
                continue  # audited before this function; never replay accepted sides
            side = label[:-1]
            engine.configure_side(side)
            engine.rpc.run_one(case,binary,p,bridge,side)
            cert = engine.certificates(case,p,bridge)
            verify_certificate(cert,side,case)
            write(p.with_suffix('.certificate.json'),cert)
            print('side_complete fixture='+str(index+1)+' label='+label,flush=True)
        mark = directory/(str(case['seed'])+'.fixture_complete.json')
        if not mark.exists():
            write(mark,fixture_marker(case,directory))
        print('fixture_complete '+str(index+1),flush=True)


def verify_package():
    manifest = load(EXEC)
    require(manifest['original_execution_sha256'] == ORIGINAL_EXEC_HASH,'upstream execution identity')
    for name,h in manifest['hashes'].items():
        require(Path(name).name == name and sha(PACKAGE/name) == h,'adapter package drift:'+name)
    require(sha(WORK/'execution366.json') == ORIGINAL_EXEC_HASH,'original366 execution drift')
    sys.path.insert(0,str(WORK))
    import score_resource_366 as original
    original.verify()  # includes exact binary, complete inherited source/contract hashes, host
    return original


def inputs(original):
    meta = original.load(original.INPUT)['splits']['protected']
    require(meta['sha256'] == SPLIT_HASH and sha(WORK/meta['path']) == SPLIT_HASH,'protected split drift')
    cases = load(WORK/meta['path'])['cases']
    require(len(cases) == len({c['seed'] for c in cases}) == 108,'protected108 identity')
    require(sum(c['days'] for c in cases)*2 == 1368,'protected ACK count')
    require(sum(sum(c['setup']['daySeconds']) for c in cases)*2 == 13680,'public window count')
    for case in cases:
        selected_order(case)
    return cases


def previous_gate(original,check=False):
    summary = load(WORK/'research/evidence'/(ID+'-holdout.summary.json'))
    complete_path = WORK/'research/evidence'/(ID+'-holdout')/'run_complete.json'
    complete = load(complete_path)
    require_previous(summary,complete)
    require(summary['completion_sha256'] == sha(complete_path) and
            summary['execution_sha256'] == complete['execution_sha256'] == ORIGINAL_EXEC_HASH,'holdout identity')
    if check:
        original.summarize('holdout',True)
    return sha(WORK/'research/evidence'/(ID+'-holdout.summary.json'))


def summarize(check=False):
    original = verify_package()
    prior_hash = previous_gate(original)
    cases = inputs(original)
    complete = load(DATA/'run_complete.json')
    require(complete['execution_sha256'] == sha(EXEC) and complete['previous_summary_sha256'] == prior_hash,
            'single-pass completion provenance')
    expected = {'fixtures':108,'results':216,'certificates':216,'actions':1368,'transitions':1152}
    require(all(complete[k] == v for k,v in expected.items()),'single-pass completion counts')
    actual = {p.relative_to(DATA).as_posix():sha(p) for p in DATA.rglob('*') if p.is_file() and p.name != 'run_complete.json'}
    require(complete['files'] == actual,'complete evidence file-set/hash drift')
    audit(cases,DATA,original.f.validate_side)
    require(len(list(DATA.glob('*.fixture_complete.json'))) == 108,'missing fixture markers')
    results = list(DATA.glob('A/*/*.result.json'))
    require(len(results) == len(list(DATA.glob('A/*/*.certificate.json'))) == 216,'atomic result count')
    require(sum(load(p)['actions'] for p in results) == 1368 and
            sum(load(p)['transitions'] for p in results) == 1152,'actual lifecycle counts')
    from gate_resource_366_single import classify
    rows = []
    for case in cases:
        paths = {s:prefix(case,s+'A',DATA) for s in ('parent','candidate')}
        ts = {s:load(p.with_suffix('.transport.json')) for s,p in paths.items()}
        comp = original.f.compare_runs(ts['parent'],ts['candidate'])
        cert = load(paths['candidate'].with_suffix('.certificate.json'))
        require(cert['frames'] == original.f.mechanism_safety(paths['candidate'].with_suffix('.replay.jsonl')),
                'raw resource frame drift')
        first = next((x['day'] for x in cert['frames'] if x['takeover']),None)
        rows.append({**{k:case[k] for k in (*original.f.STRATA,'seed')},'repeat':'A',**comp,
            'first_takeover':first,'causal':first is not None and comp['first_divergence'] is not None
                and first <= comp['first_divergence'],'certificate':cert,'safety':True,
            'selected_order':selected_order(case)})
    report = {'experiment':ID,'phase':'protected-single','complete':True,**expected,
        'execution_sha256':sha(EXEC),'completion_sha256':sha(DATA/'run_complete.json'),
        'previous_summary_sha256':prior_hash,'gate':classify(cases,rows),'summary':original.f.aggregate(rows),
        'active_only_summary':original.f.aggregate([r for r in rows if r['window_ms'] > 5000]),
        'strata':{k:{str(v):original.f.aggregate([r for r in rows if r[k] == v])
            for v in sorted({r[k] for r in rows})} for k in original.f.STRATA},'rows':rows,
        'same_binary_controls':None,'robust_intervals':None,'zero_safety_failure':True,
        'independent5000_trajectory_mismatches':sum(not r['trajectory_equal'] for r in rows if r['window_ms'] == 5000),
        'independent5000_score_mismatches':sum(r['A'] != r['B'] for r in rows if r['window_ms'] == 5000),
        'authority':'User-amended single-pass protected; no A/A or replicated intervals; not automatic product promotion'}
    report = json.loads(json.dumps(report))
    if check:
        require(load(SUMMARY) == report,'summary drift')
    else:
        write(SUMMARY,report)
    print('single_summary_complete '+sha(SUMMARY)+' '+json.dumps(report['gate']),flush=True)


def run(resume=False):
    original = verify_package()
    prior_hash = previous_gate(original,check=True)
    cases = inputs(original)
    if DATA.exists():
        require(resume and not (DATA/'run_complete.json').exists(),'existing run; no duplicate')
    else:
        for side in ('parent','candidate'):
            (DATA/'A'/side).mkdir(parents=True)
    audit(cases,DATA,original.f.validate_side)
    with original.f.rpc.BridgeContext() as bridge:
        execute(cases,DATA,Path(original.load(original.EXEC)['candidate_binary']),bridge,original.f)
    audit(cases,DATA,original.f.validate_side)
    verify_package()
    write(DATA/'run_complete.json',{'fixtures':108,'results':216,'certificates':216,'actions':1368,'transitions':1152,
        'execution_sha256':sha(EXEC),'previous_summary_sha256':prior_hash,
        'files':{p.relative_to(DATA).as_posix():sha(p) for p in DATA.rglob('*') if p.is_file()}})
    print('run_complete protected-single',flush=True)
    summarize()


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('mode',choices=('run','summarize'))
    parser.add_argument('--resume',action='store_true')
    parser.add_argument('--check',action='store_true')
    args = parser.parse_args()
    run(args.resume) if args.mode == 'run' else summarize(args.check)
