"""Protected-only binding repair; frozen solver, inputs, runner and gates reused."""
import argparse
import copy
import subprocess
import sys
import claim_cap_score_347 as original

core = original.core
ROOT, ID = original.ROOT, original.ID
BASE = original.M
BASE_SHA = '8AADA9AC98F9095403AE6DA850E879319ACC86C01990AA62ADC46F9182F5F66A'
M = ROOT / f'research/holdouts/{ID}-protected-execution.json'
BRIDGE = 'artifacts/research/341/protected-bridge.exe'
BRIDGE_SHA = 'B8C26E696885FB3C9CC8F99D2F6194C0CFD62FF0229C8C9CA0D9DF862EA47EC6'
QUALIFIED = {
    'development': '908FD2EB391F7363929C984D8E051283EF6572CF182E0B660E4E08CAC76486E2',
    'holdout': 'D20ACD48ED071AC128B9EBD75351CAFF1426BB162E2374726F9F6B58F0468913',
}


def relative(path):
    return path.relative_to(ROOT).as_posix()


def base_qualification():
    core.require(core.digest(BASE) == BASE_SHA, 'original execution drift')
    base = core.load(BASE)
    core.verify(base)
    core.require(base['hashes'][BRIDGE] == BRIDGE_SHA, 'bridge not originally frozen')
    for phase, expected in QUALIFIED.items():
        path = ROOT / f'research/evidence/{ID}-{phase}.summary.json'
        core.require(core.digest(path) == expected, 'prior qualification drift')
        report = core.load(path)
        core.require(report['complete'] and report['gate']['passed'] and
                     report['execution_sha256'] == BASE_SHA, 'prior phase did not qualify')
    return base


def validate_binding(base, amended):
    mutable = {'bridge_binary', 'hashes', 'stage'}
    core.require(set(amended) == set(base) | {'operational_amendment'}, 'unexpected manifest keys')
    core.require(all(amended[k] == v for k, v in base.items() if k not in mutable),
                 'non-operational execution change')
    core.require(amended['bridge_binary'] == BRIDGE, 'wrong protected bridge')
    core.require(all(amended['hashes'].get(k) == v for k, v in base['hashes'].items()),
                 'original dependency removed or changed')
    core.require(amended['operational_amendment']['original_execution_sha256'] == BASE_SHA,
                 'original execution provenance')


def authorize(amended, phase):
    core.require(phase == 'protected', 'protected-only operational entry')
    base = base_qualification()
    validate_binding(base, amended)
    core.verify(amended)


def prepare():
    core.require(not M.exists(), 'protected execution already frozen')
    original.configure()
    base = base_qualification()
    directory = ROOT / f'research/evidence/{ID}-protected'
    cases = core.load(ROOT / base['splits']['protected']['path'])['cases']
    core.audit(cases, directory, 'protected')
    core.require(not list(directory.rglob('*.*')), 'pre-measurement amendment requires empty evidence')
    bridge = core.Bridge(ROOT / BRIDGE)
    old = core.Bridge(ROOT / base['bridge_binary'])
    configs = roles = transitions = 0
    try:
        failed = old.request({'op': 'config', 'setup': cases[0]['setup']})
        core.require(failed == {'ok': False, 'error': 'unknown bridge operation'}, 'wrong failure attribution')
        for case in cases:
            setup = case['setup']
            core.require(bridge.request({'op': 'config', 'setup': setup}).get('ok'), 'invalid setup')
            configs += 1
            for selection in ([0] * len(setup['agents']), [i % 2 for i in range(len(setup['agents']))]):
                core.require(bridge.request({'op': 'roles', 'setup': setup, 'roles': selection}).get('ok'), 'roles invalid')
                roles += 1
        dev = core.load(ROOT / base['splits']['development']['path'])['cases']
        for case in dev:
            for label in core.LABELS:
                prefix = core.location(case, label, ROOT / f'research/evidence/{ID}-development')
                transport = core.load(prefix.with_suffix('.transport.json'))
                ledger = {'brands': [], 'totalDailyDistinct': 0, 'totalServings': 0}
                for action in transport['actions']:
                    state = copy.deepcopy(action['state'])
                    state['day'] += 1
                    state['endsAt'] //= 1000
                    request = {'op': 'step', 'setup': case['setup'], 'state': state,
                               'ledger': ledger, 'plan': action['plan']}
                    a, b = old.request(request), bridge.request(request)
                    footprint = b.pop('road_footprint')
                    core.require(a == b and a.get('ok') and a.get('agrees'), 'exact transition parity')
                    core.require(a['agents'] == action['validated']['agents'] and
                                 a['ledger'] == action['validated']['ledger'] and
                                 a['score'] == action['validated']['score'], 'accepted transition drift')
                    core.require(footprint == [0] * 64, 'roadless footprint parity')
                    ledger = a['ledger']
                    transitions += 1
    finally:
        for instance in (old, bridge):
            instance.close()
            instance.process.stdout.close()
            instance.process.stderr.close()
    commands = [[sys.executable, '-m', 'unittest', 'discover', '-s', 'research/probes', '-p', pattern, '-v']
                for pattern in ('test_protected_http_341.py', 'test_claim_cap_protected_347.py')]
    records = []
    for command in commands:
        result = subprocess.run(command, cwd=ROOT, capture_output=True, text=True)
        records.append({'command': command, 'exit_code': result.returncode,
                        'stdout': result.stdout, 'stderr': result.stderr})
        core.require(result.returncode == 0, result.stdout + result.stderr)
    preflight = ROOT / f'research/evidence/{ID}-protected-bridge-preflight.json'
    core.write_new(preflight, {'passed': True, 'configs': configs, 'role_checks': roles,
                              'exact_accepted_development_transitions': transitions,
                              'protected_scored_sides': 0, 'records': records,
                              'original_error': failed, 'original_execution_sha256': BASE_SHA})
    amended = copy.deepcopy(base)
    amended['bridge_binary'] = BRIDGE
    amended['stage'] = 'Protected-only bridge binding repair before any protected measurement'
    note = ROOT / f'research/evidence/{ID}-protected-bridge-amendment.md'
    extras = [BASE, preflight, note, ROOT / 'research/probes/claim_cap_protected_347.py',
              ROOT / 'research/probes/test_claim_cap_protected_347.py',
              ROOT / f'research/evidence/{ID}-protected.runner.stderr',
              *[ROOT / f'research/evidence/{ID}-{phase}.summary.json' for phase in QUALIFIED]]
    amended['hashes'].update({relative(path): core.digest(path) for path in extras})
    amended['operational_amendment'] = {'original_execution_sha256': BASE_SHA,
        'note': relative(note), 'preflight': relative(preflight),
        'change': 'Bind existing originally-frozen protected bridge; no solver/input/metric/cap change.'}
    authorize(amended, 'protected')
    core.write_new(M, amended)
    print({'execution_sha256': core.digest(M), 'dependencies': len(amended['hashes']),
           'preflight_sha256': core.digest(preflight), 'configs': configs,
           'role_checks': roles, 'equivalent_transitions': transitions}, flush=True)


def configure():
    original.M = M
    original.configure()
    core.authorize = authorize


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument('mode', choices=('prepare', 'run', 'summarize'))
    parser.add_argument('--resume', action='store_true')
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    if args.mode == 'prepare':
        prepare()
        return
    configure()
    authorize(core.load(M), 'protected')
    if args.mode == 'run':
        core.run('protected', args.resume)
    else:
        original.summarize('protected', args.check)


if __name__ == '__main__':
    main()
