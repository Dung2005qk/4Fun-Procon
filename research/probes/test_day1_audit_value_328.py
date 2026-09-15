import json
import unittest
from day1_audit_value_328 import ROOT, validate, bounds, object_hash, submitted_identity
from test_http_prefix_oracle_321 import fixture
from pool_boundary_value_327 import singleton
from test_pool_boundary_value_327 import Contract as PriorContract
from run_http_baseline_314 import Bridge
from http_prefix_option_loss_321 import close_bridge

class Contract(unittest.TestCase):
    def test_original_fields_all_preserved_with_extra_claims(self):
        original={'ok':True,'agrees':True,'agents':[],'ledger':{},'score':[1,2,3]}
        submitted_identity({**original,'claims':[]},original)
        for k in original:
            with self.assertRaises(ValueError):submitted_identity({**original,k:None,'claims':[]},original)

    def test_day1_singleton_four_days_and_provenance(self):
        q=fixture();bridge=Bridge(ROOT/'artifacts/research/324/claims_bridge.exe')
        plan=[[2,-4],[5,-4],[-6]];actual=bridge.request({**q,'op':'step','plan':plan});close_bridge(bridge)
        q.update(portfolios={'p00':singleton(plan,actual)},certificates=[])
        oracle=PriorContract().ask('artifacts/research/326/portfolio_oracle.exe',q)
        self.assertTrue(oracle['ok'],oracle)
        c={'id':'test','request':q,'plans':{'p00':{'plan':plan}}}
        r={'id':'test','manifest_sha256':'m','probe_sha256':'p','request_sha256':object_hash(q),'oracle':oracle}
        validate(r,c,'m','p')
        with self.assertRaises(ValueError):validate(r,c,'bad','p')
        oracle['portfolios']['p00']['days'].pop()
        with self.assertRaises(ValueError):validate(r,c,'m','p')

    def test_bounds_certified_not_provisional_authority(self):
        def s(q):return dict(zip(('lifetimeDistinct','totalDailyDistinct','totalServings'),q))
        a={'validUpperBound':s([5,20,40]),'certified':False,'finalCertifiedLowerBound':s([9,99,99]),'provisionalLowerBound':s([5,20,39])}
        b=bounds(a,[5,20,38]);self.assertFalse(b['upper_unsound']);self.assertFalse(b['certified_lower_unsound']);self.assertTrue(b['provisional_above_optimum'])
        a['certified']=True;self.assertTrue(bounds(a,[5,20,38])['certified_lower_unsound'])

if __name__=='__main__':unittest.main()
