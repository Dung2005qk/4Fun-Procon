import unittest
from conditional_ceiling_346 import deduplicate,strict_gate

class Contract(unittest.TestCase):
    def test_exact_request_dedup_retains_all_refs(self):
        refs=[{'seed':i,'request':{'state':{'fuel':f}}} for i,f in enumerate((1,1,2))]
        u=deduplicate(refs);self.assertEqual(len(u),2);self.assertEqual(sum(len(x['references']) for x in u),3)
        self.assertEqual([r['seed'] for r in u[0]['references']],[0,1])
    def test_strict_not_equal_or_lower_bound_comparison(self):
        rows=[{'seed':1,'family':'a','inherited_certificate':[4,16,37],'conditional_optimum':[4,16,37]},
              {'seed':2,'family':'b','inherited_certificate':[4,16,37],'conditional_optimum':[4,16,36]}]
        self.assertFalse(strict_gate(rows));rows[0]['conditional_optimum']=[4,16,36];self.assertTrue(strict_gate(rows))
    def test_repeats_not_independent(self):
        self.assertFalse(strict_gate([{'seed':1,'family':f,'inherited_certificate':[4,16,37],'conditional_optimum':[4,16,36]}
            for f in ('a','b')]))

if __name__=='__main__':unittest.main()
