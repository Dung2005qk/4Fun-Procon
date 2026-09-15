import copy
import tempfile
from pathlib import Path
import unittest
from unittest.mock import patch
import inactive_runtime_aa_342 as r


def transport():
    checked={'agents':[{'fuel':2,'pos':1}], 'ledger':{'servings':1}, 'road_footprint':[0,1]}
    return {'roles':[0], 'score':[1,1,1], 'actions':[{'plan':[[2]], 'validated':checked}]}


class AAContract(unittest.TestCase):
    def test_same_binary_exact_identity(self):
        t=transport(); q=r.compare_runs(t,copy.deepcopy(t))
        self.assertTrue(q['trajectory_equal']);self.assertEqual(q['comparison_B_vs_A'],'tie');self.assertIsNone(q['first_divergence'])
    def test_role_difference_precedes_day(self):
        a=transport(); b=copy.deepcopy(a);b['roles']=[1]
        q=r.compare_runs(a,b);self.assertFalse(q['trajectory_equal']);self.assertEqual(q['first_divergence'],0)
    def test_state_ledger_road_difference_even_if_plan_ties(self):
        for key in ('agents','ledger','road_footprint'):
            a=transport();b=copy.deepcopy(a);b['actions'][0]['validated'][key]=[]
            q=r.compare_runs(a,b);self.assertFalse(q['trajectory_equal']);self.assertEqual(q['first_divergence'],1)
    def test_plan_difference_with_equal_score(self):
        a=transport();b=copy.deepcopy(a);b['actions'][0]['plan']=[[3]]
        q=r.compare_runs(a,b);self.assertEqual(q['comparison_B_vs_A'],'tie');self.assertFalse(q['plans_equal'])
    def test_lexicographic_loss_not_masked_by_servings(self):
        a=transport();a['score']=[4,8,20];b=copy.deepcopy(a);b['score']=[4,7,99]
        q=r.compare_runs(a,b);self.assertEqual(q['comparison_B_vs_A'],'loss');self.assertEqual(q['first_tier'],2)
    def test_order_counterbalances_positions(self):
        orders=[r.order_for(i) for i in range(8)]
        for pos in range(4):self.assertEqual(r.Counter(o[pos] for o in orders),{k:2 for k in r.LABELS})
    def test_partial_side_and_foreign_evidence_block_resume(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp)
            for rep,side in r.LABELS.values():(d/rep/side).mkdir(parents=True)
            c={'seed':34201,'run_order':r.order_for(0)}
            r.audit([c],d,validator=lambda *a:None)
            (d/'A'/'parent'/'34201.replay.jsonl').touch()
            with self.assertRaisesRegex(ValueError,'ambiguous'):r.audit([c],d,validator=lambda *a:None)
    def test_result_requires_validation_not_just_existence(self):
        with tempfile.TemporaryDirectory() as tmp:
            d=Path(tmp)
            for rep,side in r.LABELS.values():(d/rep/side).mkdir(parents=True)
            (d/'A'/'parent'/'34201.result.json').touch()
            c={'seed':34201,'run_order':r.order_for(0)}
            with self.assertRaisesRegex(ValueError,'hash'):
                r.audit([c],d,validator=lambda *a: r.require(False,'hash'))
    def test_supported_counter_blocks_negative_control(self):
        stats={k:0 for k in r.FIELDS};stats['supported']=1
        day={'profile':{'horizonPricing':stats},'audit':{'candidates':[]}}
        with patch.object(r,'validate_side',return_value={}),patch.object(r,'decisions',return_value=[day]):
            with self.assertRaisesRegex(ValueError,'supported'):r.validate_control({'seed':1},'candidateA',Path('.'))
    def test_parent_does_not_require_new_telemetry(self):
        with patch.object(r,'validate_side',return_value={'ok':True}),patch.object(r,'decisions') as d:
            self.assertEqual(r.validate_control({'seed':1},'parentA',Path('.')),{'ok':True});d.assert_not_called()


if __name__=='__main__':unittest.main()
