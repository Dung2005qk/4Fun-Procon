import unittest
from w1_prefix_headroom_343 import headroom


class HeadroomContract(unittest.TestCase):
    def test_does_not_spend_network_validation_or_rounding(self):
        self.assertEqual(headroom({'totalMs':5000,'networkMs':1600,'certificationMs':1000},{'totalMs':200}),3174)
    def test_expired_interval_stays_zero(self):
        self.assertEqual(headroom({'totalMs':5000,'networkMs':1600,'certificationMs':1000},{'totalMs':3380}),0)
    def test_small_certification_bucket_is_not_expanded(self):
        self.assertEqual(headroom({'totalMs':100,'networkMs':50,'certificationMs':10},{'totalMs':30}),9)


if __name__=='__main__':unittest.main()
