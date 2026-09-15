import copy
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

import pair_pricing_prevalence_337 as task


class Contract(unittest.TestCase):
    def test_freshness_is_gameplay_not_label_or_order(self):
        setup = {"startsAt": 1, "players": 8, "daySteps": [16]*4,
                 "spots": [{"pos": 4, "brand": 8, "stocks": 2}, {"pos": 3, "brand": 7, "stocks": 1}]}
        alias = copy.deepcopy(setup); alias["players"] = 10; alias["startsAt"] = 9; alias["spots"].reverse()
        self.assertEqual(task.gameplay_hash(setup), task.gameplay_hash(alias))
        with self.assertRaisesRegex(ValueError, "duplicate"): task.assert_fresh([setup, alias], [])
        with self.assertRaisesRegex(ValueError, "consumed"): task.assert_fresh([setup], [alias])
        fresh = copy.deepcopy(setup); fresh["daySteps"][0] += 1
        self.assertEqual(len(task.assert_fresh([fresh], [setup])), 1)

    def test_resume_rejects_partial_or_noncontiguous_http(self):
        m = {"cases": [{"seed": 1}, {"seed": 2}]}
        with tempfile.TemporaryDirectory(prefix="udon337-contract-") as tmp:
            d = Path(tmp)
            self.assertEqual(task.baseline_inventory(m, d), [])
            (d/"1.replay.jsonl").touch()
            with self.assertRaisesRegex(ValueError, "ambiguous"): task.baseline_inventory(m, d)
        with tempfile.TemporaryDirectory(prefix="udon337-contract-") as tmp:
            d = Path(tmp)
            for suffix in task.SUFFIXES: (d/("2"+suffix)).touch()
            with self.assertRaisesRegex(ValueError, "noncontiguous"): task.baseline_inventory(m, d)

    def test_frozen_pair_validation_and_forgery_rejection(self):
        manifest = task.load(task.ROOT/"research/holdouts/ATTR-W1-PAIR-RESOURCE-RESPONSE-336.json")
        c = manifest["cases"][0]
        single = task.load(task.ROOT/("research/evidence/ATTR-W1-HORIZON-PATROL-BEST-RESPONSE-330/"+c["id"]+".result.json"))["output"]
        pair = task.load(task.ROOT/("research/evidence/ATTR-W1-PAIR-RESOURCE-RESPONSE-336/"+c["id"]+".result.json"))["output"]
        # Independent bridge verifies recorded complete outputs; no new search.
        bridge = task.Bridge(task.ROOT/"artifacts/research/314/bridge.exe")
        try:
            state = c["request"]["state"]; ledger = c["request"]["ledger"]; days = []
            for plan in c["request"]["plans"]:
                r = bridge.request({"op": "step", "setup": c["request"]["setup"], "state": state, "ledger": ledger, "plan": plan})
                self.assertTrue(r["ok"]); days.append(r)
                state = {**state, "day": state["day"]+1, "agents": r["agents"]}; ledger = r["ledger"]
            context = {"request": c["request"], "baseline": c["baseline"], "valid_upper_bound": c["exact_value"], "original_days": days}
            self.assertEqual(task.validate_both(single, pair, context, bridge), 18)
            bad = copy.deepcopy(pair); bad["responses"].pop()
            with self.assertRaisesRegex(ValueError, "wrong control"): task.validate_both(single, bad, context, bridge)
            bad = copy.deepcopy(pair); bad["responses"][0]["days"][0]["ledger"]["totalServings"] += 1
            with self.assertRaisesRegex(ValueError, "dual identity"): task.validate_both(single, bad, context, bridge)
        finally: task.close_bridge(bridge)


if __name__ == "__main__": unittest.main()
