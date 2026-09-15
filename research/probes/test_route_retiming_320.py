import copy
import unittest
from run_http_baseline_314 import ROOT, Bridge
from selected_stock_neutral_319 import close_bridge

def fixture():
    return {"setup":{"startsAt":0,"daySeconds":[5]*4,"daySteps":[9]*4,
        "map":{"width":8,"height":8,"cells":[[0]*8 for _ in range(8)]},
        "spots":[{"brand":10,"pos":17,"stocks":3},{"brand":20,"pos":18,"stocks":3}],
        "agents":[16,0,7],"fuelLimits":4,"players":8,"busyThreshold":1,"jammedThreshold":2},
        "state":{"day":2,"endsAt":0,"agents":[{"kind":0,"pos":p,"fuel":4} for p in (17,0,7)],"others":[],"traffics":[]},
        "ledger":{"brands":[],"totalDailyDistinct":0,"totalServings":0},
        "plans":[[[-1,2,5,2,5],[-9],[-9]],[[-9],[-9],[-9]],[[-9],[-9],[-9]]]}

class Contract(unittest.TestCase):
    def setUp(self):self.p=Bridge(ROOT/"artifacts/research/320/probe.exe")
    def tearDown(self):close_bridge(self.p)
    def test_all_cuts_geometry_and_known_gain(self):
        q=fixture();r=self.p.request(q);self.assertTrue(r["ok"],r)
        self.assertEqual(r["attempted"],17)
        self.assertEqual(r["baseline"]["score"],[2,4,4])
        self.assertGreater(r["best"]["score"],r["baseline"]["score"])
        for row in r["rows"]:
            self.assertEqual(row["replay"]["agents"],r["baseline"]["agents"])
            for a in range(3):
                before=[x for d in q["plans"] for x in d[a] if x>=0]
                after=[x for d in row["replay"]["plans"] for x in d[a] if x>=0]
                self.assertEqual(before,after)
                if a!=row["agent"]:self.assertEqual([d[a] for d in q["plans"]],[d[a] for d in row["replay"]["plans"]])
        self.assertEqual(r,self.p.request(q))
    def test_variable_days_and_infeasible_normalization(self):
        q=fixture();q["setup"]["daySteps"]=[9,8,5,1]
        q["plans"]=[[[2,5,2,5],[-8],[-8]],[[-5],[-5],[-5]],[[-1],[-1],[-1]]]
        r=self.p.request(q);self.assertTrue(r["ok"],r)
        self.assertIsNone(r["controls"][0]["normalized"])
        self.assertGreater(r["infeasible_duration"],0)
        self.assertEqual(r["attempted"],r["valid"]+r["duplicates"]+r["infeasible_duration"])
    def test_scope_and_invalid_recorded_fail_closed(self):
        for change in ("tanker","road","count","invalid"):
            q=copy.deepcopy(fixture())
            if change=="tanker":q["state"]["agents"][0]["kind"]=1
            elif change=="road":q["setup"]["map"]["cells"][3][0]=1
            elif change=="count":q["plans"].pop()
            else:q["plans"][0][0]=[-100]
            self.assertFalse(self.p.request(q)["ok"])

if __name__=="__main__":unittest.main()
