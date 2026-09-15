"""Re-run every frozen340 contract against the fresh341 integrated library."""
from run_http_baseline_314 import ROOT, Bridge
import test_pair_stock_bound_340 as bounded


class Contract(bounded.Contract):
    def setUp(self):
        super().setUp()
        self.p.close(); self.p.process.stdout.close(); self.p.process.stderr.close()
        self.p = Bridge(ROOT/'artifacts/research/341/probe.exe')


if __name__ == '__main__':
    import unittest
    unittest.main()
