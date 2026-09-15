"""Frozen-byte takeover-history lifecycle contract; never a score experiment."""
import argparse
import copy
import hashlib
import json
import os
from pathlib import Path
import queue
import shutil
import subprocess
import sys
import threading
import time

ROOT = Path(__file__).resolve().parents[2]
WORK = Path('/home/LMC/udon364-0909')
BASE = Path('/home/LMC/udon360-0909')
PACKAGE = Path('/home/LMC/udon357-0909')
OLD_DATA = Path('/home/LMC/udon362-0909/research/evidence/SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362-development')
ID = 'CONTRACT-CERTIFIED-TAKEOVER-RESUME-364'
BINARY_HASH = 'D3684DC3F4ADB5E6E74F5E75412CCB76E681D21F760B6BC0FA61EDC25A029678'
COMPLETION_HASH = '24A5574453D6FD284CABABFBD764479AD65A68AED06E1C298923193DE7CD4791'


def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest().upper()
def load(p): return json.loads(p.read_text())
def write(p, obj):
    with p.open('x', encoding='utf-8', newline='\n') as f:
        json.dump(obj, f, indent=2, sort_keys=True)
        f.write('\n')


def prefix_bytes(path, count):
    selected = []
    for line in path.read_bytes().splitlines(keepends=True):
        e = json.loads(line)
        if e['kind'] == 'day_state' and e['body']['day'] == count: break
        selected.append(line)
    prefix = b''.join(selected)
    events = [json.loads(line) for line in prefix.splitlines()]
    assert sum(e['kind'] == 'day_state' for e in events) == count
    assert sum(e['kind'] == 'action_result' for e in events) == count
    frames = [e['body'] for e in events if e['kind'] == 'resource_marginal']
    assert any(x['day'] == count and x['takeover'] and not x['failure'] for x in frames)
    return prefix


def stage():
    local_data = ROOT / 'artifacts/research/362/completed/research/evidence/SCORE-LATE-CONTROL-RESOURCE-MARGINAL-362-development'
    assert sha(local_data/'run_complete.json') == COMPLETION_HASH
    assert sha(ROOT/'artifacts/research/360/completed/btc360-plain') == BINARY_HASH
    complete = load(local_data/'run_complete.json')
    sessions = []
    for seed, count, window, role in ((202609093620092, 3, 10000, 'native'),
                                      (202609093620018, 2, 15000, 'fixed-all-Patrol')):
        names = {suffix: f'A/candidate/{seed}.{suffix}' for suffix in ('transport.json', 'replay.jsonl', 'certificate.json')}
        hashes = {}
        for name in names.values():
            assert sha(local_data/name) == complete['files'][name]
            hashes[name] = complete['files'][name]
        transport = load(local_data/names['transport.json'])
        assert transport['case']['days'] == count + 2 and transport['case']['role'] == role
        assert transport['case']['window_ms'] == window
        prefix = prefix_bytes(local_data/names['replay.jsonl'], count)
        sessions.append({'seed': seed, 'restored_days': count, 'window_ms': window, 'role': role,
            'source_hashes': hashes, 'prefix_sha256': hashlib.sha256(prefix).hexdigest().upper(),
            'prefix_bytes': len(prefix)})
    folder = ROOT/'artifacts/research/364/stage'
    folder.mkdir(parents=True, exist_ok=False)
    shutil.copyfile(Path(__file__), folder/'takeover_resume_364.py')
    shutil.copyfile(ROOT/'research/evidence'/f'{ID}-preregistration.md', folder/'preregistration.md')
    write(folder/'input364.json', {'experiment': ID, 'sessions': sessions, 'binary_sha256': BINARY_HASH,
        'completion362_sha256': COMPLETION_HASH, 'controls': ['0', '1'], 'expected_new_acks': 4,
        'expected_expired_waits': 4, 'expected_restored_days': 10, 'expected_full_days': 18,
        'expected_transitions': 14, 'no_score_authority': True})
    write(folder/'stage364.json', {'hashes': {p.name: sha(p) for p in sorted(folder.iterdir()) if p.is_file()}})
    print('stage364', sha(folder/'stage364.json'))


def verify():
    assert subprocess.check_output(['hostname'], text=True).strip() == 'udon-f0-240-0829'
    for name, h in load(WORK/'stage364.json')['hashes'].items(): assert sha(WORK/name) == h, name
    assert sha(BASE/'btc360-plain') == BINARY_HASH
    assert sha(OLD_DATA/'run_complete.json') == COMPLETION_HASH
    assert sha(BASE/'execution360.json') == '47488178393C4578A1D28795CA2EFDC9329F0B6BE96A75967774446F28FC68F9'
    for name, h in load(BASE/'execution360.json')['hashes'].items(): assert sha(BASE/name) == h, name
    sys.path.insert(0, str(BASE))
    import late_control_360
    late_control_360.verify_package(PACKAGE)
    m = load(WORK/'input364.json')
    for session in m['sessions']:
        for name, h in session['source_hashes'].items(): assert sha(OLD_DATA/name) == h, name
    return m


def server_class(rpc):
    class RestoredTakeover(rpc.ProtectedCase):
        def __init__(self, evidence, bridge, count):
            fixture = evidence['case']
            super().__init__(fixture, bridge)
            if self.roles is None: self.assign(evidence['roles'])
            assert self.roles == evidence['roles']
            self.restored = []
            for action in evidence['actions'][:count]:
                assert action['wire_day'] == self.day and action['state']['agents'] == self.agents
                assert action['state']['traffics'] == rpc.traffic_for(self.setup, self.day, self.own, fixture['external_road_footprints'])
                checked = bridge.request({'op': 'step', 'setup': self.setup,
                    'state': dict(action['state'], day=self.day+1, endsAt=action['state']['endsAt']//1000),
                    'ledger': self.ledger, 'plan': action['plan']})
                assert checked == action['validated'] and checked.get('ok') and checked.get('agrees')
                self.agents, self.ledger, self.score = checked['agents'], checked['ledger'], checked['score']
                self.own.append(checked['road_footprint'])
                self.restored.append(checked)
                self.day += 1
            self.expire_day, self.expired = count, None

        def get(self, endpoint):
            if endpoint == 'state' and self.day == self.expire_day and self.deadline is None:
                status, body = super().get(endpoint)
                assert status == 200
                self.ends_at = int(time.time()*1000)-2000
                self.deadline = time.monotonic()-2
                self.last_state['endsAt'] = self.ends_at
                plan = [[-self.setup['daySteps'][self.day]] for _ in self.agents]
                checked = self.bridge.request({'op': 'step', 'setup': self.setup,
                    'state': dict(self.last_state, day=self.day+1, endsAt=self.ends_at//1000),
                    'ledger': self.ledger, 'plan': plan})
                assert checked.get('ok') and checked.get('agrees')
                self.pending, self.plan = checked, plan
                self.expired = {'state': copy.deepcopy(self.last_state), 'plan': plan, 'validated': checked}
                return status, self.last_state
            return super().get(endpoint)

        def post(self, endpoint, body):
            assert endpoint != 'assignment', 'reposted frozen assignment'
            assert self.day == self.expire_day+1, 'posted accepted or expired day'
            assert self.pending is None, 'duplicate accepted terminal POST'
            return super().post(endpoint, body)
    return RestoredTakeover


def run_session(session, control, rpc, bridge):
    seed, count = session['seed'], session['restored_days']
    folder = WORK/'sessions'/f'{seed}-{control}'
    folder.mkdir(parents=True, exist_ok=False)
    evidence = load(OLD_DATA/f'A/candidate/{seed}.transport.json')
    state = server_class(rpc)(evidence, bridge, count)
    before = prefix_bytes(OLD_DATA/f'A/candidate/{seed}.replay.jsonl', count)
    assert len(before) == session['prefix_bytes']
    assert hashlib.sha256(before).hexdigest().upper() == session['prefix_sha256']
    replay = folder/'replay.jsonl'
    with replay.open('xb') as f: f.write(before)
    env = os.environ.copy()
    env['HEXUDON_TOKEN'] = 'synthetic-loopback-only-no-credential'
    env['UDON_RESOURCE_MARGINAL_357'] = control
    binary, match = BASE/'btc360-plain', f'm-364-{seed}-{control}'
    cmd = [str(binary), 'http', '--url', 'http://127.0.0.1:352', '--match', match,
           '--response-ms', '5000', '--poll-ms', '220', '--replay', str(replay)]
    output = queue.Queue()
    started, requests, error, last_id = time.monotonic(), [], None, 0
    with (folder/'stdout').open('x') as out, (folder/'stderr').open('x') as err:
        p = subprocess.Popen(cmd, stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=err, text=True, env=env)
        def read():
            for line in p.stdout: output.put(line)
            output.put(None)
        thread = threading.Thread(target=read, daemon=True)
        thread.start()
        try:
            while True:
                assert time.monotonic()-started < 120, 'contract wall limit'
                line = output.get(timeout=30)
                if line is None: break
                q = json.loads(line)
                if q.get('transportRPC') != 352:
                    assert q.get('synthetic') is True and state.day == count+2
                    out.write(line); out.flush(); continue
                assert q['id'] == last_id+1
                last_id = q['id']
                response = rpc.dispatch(state, q, match)
                requests.append({'query': q, 'response': response, 'wire_day': state.day})
                p.stdin.write(json.dumps(response, separators=(',', ':'))+'\n'); p.stdin.flush()
            p.wait(timeout=10)
        except Exception as exc: error = repr(exc)
        finally:
            p.stdin.close(); thread.join(timeout=1)
    write(folder/'transport.json', {'restored': state.restored, 'expired': state.expired, 'new_actions': state.actions,
        'requests': requests, 'own': state.own, 'error': error, 'exit_code': p.poll(), 'day': state.day,
        'assignment_posts': state.assignment_posts, 'prefix_sha256': session['prefix_sha256'], 'prefix_bytes': len(before)})
    assert error is None and p.poll() == 0, error
    p.stdout.close()
    assert state.day == count+2 and len(state.actions) == 1 and state.actions[0]['wire_day'] == count+1
    assert not state.assignment_posts and len(state.own) == count+2 and (folder/'stderr').stat().st_size == 0
    assert replay.read_bytes().startswith(before)
    appended = [json.loads(line) for line in replay.read_bytes()[len(before):].splitlines()]
    for kind in ('action_result', 'actions_deadline_skip', 'actions_server_wait'):
        assert sum(e['kind'] == kind for e in appended) == 1, kind
    assert not any(e['kind'] in ('resource_marginal', 'actions_fallback', 'actions_recovery_wait', 'virtual_parent_dropped') for e in appended)
    for e in appended:
        if e['kind'] == 'decision':
            d = e['body']['decision']
            assert not d['emergency'] and d['candidate']['simulation']['valid'] and d['timing']['totalMs'] <= 5000
        if e['kind'] == 'protected_slack': assert not any(v for k, v in e['body'].items() if k.endswith('Failure'))
    checked = subprocess.run([str(binary), 'replay-check', '--replay', str(replay)], capture_output=True, text=True, env=env, timeout=60)
    with (folder/'replay-check.txt').open('x') as f: f.write(checked.stdout+checked.stderr)
    assert checked.returncode == 0 and not checked.stderr
    assert f'summary days={count+2} reconciled_transitions={count+1}' in checked.stdout
    assert f'resume accepted_days={count+2} last_wire_day={count+1}' in checked.stdout
    write(folder/'case_complete.json', {'seed': seed, 'control': control, 'new_acks': 1, 'expired_waits': 1,
        'restored_days': count, 'full_days': count+2, 'transitions': count+1,
        'files': {p.name: sha(p) for p in sorted(folder.iterdir()) if p.is_file()}})
    print('contract_complete', seed, control, flush=True)


def summary():
    m = load(WORK/'input364.json'); rows = []
    for s in m['sessions']:
        for control in m['controls']:
            folder = WORK/'sessions'/f"{s['seed']}-{control}"
            row = load(folder/'case_complete.json')
            for name, h in row['files'].items(): assert sha(folder/name) == h, name
            rows.append(row)
    result = {'experiment': ID, 'complete': True, 'sessions': 4, 'rows': rows,
        'stage_sha256': sha(WORK/'stage364.json'), 'authority': 'Synthetic takeover-history recovery only, not score or BTC promotion'}
    for key in ('new_acks', 'expired_waits', 'restored_days', 'full_days', 'transitions'):
        result[key] = sum(r[key] for r in rows)
        assert result[key] == m['expected_'+key], key
    return result


def run():
    m = verify()
    assert not (WORK/'sessions').exists(), 'no duplicate or ambiguous session'
    sys.path.insert(0, str(PACKAGE/'research/probes'))
    import run_resource_357 as frozen
    with frozen.rpc.BridgeContext() as bridge:
        for s in m['sessions']:
            for control in m['controls']: run_session(s, control, frozen.rpc, bridge)
    verify(); write(WORK/'complete364.json', summary())
    print('complete364', sha(WORK/'complete364.json'), flush=True)


if __name__ == '__main__':
    p = argparse.ArgumentParser(); p.add_argument('mode', choices=('stage', 'run', 'check'))
    mode = p.parse_args().mode
    if mode == 'stage': stage()
    elif mode == 'run': run()
    else:
        verify(); assert load(WORK/'complete364.json') == summary(); print('verified364', sha(WORK/'complete364.json'))
