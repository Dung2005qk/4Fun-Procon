import copy
import json
import unittest
from selected_stock_neutral_319 import selected_request,load,ROOT,Bridge,close_bridge

class SelectedContract(unittest.TestCase):
    def test_actual_selected_witness_revalidates_and_invalid_certificate_blocks(self):
        m=load("research/holdouts/ATTR-HTTP-SHORTLIST-CERTIFICATE-YIELD-316.json")
        case=next(c for c in m["cases"] if c["seed"]==10700100)
        path=ROOT/"research/evidence/ATTR-THREE-PATROL-HTTP-BASELINE-314/10700100.replay.jsonl"
        body=next(e["body"] for e in map(json.loads,path.read_text().splitlines()) if e["kind"]=="decision")
        bridge=Bridge(ROOT/"artifacts/research/314/bridge.exe")
        try:
            request,score=selected_request(body,case["setup"],bridge)
            self.assertEqual(request["state"]["day"],2);self.assertEqual(score,[4,16,36])
            bad=copy.deepcopy(body);bad["decision"]["profile"]["outcomes"][0]["certified"]=False
            with self.assertRaisesRegex(ValueError,"selected full"):
                selected_request(bad,case["setup"],bridge)
        finally:
            close_bridge(bridge)

if __name__=="__main__":
    unittest.main()
