import copy
import unittest
from http_prefix_option_loss_321 import ROOT,load,digest,require,write_new,close_bridge
from run_http_baseline_314 import Bridge
from test_http_prefix_oracle_321 import fixture
import test_complete_pool_344 as inherited
import test_report_complete_pool_344 as reporting
import claim_cap_score_347 as runner

PROBE=ROOT/'artifacts/research/347/build/udonshield_claim_cap_347.exe'
ID='SCORE-PATROL-CLAIM-CAP-COMPLETE-POOL-347'

class PricingContract(inherited.PricingContract):
    def setUp(self):
        super().setUp();close_bridge(self.p)
        self.p=Bridge(ROOT/'artifacts/research/347/build/udonshield_pricing_contract_344.exe')

class RunnerContract(inherited.RunnerContract):
    def setUp(self):
        self.old=(runner.core.ID,runner.core.M,runner.core.summarize);runner.configure()
    def tearDown(self):runner.core.ID,runner.core.M,runner.core.summarize=self.old

class ReportingContract(reporting.ReportingContract):
    pass

class BoundContract(unittest.TestCase):
    def setUp(self):self.p=Bridge(PROBE)
    def tearDown(self):close_bridge(self.p)
    def test_fuel_zero_stays_at_spot_and_one_spot_limit(self):
        q=fixture();q['state']['day']=3
        q['state']['agents']=[{'kind':0,'pos':p,'fuel':0} for p in (17,18,7)]
        q['ledger']={'brands':[10],'totalDailyDistinct':3,'totalServings':5}
        before=copy.deepcopy(q);r=self.p.request(q)
        self.assertTrue(r['ok'],r);self.assertEqual(r['upper'],[2,7,9]);self.assertEqual(q,before)
        q['setup']['spots']=q['setup']['spots'][:1]
        for a in q['state']['agents']:a['fuel']=3
        # Two reachable Patrols, one spot, two days: shared stock2 caps at4.
        r=self.p.request(q);self.assertEqual(r['upper'],[1,5,9])
    def test_tanker_path_and_expired_coarse_bound_remain(self):
        q=fixture();q['state']['day']=4
        q['state']['agents']=[{'kind':0,'pos':p,'fuel':0} for p in (17,18,7)]
        r=self.p.request(q);self.assertEqual(r['upper'],[2,2,2])
        q['state']['agents'][2]['kind']=1
        r=self.p.request(q);self.assertEqual(r['upper'],[2,2,4])
        q['expired']=True;r=self.p.request(q)
        self.assertTrue(r['expired']);self.assertEqual(r['upper'],[2,2,4])
    def test_full_consumed_exact_reference_suite(self):
        rows=reference_check(self.p)
        self.assertEqual(len(rows),37)
        strict=[ref for row in rows for ref in row['references'] if ref['strict']]
        self.assertGreaterEqual(len({r['seed'] for r in strict}),2)
        self.assertGreaterEqual(len({r['family'] for r in strict}),2)

def reference_check(p):
    m=load(ROOT/'research/holdouts/ATTR-CURRENT-FLOOR-CONDITIONAL-CEILING-346.json')
    summary=ROOT/'research/evidence/ATTR-CURRENT-FLOOR-CONDITIONAL-CEILING-346.summary.json'
    require(digest(summary)=='17E8E851026BC0125C1BD17F4E6C0BC84532A1EF6BA8E10570F74D741D51B517','346 drift')
    byid={r['request_id']:r['conditional_optimum'] for r in load(summary)['rows']};rows=[]
    for c in m['cases']:
        q=c['request'];r=p.request(q);require(r['ok'] and not r['expired'],'bound process')
        require(byid[c['id']]<=r['upper'],'UNSOUND global upper')
        require(all(r['upper']<=ref['selected_upper'] for ref in c['references']),'unexpected wider bound')
        require(all(t['servings']==r['upper'][2] for t in r['tiers']),'conditional consumer drift')
        rows.append({'id':c['id'],'oracle':byid[c['id']],'upper':r['upper'],
            'references':[{'seed':ref['seed'],'family':ref['family'],'repeat':ref['repeat'],'side':ref['side'],
                'strict':ref['inherited_certificate']>r['upper']} for ref in c['references']]})
    return rows

if __name__=='__main__':unittest.main()
