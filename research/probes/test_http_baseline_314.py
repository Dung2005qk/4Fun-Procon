import copy
import unittest
from run_http_baseline_314 import ROOT, Bridge, CaseState, EMPTY_LEDGER
from summarize_http_baseline_314 import compare


class Clock:
    now=0.0
    def __call__(self): return self.now


class HttpBaselineTests(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        cls.bridge=Bridge(ROOT/"artifacts/research/314/bridge.exe")
        result=cls.bridge.request({"op":"fixture","seed":10700000,"family":"three-balanced","players":8})
        if not result.get("ok"): raise AssertionError(result)
        cls.setup=result["setup"]

    @classmethod
    def tearDownClass(cls): cls.bridge.close()

    def new_case(self):
        clock=Clock()
        return CaseState(copy.deepcopy(self.setup),self.bridge,clock=clock,wall=lambda:1700000000+clock.now),clock

    def test_fixture_identity(self):
        self.assertEqual(self.setup["agents"],[16,22,23])
        self.assertEqual(self.setup["fuelLimits"],10)
        self.assertEqual(self.setup["daySeconds"],[5]*4)
        self.assertEqual(len(self.setup["spots"]),5)
        self.assertFalse(any(c==1 for row in self.setup["map"]["cells"] for c in row))

    def test_wait_is_dual_valid_and_ack_advances_once_at_day_end(self):
        case,clock=self.new_case()
        status,state=case.get("state")
        self.assertEqual((status,state["day"]),(200,0))
        plan=[[-self.setup["daySteps"][0]]]*3
        self.assertEqual(case.post("actions",plan)[1]["day"],1)
        self.assertEqual(case.get("state")[0],425)
        self.assertEqual(case.day,0)
        self.assertEqual(case.ledger,EMPTY_LEDGER)
        case.post("actions",plan)
        self.assertEqual(len(case.actions),1)
        clock.now=5.0
        self.assertEqual(case.get("state")[1]["day"],1)
        self.assertEqual(case.agents,state["agents"])

    def test_late_actions_never_migrate_into_next_day(self):
        case,clock=self.new_case(); case.get("state"); clock.now=5.001
        with self.assertRaisesRegex(RuntimeError,"after.*deadline"):
            case.post("actions",[[-1]]*3)
        self.assertEqual(case.day,0)

    def test_invalid_move_rejected_and_ledger_unchanged(self):
        case,_=self.new_case(); case.get("state")
        with self.assertRaisesRegex(RuntimeError,"dual validation"):
            case.post("actions",[[0,-15],[-16],[-16]])
        self.assertEqual(case.ledger,EMPTY_LEDGER)
        self.assertIsNone(case.pending)

    def test_unanswered_day_fails_closed(self):
        case,clock=self.new_case(); case.get("state"); clock.now=5
        with self.assertRaisesRegex(RuntimeError,"without an accepted"):
            case.get("state")

    def test_changed_retry_body_is_rejected(self):
        case,_=self.new_case(); case.get("state")
        case.post("actions",[[-self.setup["daySteps"][0]]]*3)
        with self.assertRaisesRegex(RuntimeError,"non-idempotent"):
            case.post("actions",[[-1]]*3)

    def test_official_comparison_is_lexicographic(self):
        self.assertEqual(compare([5,1,1],[4,1000,1000]),("win",1,1))
        self.assertEqual(compare([5,20,38],[5,20,36]),("win",3,2))
        self.assertEqual(compare([5,20,38],[5,20,38]),("tie",0,0))


if __name__=="__main__": unittest.main()
