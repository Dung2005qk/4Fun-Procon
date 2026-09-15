import copy
import unittest
from report_complete_pool_344 import normalize_transport
from inactive_runtime_aa_342 import compare_runs


def fixture():
    c={'spec':{'seed':17},'setup':{'map':{'width':2,'height':2,'cells':[[0,0],[0,0]]},'agents':[0,1,2]}}
    agents=[{'kind':0,'pos':i,'fuel':3} for i in range(3)]
    t={'seed':17,'setup':copy.deepcopy(c['setup']),'score':[1,4,4],
       'actions':[{'plan':[[-1]]*3,'state':{'agents':agents},'validated':{'agents':agents,'ledger':{'totalServings':4}}}]}
    return c,t


class ReportingContract(unittest.TestCase):
    def test_narrow_proof_is_exact_and_input_immutable(self):
        c,t=fixture(); before=copy.deepcopy(t); n=normalize_transport(t,c)
        self.assertEqual(t,before);self.assertEqual(n['roles'],[0]*3)
        self.assertEqual(n['actions'][0]['validated']['road_footprint'],[0]*4)
        self.assertTrue(compare_runs(n,n)['trajectory_equal'])

    def test_roaded_missing_data_is_not_silently_zeroed(self):
        c,t=fixture();c['setup']['map']['cells'][0][0]=1;t['setup']=copy.deepcopy(c['setup'])
        with self.assertRaises(ValueError):normalize_transport(t,c)

    def test_identity_roles_and_bad_footprints_fail_closed(self):
        for field in ('identity','role','footprint','shape'):
            c,t=fixture()
            if field=='identity':t['seed']=18
            if field=='role':t['actions'][0]['state']['agents'][0]['kind']=1
            if field=='footprint':t['actions'][0]['validated']['road_footprint']=[1,0,0,0]
            if field=='shape':t['actions'][0]['validated']['road_footprint']=[0]
            with self.assertRaises(ValueError):normalize_transport(t,c)

    def test_protected_real_road_footprint_and_divergence_retained(self):
        c,t=fixture();c['setup']['map']['cells'][0][0]=1
        del t['setup'];t['case']=copy.deepcopy(c);t['roles']=[0]*3
        t['actions'][0]['validated']['road_footprint']=[2,0,0,0]
        n=normalize_transport(t,c);self.assertEqual(t,n)
        b=copy.deepcopy(n);b['actions'][0]['validated']['road_footprint'][0]=3
        comparison=compare_runs(n,b)
        self.assertFalse(comparison['transition_equal']['road_footprint'])
        self.assertEqual(comparison['first_divergence'],1)

    def test_score_and_state_differences_are_not_erased(self):
        c,t=fixture();a=normalize_transport(t,c);b=copy.deepcopy(a)
        b['score']=[1,3,20];b['actions'][0]['validated']['ledger']['totalServings']=20
        q=compare_runs(a,b)
        self.assertEqual(q['comparison_B_vs_A'],'loss');self.assertEqual(q['first_tier'],2)
        self.assertFalse(q['transition_equal']['ledger'])


if __name__=='__main__':unittest.main()
