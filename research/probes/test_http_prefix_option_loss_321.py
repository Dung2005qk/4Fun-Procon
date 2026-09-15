import copy
import unittest
from http_prefix_option_loss_321 import option_losses, validate_result, object_hash

class Contract(unittest.TestCase):
    def test_full_telescope_with_later_tier_tradeoff(self):
        losses = option_losses([[5,20,40], [5,20,40], [5,19,43], [5,19,42], [5,19,42]])
        self.assertEqual([x["first_tier"] for x in losses], [None,2,3,None])
        self.assertEqual(losses[1]["components_lost"], [0,1,-3])

    def test_nonmonotonic_rejected(self):
        with self.assertRaisesRegex(ValueError, "nonmonotone"):
            option_losses([[5,20,40], [5,20,39], [5,20,40], [5,20,38], [5,20,37]])

    def test_result_identity_and_completeness(self):
        case={"id":"contract-d3", "after_day":3, "request":{"state":{"day":4}}}
        result={"id":case["id"], "manifest_sha256":"manifest", "probe_sha256":"probe",
            "request_sha256":object_hash(case["request"]),
            "oracle":{"ok":True,"complete":True,"failure":None,"score":[1,4,8],
                      "days":[{"day":4,"score":[1,4,8]}]}}
        validate_result(result, case, "manifest", "probe")
        for mutate in (lambda r:r.update(id="wrong"), lambda r:r["oracle"].update(complete=False),
                       lambda r:r["oracle"].update(days=[])):
            altered=copy.deepcopy(result);mutate(altered)
            with self.assertRaises((ValueError,IndexError)):
                validate_result(altered, case, "manifest", "probe")

if __name__ == "__main__": unittest.main()
