import copy
import unittest
from build_pre_f0_capture_324 import HOOKS, transform
from pre_f0_attribution_324 import ROOT, qualifying, feature, checked_step, load, M322
from run_http_baseline_314 import Bridge
from http_prefix_option_loss_321 import close_bridge

class CaptureTests(unittest.TestCase):
    def test_roundtrip_unique_anchors_and_no_policy_replacement(self):
        for name,hooks in HOOKS.items():
            source=(ROOT/f"src/{name}.cpp").read_text()
            changed=transform(source,hooks)
            for index,(_,body) in enumerate(hooks):
                changed=changed.replace(f"// BEGIN CAPTURE324 {index}\n"+body+f"// END CAPTURE324 {index}\n","")
            self.assertEqual(source,changed)
        with self.assertRaisesRegex(ValueError,"not unique"):
            transform("anchor anchor",[("anchor","obs")])

    def test_observation_cannot_qualify_a_changed_prefix_or_day2_choice(self):
        state={"agents":[{"kind":0,"pos":1,"fuel":2}]*3}
        ledger={"brands":[1],"totalDailyDistinct":2,"totalServings":4}
        dec={"state":state,"ledger":ledger,"decision":{"candidate":{"stableId":"old"}}}
        old={"actions":[{"plan":[[-1]]*3,"validated":{"agents":state["agents"],"ledger":ledger}}, {"plan":[[-1]]*3}]}
        self.assertTrue(qualifying(old,copy.deepcopy(old),dec,copy.deepcopy(dec))[1])
        changed=copy.deepcopy(old);changed["actions"][0]["validated"]["ledger"]["totalServings"]+=1
        self.assertFalse(qualifying(old,changed,dec,dec)[1])
        changed_dec=copy.deepcopy(dec);changed_dec["decision"]["candidate"]["stableId"]="different"
        self.assertFalse(qualifying(old,old,dec,changed_dec)[1])

    def test_features_include_spot_identity_not_only_count_and_fuel(self):
        a={"agents":[{"kind":0,"pos":1,"fuel":2}],"claims":[{"agent":0,"spot":1}]}
        b=copy.deepcopy(a);b["claims"][0]["spot"]=2
        self.assertNotEqual(feature(a,0),feature(b,0))

    def test_claim_bridge_equals_frozen_bridge_and_reports_exact_claims(self):
        canonical=Bridge(ROOT/"artifacts/research/314/bridge.exe")
        claims=Bridge(ROOT/"artifacts/research/324/claims_bridge.exe")
        try:
            case=load(ROOT/M322)["cases"][0]
            request=case["request"]
            plan=case["oracle"]["days"][0]["plan"]
            actual=checked_step(claims,request,plan)
            reference=checked_step(canonical,request,plan)
            self.assertEqual({k:v for k,v in actual.items() if k!="claims"},reference)
            self.assertEqual(sum(c["served"] for c in actual["claims"]),actual["score"][2]-request["ledger"]["totalServings"])
            self.assertEqual(len({(c["agent"],c["spot"]) for c in actual["claims"]}),len(actual["claims"]))
        finally:
            close_bridge(canonical);close_bridge(claims)

if __name__=="__main__": unittest.main()
