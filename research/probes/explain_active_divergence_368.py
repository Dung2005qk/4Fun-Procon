"""Explain completed368 observations without rerunning or tuning a solver."""
from collections import Counter
import json
from pathlib import Path

from audit_candidate_availability_363 import identity, semantic, record
from explain_execution_divergence_367 import singleton_rank_check, stripped_work
from verify_resource_366_protected_causal_copy import ROOT, DATA, load, sha, verify_files

ID = 'ATTR-ACTIVE-PROTECTED-DIVERGENCE-368'
INVENTORY_HASH = '9040D953220C58BD597B8EA89CF89D447219753C7764BEB48A9FEF89726F69D6'
PROOF_WORK_FIELDS = frozenset(('combinationsVisited', 'branchesPruned'))


def proof_semantics(proof):
    # These two diagnostic counters are not read by the production main solve.
    # Keep complete/infeasible, bounds, identity and any unknown field.
    return {k: v for k, v in proof.items() if k not in PROOF_WORK_FIELDS}


def checkpoint_comparison(left, right):
    assert left and right
    assert left[-1]['acceptedDay'] == right[-1]['acceptedDay']
    a, b = left[-1], right[-1]
    return {
        'serialized_record_counts': [len(left), len(right)],
        'first_checkpoint_equal': left[0] == right[0],
        'last_checkpoint_equal': a == b,
        'last_cached_contingencies_equal': a['cachedContingencies'] == b['cachedContingencies'],
        'last_proof_semantics_equal': list(map(proof_semantics, a['strongProofs'])) ==
                                     list(map(proof_semantics, b['strongProofs'])),
        'last_proofs': [a['strongProofs'], b['strongProofs']],
        'last_cache_hashes': [identity(a['cachedContingencies']), identity(b['cachedContingencies'])],
    }


def served_claims(setup, simulation):
    claims = [dict(c, brand=setup['spots'][c['spot']]['brand'],
                   pos=setup['spots'][c['spot']]['pos'])
              for c in simulation['claims'] if c['served']]
    brands = sorted({c['brand'] for c in claims})
    assert len(claims) == simulation['score']['servings']
    assert len(brands) == simulation['score']['dailyDistinct']
    assert sum(1 << b for b in brands) == int(simulation['score']['brandsMask'])
    return {'brands': brands, 'servings': len(claims), 'claims': claims}


def telescoping_delta(days):
    assert days
    total = [sum(d['daily_delta'][k] for d in days) for k in range(3)]
    final = [b - a for a, b in zip(days[-1]['sides']['parent']['score'],
                                    days[-1]['sides']['candidate']['score'], strict=True)]
    assert total == final, 'daily deltas must telescope to the final difference'
    return total


def run():
    verify_files()
    source = ROOT / 'research/evidence' / (ID + '.json')
    assert sha(source) == INVENTORY_HASH
    inventory = load(source)
    findings = []
    rank_checks = Counter()
    for loss in inventory['losses']:
        events, docs, transports = {}, {}, {}
        for side in ('parent', 'candidate'):
            p = DATA / 'A' / side / str(loss['seed'])
            events[side] = [json.loads(line) for line in p.with_suffix('.replay.jsonl').read_text().splitlines()]
            docs[side] = [e['body'] for e in events[side] if e['kind'] == 'decision']
            transports[side] = load(p.with_suffix('.transport.json'))
            for doc in docs[side]:
                check = singleton_rank_check(doc['decision'])
                rank_checks[check['status']] += 1
                assert check['status'] != 'strict-rank-inversion'
        item = {'seed': loss['seed'], 'final_delta': telescoping_delta(loss['days_detail']),
                'roles': {s: transports[s]['roles'] for s in transports},
                'first_main_difference': loss['first_main_plan_difference'],
                'first_resource_gain': loss['first_takeover'],
                'daily_deltas': [d['daily_delta'] for d in loss['days_detail']]}
        if not loss['roles_equal']:
            assert loss['first_divergence'] == 0
            item['boundary'] = 'pre-treatment-role-selection'
            item['causal_treatment_effect_identified'] = False
            findings.append(item)
            continue
        first = loss['first_main_plan_difference']
        assert first is not None
        a, b = (docs[s][first - 1] for s in ('parent', 'candidate'))
        item['boundary'] = 'equal-main-input-different-F0-W1-admission'
        item['first_main_inputs_equal'] = {
            'state': semantic(a['state']) == semantic(b['state']),
            'ledger': a['ledger'] == b['ledger'],
            'manifest': a['decision']['manifest'] == b['decision']['manifest'],
            'deadline': a['decision']['deadline'] == b['decision']['deadline'],
        }
        item['earlier_resource_prefix'] = [{k: d[k] for k in
            ('day', 'main_plan_equal', 'main_state_equal', 'main_ledger_equal',
             'same_day_pre_resource_equals_other_parent_plan', 'daily_delta')}
            for d in loss['days_detail'][:first-1]]
        item['post_ack_before_first_main_difference'] = []
        for day in range(1, first):
            checkpoints = [[e['body'] for e in events[s] if e['kind'] == 'session_checkpoint'
                            and e['body']['acceptedDay'] == day] for s in ('parent', 'candidate')]
            item['post_ack_before_first_main_difference'].append(
                dict(day=day, **checkpoint_comparison(*checkpoints)))
        item['selection'] = {}
        for side, document in (('parent', a), ('candidate', b)):
            selected_id = document['decision']['candidate']['stableId']
            appearances = {}
            for other, other_doc in (('parent', a), ('candidate', b)):
                pool = other_doc['decision']['audit']['candidates']
                found = [(i, r) for i, r in enumerate(pool) if r['stableId'] == selected_id]
                assert len(found) == 1
                index, candidate = found[0]
                appearances[other] = dict(record(candidate), audit_index=index, pool_size=len(pool))
            item['selection'][side] = appearances
        item['work_at_first_difference'] = {s: dict(stripped_work(d['decision']), timing=d['decision']['timing'])
                                            for s, d in (('parent', a), ('candidate', b))}
        item['daily_distinct_deficit_days'] = []
        for day in loss['days_detail']:
            if day['daily_delta'][1] >= 0:
                continue
            index = day['day'] - 1
            coverage = {}
            for side in ('parent', 'candidate'):
                doc = docs[side][index]
                setup = next(e['body'] for e in events[side] if e['kind'] == 'setup')
                action = transports[side]['actions'][index]
                assert doc['decision']['candidate']['plan'] == action['plan']
                coverage[side] = {
                    'main_served': served_claims(setup, doc['decision']['candidate']['simulation']),
                    'main_start_agents': doc['state']['agents'],
                    'actual_start_agents': action['state']['agents'],
                    'submitted_plan': action['plan'],
                }
            item['daily_distinct_deficit_days'].append({'day': day['day'], 'sides': coverage,
                'parent_only_brands': sorted(set(coverage['parent']['main_served']['brands']) -
                                            set(coverage['candidate']['main_served']['brands']))})
        item['causal_treatment_effect_identified'] = False
        item['unobserved'] = ['exact per-query historical clock/OS scheduling',
                              'full internal cache/allocator/CPU history at main entry',
                              'same-clock execution with treatment removed']
        findings.append(item)
    sources = ('src/decision.cpp', 'src/runtime.cpp', 'src/slack_refiner.cpp',
               'artifacts/research/357/completed/source/src/decision.cpp',
               'artifacts/research/357/completed/source/src/slack_refiner.cpp',
               'artifacts/research/360/completed/btc_main.cpp')
    assert (ROOT / sources[0]).read_text() == (ROOT / sources[3]).read_text()
    research_refiner = (ROOT / sources[4]).read_text()
    prefix, _ = research_refiner.split('ResourceMarginalResult ProtectedSlackRefiner::refine_resource_marginal(', 1)
    assert (ROOT / sources[2]).read_text() == prefix + '} // namespace udon\n'
    result = {'experiment': ID, 'inventory_sha256': INVENTORY_HASH,
              'script_sha256': sha(Path(__file__)), 'findings': findings,
              'singleton_rank_checks': dict(rank_checks),
              'source_hashes': {p: sha(ROOT / p) for p in sources},
              'solver_executed': False, 'gate_changed': False, 'promotion_authorized': False,
              'exact_historical_OS_cause_proven': False,
              'source_equivalence': 'normalized full decision; full accepted refiner prefix unchanged'}
    out = ROOT / 'research/evidence' / (ID + '-explanation.json')
    if out.exists():
        assert load(out) == result
    else:
        with out.open('x', encoding='utf-8', newline='\n') as f:
            json.dump(result, f, indent=2, sort_keys=True)
            f.write('\n')
    print('findings', [(r['seed'], r['boundary'], r['final_delta']) for r in findings])
    print('rank_checks', result['singleton_rank_checks'])
    print('explanation_sha256', sha(out))


if __name__ == '__main__':
    run()
