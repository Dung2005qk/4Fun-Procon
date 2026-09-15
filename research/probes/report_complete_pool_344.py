"""Canonical 344 reporting adapter; immutable measurement and comparator retained."""
import argparse
import copy
from pathlib import Path
from unittest.mock import patch
import complete_pool_score_344 as frozen

ROOT, ID = frozen.ROOT, frozen.ID
CONTRACT = ROOT / f'research/holdouts/{ID}-reporting-contract.json'


def normalize_transport(transport, case):
    t = copy.deepcopy(transport)
    setup = case['setup']; m = setup['map']; cells = m['cells']
    frozen.require(len(cells) == m['height'] and all(len(row) == m['width'] for row in cells), 'map shape')
    roadless = all(cell != 1 for row in cells for cell in row)
    if 'setup' in t:
        frozen.require(t['setup'] == setup and t['seed'] == frozen.seed(case), 'narrow transport identity')
        frozen.require(roadless and len(setup['agents']) == 3, 'narrow transport domain')
        roles = [0, 0, 0]
        frozen.require('roles' not in t or t['roles'] == roles, 'fixed role mismatch')
        t['roles'] = roles
        frozen.require(all(a['kind'] == 0 for day in t['actions']
                           for a in day['state']['agents'] + day['validated']['agents']), 'narrow role evidence')
    else:
        frozen.require(t['case'] == case and len(t['roles']) == len(setup['agents']), 'protected transport identity')
    n = m['width'] * m['height']
    for day in t['actions']:
        checked = day['validated']
        if 'road_footprint' not in checked:
            frozen.require('setup' in t and roadless, 'missing footprint without exact roadless proof')
            checked['road_footprint'] = [0] * n
        footprint = checked['road_footprint']
        frozen.require(len(footprint) == n and all(type(v) is int and v >= 0 for v in footprint), 'footprint shape/type')
        frozen.require(all(v == 0 for i, v in enumerate(footprint) if cells[i // m['width']][i % m['width']] != 1),
                       'traffic on non-road cell')
    return t


def report(phase, check=False):
    contract = frozen.load(CONTRACT); frozen.verify(contract)
    frozen.require(contract['execution_sha256'] == frozen.digest(frozen.M), 'report execution mismatch')
    m = frozen.load(frozen.M); frozen.verify(m)
    cases = frozen.load(ROOT / m['splits'][phase]['path'])['cases']
    directory = ROOT / f'research/evidence/{ID}-{phase}'
    frozen.require((directory/'run_complete.json').exists(), 'no partial reports')
    frozen.audit(cases, directory, phase)
    paths = {frozen.location(c, label, directory).with_suffix('.transport.json'):c for c in cases for label in frozen.LABELS}
    original_load, original_write = frozen.load, frozen.write_new
    normalized = {}
    for path, c in paths.items():
        t = normalize_transport(original_load(path), c)
        if phase != 'protected':
            replay = path.with_name(path.name.replace('.transport.json', '.replay.jsonl'))
            days = frozen.decisions(replay)
            frozen.require(len(days) == len(t['actions']), 'roadless replay day count')
            for day, action in zip(days, t['actions'], strict=True):
                frozen.require(day['candidate']['simulation']['roadFootprint'] == action['validated']['road_footprint'],
                               'roadless replay footprint mismatch')
        normalized[path] = t
    def read(p):
        p = Path(p)
        return copy.deepcopy(normalized[p]) if p in normalized else original_load(p)
    def write(p, value):
        value['reporting_correction'] = {'contract_sha256':frozen.digest(CONTRACT),
            'adapter_sha256':frozen.digest(Path(__file__)), 'method':'Exact zero footprint on proven roadless narrow maps; protected footprints retained.',
            'normalized_transports':len(normalized), 'measurement_rerun':False, 'score_gate_changed':False}
        value['operational_amendment'] = m['operational_amendment']
        if check:
            frozen.require(original_load(p) == value, 'report recomputation mismatch')
        else:
            original_write(p, value)
    with patch.object(frozen, 'load', side_effect=read), patch.object(frozen, 'write_new', side_effect=write):
        frozen.summarize(phase)


if __name__ == '__main__':
    parser = argparse.ArgumentParser()
    parser.add_argument('--phase', choices=('development','holdout','protected'), required=True)
    parser.add_argument('--check', action='store_true')
    a = parser.parse_args(); report(a.phase, a.check)
