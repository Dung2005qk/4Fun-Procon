import copy
import itertools
import unittest
from run_http_baseline_314 import ROOT, Bridge
from http_prefix_option_loss_321 import close_bridge
from test_http_prefix_oracle_321 import fixture


class Contract(unittest.TestCase):
    def setUp(self):
        self.probe=Bridge(ROOT/"artifacts/research/336/probe.exe")
        self.single=Bridge(ROOT/"artifacts/research/330/probe.exe")
        self.judge=Bridge(ROOT/"artifacts/research/314/bridge.exe")

    def tearDown(self):
        for b in (self.probe,self.single,self.judge): close_bridge(b)

    def q(self):
        q=fixture(); q["state"]["day"]=3; q["setup"]["daySteps"]=[2]*4
        q["state"]["agents"]=[{"kind":0,"pos":p,"fuel":1} for p in (17,18,7)]
        q["ledger"]={"brands":[10],"totalDailyDistinct":3,"totalServings":5}
        q["plans"]=[[[-2],[-2],[-2]],[[-2],[-2],[-2]]]; return q

    def brute(self,q,pair):
        # Complete actions on this horizontal two-step corridor: WAIT or one move.
        def visit(state,ledger):
            if state["day"]==5: return [len(ledger["brands"]),ledger["totalDailyDistinct"],ledger["totalServings"]]
            best=None
            for a,b in itertools.product(([-2],[2],[5]),repeat=2):
                plan=[[-2],[-2],[-2]];plan[pair[0]]=a;plan[pair[1]]=b
                r=self.judge.request({"op":"step","setup":q["setup"],"state":state,"ledger":ledger,"plan":plan})
                if not r.get("ok"): continue
                self.assertTrue(r["agrees"])
                value=visit({**state,"day":state["day"]+1,"agents":r["agents"]},r["ledger"])
                if best is None or value>best: best=value
            return best
        return visit(q["state"],q["ledger"])

    def test_pairs_against_independent_walks(self):
        for stock in (1,2,3):
            q=self.q()
            for spot in q["setup"]["spots"]: spot["stocks"]=stock
            out=self.probe.request(q); self.assertTrue(out["ok"],out);self.assertTrue(out["complete"])
            single=self.single.request(q);self.assertTrue(single["ok"])
            for r in out["responses"]:
                self.assertEqual(r["score"],self.brute(q,r["pair"]))
                for a in r["pair"]: self.assertGreaterEqual(r["score"],single["responses"][a]["score"])
                fixed=next(a for a in range(3) if a not in r["pair"])
                for day in r["days"]:
                    self.assertEqual(day["plan"][fixed],[-2]);self.assertEqual(day["agents"][fixed],q["state"]["agents"][fixed])

    def test_zero_fuel_nonreset(self):
        q=self.q()
        for a in q["state"]["agents"]:a["fuel"]=0
        out=self.probe.request(q);self.assertTrue(out["ok"],out)
        self.assertEqual(out["baseline"],[2,7,9])
        for r in out["responses"]:
            self.assertEqual(r["score"],out["baseline"]);self.assertEqual(r["days"][-1]["agents"],q["state"]["agents"])

    def test_safe_exhaustion_and_scope(self):
        for field in ("memo_limit","transition_limit"):
            q=self.q();q[field]=0;r=self.probe.request(q)
            self.assertTrue(r["ok"]);self.assertFalse(r["complete"]);self.assertNotIn("responses",r)
        q=self.q();q["memo_limit"]=250001;self.assertFalse(self.probe.request(q)["ok"])
        q=self.q();q["plans"].pop();self.assertFalse(self.probe.request(q)["ok"])
        q=self.q();q["state"]["agents"][0]["kind"]=1;self.assertFalse(self.probe.request(q)["ok"])
        q=self.q();q["setup"]["map"]["cells"][2][1]=1;self.assertFalse(self.probe.request(q)["ok"])


if __name__=="__main__":unittest.main()
