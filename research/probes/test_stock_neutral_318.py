import copy
import unittest
from run_http_baseline_314 import ROOT, Bridge
from replay_witness_suffix_317 import load, M316

class ProbeContract(unittest.TestCase):
    def test_wait_control_and_scope_guard(self):
        case=load(M316)["cases"][0]
        request={"setup":case["setup"],"state":dict(case["state"],day=2),
                 "ledger":case["ledger"],"plan":[[-case["setup"]["daySteps"][1]]]*3}
        process=Bridge(ROOT/"artifacts/research/318/probe.exe")
        try:
            result=process.request(request)
            self.assertTrue(result["ok"],result)
            self.assertEqual(len(result["alternatives"]),3)
            for alternative in result["alternatives"]:
                self.assertTrue(alternative["neutral"])
                self.assertEqual(alternative["certificate"]["score"],result["regenerated_control"]["score"])
                self.assertEqual(alternative["certificate"]["dual_valid_days"],2)
            invalid=copy.deepcopy(request);invalid["state"]["day"]=1
            self.assertFalse(process.request(invalid)["ok"])
        finally:
            process.close();process.process.stdout.close();process.process.stderr.close()

if __name__=="__main__":
    unittest.main()
