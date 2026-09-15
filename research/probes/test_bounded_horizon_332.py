import copy
import itertools
import unittest
from http_prefix_option_loss_321 import ROOT, load, close_bridge
from run_http_baseline_314 import Bridge
from test_http_prefix_oracle_321 import fixture


class Contract(unittest.TestCase):
    def setUp(self):
        self.p = Bridge(ROOT/"artifacts/research/332/probe.exe")
        self.parent = Bridge(ROOT/"artifacts/research/332/parent-probe.exe")
        self.judge = Bridge(ROOT/"artifacts/research/314/bridge.exe")
        self.claims = Bridge(ROOT/"artifacts/research/324/claims_bridge.exe")

    def tearDown(self):
        for p in (self.p, self.parent, self.judge, self.claims): close_bridge(p)

    def q(self):
        q = fixture(); q["state"]["day"] = 3; q["setup"]["daySteps"] = [4]*4
        q["state"]["agents"] = [{"kind": 0, "pos": p, "fuel": 2} for p in (17, 18, 7)]
        q["ledger"] = {"brands": [10], "totalDailyDistinct": 3, "totalServings": 5}
        q.update(op="price", expired=False, corrupt=False, plans=[[[-4]]*3, [[-4]]*3])
        return q

    def brute(self, q, agent):
        n = len(q["state"]["agents"])
        def visit(state, ledger):
            if state["day"] == 5: return [len(ledger["brands"]), ledger["totalDailyDistinct"], ledger["totalServings"]]
            best = None
            for count in range(3):
                for moves in itertools.product((2, 5), repeat=count):
                    for wait in (0, 1):
                        used = wait+2*count
                        if used > 4: continue
                        plan = [[-4] for _ in range(n)]
                        plan[agent] = ([-1] if wait else []) + list(moves) + ([-(4-used)] if used < 4 else [])
                        r = self.judge.request({"op": "step", "setup": q["setup"], "state": state, "ledger": ledger, "plan": plan})
                        if not r.get("ok"): continue
                        self.assertTrue(r["agrees"])
                        score = visit({**state, "day": state["day"]+1, "agents": r["agents"]}, r["ledger"])
                        if best is None or score > best: best = score
            return best
        return visit(q["state"], q["ledger"])

    def test_whole_horizon_against_independent_walks_and_stock(self):
        for stock in (1, 2, 3):
            q = self.q()
            for spot in q["setup"]["spots"]: spot["stocks"] = stock
            r = self.p.request(q); self.assertTrue(r["ok"], r)
            self.assertEqual(r["completed"], 1, r); self.assertEqual(r["failures"], 0)
            self.assertEqual(r["score"], max(self.brute(q, a) for a in range(3)))
            self.assertTrue(r["originalUnchanged"])

    def test_three_to_eight_patrols_and_zero_fuel(self):
        for n in (3, 4, 5, 6, 7, 8):
            q = self.q()
            extra = list(range(n-3))
            for p in extra: q["setup"]["map"]["cells"][p//8][p%8] = 0
            q["setup"]["agents"] += extra
            q["state"]["agents"] += [{"kind": 0, "pos": p, "fuel": 0} for p in extra]
            q["plans"] = [[[-4] for _ in range(n)] for _ in range(2)]
            for a in q["state"]["agents"]: a["fuel"] = 0
            r = self.p.request(q); self.assertTrue(r["ok"], r)
            self.assertEqual(r["completed"], 1, r); self.assertEqual(r["failures"], 0)
            self.assertEqual(r["score"], [2, 7, 9]); self.assertEqual(r["plans"], q["plans"])

    def test_exhaustion_and_unsupported_or_corrupt_preserve_original(self):
        q = self.q(); q["expired"] = True
        r = self.p.request(q); self.assertEqual(r["exhausted"], 1); self.assertEqual(r["plans"], q["plans"])
        self.assertTrue(r["originalUnchanged"]); self.assertEqual(r["improvements"], 0)
        q = self.q(); q["corrupt"] = True
        r = self.p.request(q); self.assertEqual(r["failures"], 1); self.assertTrue(r["originalUnchanged"])
        q = self.q(); q["state"]["agents"][2]["kind"] = 1
        r = self.p.request(q); self.assertEqual(r["supported"], 0); self.assertEqual(r["plans"], q["plans"])
        q = self.q(); q["setup"]["map"]["cells"][7][7] = 1
        q["state"]["traffics"] = [{"pos": 63, "status": 0}]
        r = self.p.request(q); self.assertEqual(r["supported"], 0); self.assertEqual(r["plans"], q["plans"])

    def test_frontier_resource_caps_and_initial_wait_mask(self):
        q = self.q(); q.update(op="frontier", settled=32768, created=196609, actions=262144)
        r = self.p.request(q); self.assertTrue(r["complete"], r)
        self.assertTrue(any(p["cell"] == 16 for p in r["routes"]))
        for route in r["routes"]:
            plan = [route["plan"][0], [-4], [-4]]
            s = self.claims.request({"op": "step", "setup": q["setup"], "state": q["state"], "ledger": q["ledger"], "plan": plan})
            self.assertTrue(s["ok"] and s["agrees"], s)
            mask = 0
            for c in s["claims"]:
                if c["agent"] == 0: mask |= 1 << c["spot"]
            self.assertEqual(route["mask"], mask)
            self.assertEqual(s["agents"][0]["fuel"], q["state"]["agents"][0]["fuel"]-route["fuel"])
        for key in ("settled", "created", "actions"):
            limited = copy.deepcopy(q); limited[key] = 1
            r = self.p.request(limited); self.assertFalse(r["complete"], (key, r)); self.assertEqual(r["routes"], [])

    def test_variable_days_and_incomplete_witness(self):
        q = self.q(); q["state"]["day"] = 2
        q["setup"]["daySteps"] = [4, 3, 5, 7]
        q["plans"] = [[[-s] for _ in range(3)] for s in (3, 5, 7)]
        r = self.p.request(q); self.assertTrue(r["ok"], r)
        self.assertEqual(r["completed"], 1, r); self.assertEqual(r["failures"], 0)
        self.assertGreaterEqual(r["score"], r["baseline"])
        q["plans"] = q["plans"][:1]
        r = self.p.request(q); self.assertTrue(r["ok"], r)
        self.assertEqual(r["failures"], 1); self.assertTrue(r["originalUnchanged"])

    def test_validation_work_is_bounded_before_entering_engines(self):
        q = self.q(); q["setup"]["daySteps"] = [50000]*4
        q["plans"] = [[[-50000]]*3]*2
        r = self.p.request(q); self.assertTrue(r["ok"], r)
        self.assertEqual(r["exhausted"], 1); self.assertEqual(r["completed"], 0)
        self.assertEqual(r["transitions"], 0); self.assertTrue(r["originalUnchanged"])

    def test_original_sparse_and_dense_callers_remain_exact(self):
        m = load(ROOT/"research/holdouts/ATTR-THREE-PATROL-HTTP-BASELINE-314.json")
        for setup in m["setups"]:
            q = {"op": "legacy", "setup": setup, "state": {"day": 1, "endsAt": 0,
                 "agents": [{"kind": 0, "pos": p, "fuel": setup["fuelLimits"]} for p in setup["agents"]],
                 "others": [], "traffics": []}, "ledger": {"brands": [], "totalDailyDistinct": 0, "totalServings": 0}}
            a = self.parent.request(q); b = self.p.request(q)
            self.assertTrue(a["ok"] and b["ok"]); self.assertEqual(a, b)

    def test_complete_consumed_control_capability_matches330(self):
        m = load(ROOT/"research/holdouts/ATTR-W1-HORIZON-PATROL-BEST-RESPONSE-330.json")
        for c in m["cases"]:
            old = load(ROOT/("research/evidence/ATTR-W1-HORIZON-PATROL-BEST-RESPONSE-330/"+c["id"]+".result.json"))
            q = {**c["request"], "op": "price", "expired": False, "corrupt": False}
            r = self.p.request(q); self.assertTrue(r["ok"], r); self.assertEqual(r["failures"], 0, r)
            self.assertEqual(r["completed"], 1, (c["id"], r))
            self.assertEqual(r["score"], max(a["score"] for a in old["output"]["responses"]), c["id"])
            self.assertLessEqual(r["settled"], 32768); self.assertLessEqual(r["created"], 196609)
            self.assertLessEqual(r["actions"], 262144); self.assertLessEqual(r["memo"], 8192)
            self.assertLessEqual(r["transitions"], 262144); self.assertLessEqual(r["queries"], 1024)


if __name__ == "__main__": unittest.main()
