"""Synthetic contract fixtures only, never frozen protected gameplay results."""
import copy
import unittest
from unittest.mock import patch
from protected_http_transport_341 import operational
from protected_http_transport_341 import ROOT,Bridge,ProtectedCase,traffic_for
from test_http_baseline_314 import Clock


def contract_case(role='fixed-all-Patrol',days=4):
    cells=[[0]*8 for _ in range(8)];cells[1][1]=cells[1][2]=1
    setup={'startsAt':1778227200,'map':{'width':8,'height':8,'cells':cells},
        'agents':[8,16,24],'spots':[{'pos':18,'brand':100,'stocks':2},{'pos':26,'brand':101,'stocks':1}],
        'daySteps':[6+i%4 for i in range(days)],'daySeconds':[(5,10,15,5)[i%4] for i in range(days)],
        'fuelLimits':6,'players':8,'busyThreshold':1,'jammedThreshold':2}
    external=[[0]*64 for _ in range(days)]
    for i in range(days):external[i][9]=(0,8,16,0)[i%4]
    return {'seed':3419001,'role':role,'days':days,'family':'contract-only','setup':setup,
            'external_road_footprints':external}


class ProtectedContract(unittest.TestCase):
    @classmethod
    def setUpClass(cls):cls.bridge=Bridge(ROOT/'artifacts/research/341/protected-bridge.exe')
    @classmethod
    def tearDownClass(cls):
        cls.bridge.close();cls.bridge.process.stdout.close();cls.bridge.process.stderr.close()
    def make(self,role='fixed-all-Patrol',days=4):
        clock=Clock();return ProtectedCase(contract_case(role,days),self.bridge,clock,lambda:1700000000+clock.now),clock
    def test_fixed_roles_from_day_one(self):
        case,_=self.make();self.assertEqual(case.roles,[0]*3)
        with self.assertRaises(ValueError):case.post('assignment',[0]*3)
    def test_parent_without_new_counter_is_not_candidate(self):
        replay=ROOT/'research/evidence/SCORE-W1-STOCK-RELAXED-PAIR-PRICING-341-protected-http-preflight/parent/3419001.replay.jsonl'
        with patch('protected_http_transport_341.pricing_safety') as check:
            self.assertTrue(operational(replay,contract_case('native'),'parent')['safety_pass'])
            check.assert_not_called()
        with self.assertRaises(KeyError):operational(replay,contract_case('native'),'candidate')
        with self.assertRaisesRegex(ValueError,'binary side'):operational(replay,contract_case(),'unknown')
    def test_native_real_assignment_before_state(self):
        case,_=self.make('native');self.assertEqual(case.get('state')[0],425)
        self.assertEqual(case.get('start')[0],425)
        case.post('assignment',[0,1,0]);self.assertEqual(case.get('start')[0],200)
        self.assertEqual([a['kind'] for a in case.get('state')[1]['agents']],[0,1,0])
        with self.assertRaises(ValueError):case.post('assignment',[0,0,0])
    def test_invalid_roles_do_not_mutate(self):
        for roles in ([0],[0,2,0],['0',0,0]):
            case,_=self.make('native')
            with self.assertRaises(ValueError):case.post('assignment',roles)
            self.assertIsNone(case.roles)
    def test_native_assignment_idempotent_before_start_only(self):
        case,_=self.make('native');case.post('assignment',[0,1,0]);case.post('assignment',[0,1,0])
        self.assertEqual(case.roles,[0,1,0]);case.get('state')
        with self.assertRaises(ValueError):case.post('assignment',[0,1,0])
    def test_variable_windows_and_ten_day_completion(self):
        case,clock=self.make(days=10)
        for day in range(10):
            status,state=case.get('state');self.assertEqual((status,state['day']),(200,day))
            self.assertEqual(case.window_ms,case.setup['daySeconds'][day]*1000)
            self.assertEqual(state['endsAt'],int((1700000000+clock.now)*1000)+case.window_ms)
            plan=[[-case.setup['daySteps'][day]]]*3
            before=copy.deepcopy(case.ledger);case.post('actions',plan);case.post('actions',plan)
            self.assertEqual(case.day,day);self.assertEqual(case.ledger,before)
            self.assertEqual(case.get('state')[0],425)
            clock.now=case.deadline
        self.assertEqual(case.get('state')[0],404);self.assertEqual(case.get('result')[0],200)
        self.assertEqual(len(case.actions),10);self.assertEqual(len(case.own),10)
    def test_late_or_changed_action_never_commits(self):
        case,clock=self.make();case.get('state');clock.now=case.deadline
        with self.assertRaisesRegex(ValueError,'deadline'):case.post('actions',[[-6]]*3)
        self.assertEqual(case.day,0);self.assertEqual(case.own,[])
        case,clock=self.make();case.get('state');case.post('actions',[[-6]]*3)
        with self.assertRaisesRegex(ValueError,'non-idempotent'):case.post('actions',[[-1]]*3)
    def test_unanswered_day_and_invalid_plan_fail_closed(self):
        case,clock=self.make();case.get('state')
        with self.assertRaisesRegex(ValueError,'dual'):case.post('actions',[[2]*20,[-6],[-6]])
        self.assertIsNone(case.pending);clock.now=case.deadline
        with self.assertRaisesRegex(ValueError,'without accepted'):case.get('state')
    def test_source_cost_and_own_road_footprint_are_exact(self):
        case,clock=self.make();case.get('state');case.post('actions',[[2,-4],[-6],[-6]])
        self.assertEqual(case.pending['agents'][0]['pos'],9)
        self.assertEqual(case.pending['agents'][0]['fuel'],5)
        self.assertGreater(case.pending['road_footprint'][9],0)
        self.assertEqual(case.own,[]);clock.now=case.deadline;case.get('state')
        self.assertEqual(len(case.own),1)
    def test_two_day_traffic_thresholds_and_expiration(self):
        case=contract_case();s=case['setup'];ext=[[0]*64 for _ in range(4)];own=[[0]*64 for _ in range(4)]
        self.assertEqual(traffic_for(s,0,[],ext),[{'pos':9,'status':0},{'pos':10,'status':0}])
        own[0][9]=3;ext[0][9]=5
        self.assertEqual(traffic_for(s,1,own[:1],ext)[0]['status'],1)
        ext[1][9]=8
        self.assertEqual(traffic_for(s,2,own[:2],ext)[0]['status'],2)
        self.assertEqual(traffic_for(s,3,own[:3],ext)[0]['status'],1)
        self.assertEqual(traffic_for(s,4,own[:4],ext)[0]['status'],0)
    def test_team_normalization_and_exact_threshold_boundary(self):
        for teams in (8,9,10):
            s=contract_case()['setup'];s['players']=teams
            ext=[[0]*64 for _ in range(4)];own=[[0]*64]
            for mass,status in ((teams-1,0),(teams,1),(2*teams-1,1),(2*teams,2)):
                ext[0][9]=mass;self.assertEqual(traffic_for(s,1,own,ext)[0]['status'],status)
    def test_state_traffic_frozen_for_open_day(self):
        case,clock=self.make();first=copy.deepcopy(case.get('state')[1]);clock.now+=1
        self.assertEqual(case.get('state')[1],first)
    def test_new_bridge_matches_original_exact_transition(self):
        original=Bridge(ROOT/'artifacts/research/314/bridge.exe')
        try:
            case,_=self.make();state=case.get('state')[1]
            q={'op':'step','setup':case.setup,'state':dict(state,day=1,endsAt=state['endsAt']//1000),
               'ledger':case.ledger,'plan':[[2,-4],[-6],[-6]]}
            new=self.bridge.request(q);old=original.request(q);new.pop('road_footprint')
            self.assertEqual(new,old)
        finally:original.close();original.process.stdout.close();original.process.stderr.close()


if __name__=='__main__':unittest.main()
