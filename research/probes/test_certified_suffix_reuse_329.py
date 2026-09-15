import copy
import unittest
from certified_suffix_reuse_329 import mappings, splice, replay, ROOT, Bridge, close_bridge
from test_http_prefix_oracle_321 import fixture

class Contract(unittest.TestCase):
    def test_resource_mapping(self):
        donor=[{'kind':0,'pos':p,'fuel':f} for p,f in ((17,1),(18,2),(19,3))]
        recipient=[dict(donor[i],fuel=donor[i]['fuel']+1) for i in (2,0,1)]
        self.assertEqual(mappings(recipient,donor),[[2,0,1]])
        recipient[0]['fuel']=2;self.assertEqual(mappings(recipient,donor),[])
        recipient[0]['fuel']=4;recipient[0]['pos']=20;self.assertEqual(mappings(recipient,donor),[])
        recipient[0]['kind']=1
        with self.assertRaises(ValueError):mappings(recipient,donor)

    def test_splice_preserves_prefix_and_permutation_entire_suffix(self):
        a=[[[i*10+j] for j in range(3)] for i in range(4)]
        b=[[[100+i*10+j] for j in range(3)] for i in range(4)]
        original=copy.deepcopy(a);result=splice(a,b,2,[2,0,1])
        self.assertEqual(result[:2],a[:2]);self.assertEqual(result[2:],[[day[2],day[0],day[1]] for day in b[2:]])
        result[0][0].append(999);self.assertEqual(a,original)

    def test_full_replay_keeps_recipient_ledger_not_donor_credit(self):
        q=fixture();q['ledger']={'brands':[10],'totalDailyDistinct':3,'totalServings':7}
        plans=[[[2,-4],[5,-4],[-6]],[[-7],[-7],[-7]],[[-6],[-6],[-6]],[[-7],[-7],[-7]]]
        bridge=Bridge(ROOT/'artifacts/research/314/bridge.exe')
        try:
            r=replay(bridge,q['setup'],q['state'],q['ledger'],plans)
            self.assertEqual(r['nodes'][0]['ledger'],q['ledger'])
            self.assertEqual(r['score'],[2,11,15])
        finally:close_bridge(bridge)

if __name__=='__main__':unittest.main()
