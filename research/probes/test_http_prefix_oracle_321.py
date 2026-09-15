import copy
import json
from pathlib import Path
import subprocess
import sys
import unittest

ROOT=Path(__file__).resolve().parents[2]
DEFAULT=ROOT/"artifacts/research/321/probe.exe"
if "--probe" in sys.argv:
    index=sys.argv.index("--probe");DEFAULT=Path(sys.argv[index+1]);del sys.argv[index:index+2]

def fixture():
    cells=[[3]*8 for _ in range(8)]
    for pos in (16,17,18,19,7):cells[pos//8][pos%8]=0
    return {"setup":{"startsAt":0,"daySeconds":[5]*4,"daySteps":[6,7,6,7],
        "map":{"width":8,"height":8,"cells":cells},
        "spots":[{"brand":10,"pos":17,"stocks":2},{"brand":20,"pos":18,"stocks":2}],
        "agents":[16,19,7],"fuelLimits":3,"players":8,"busyThreshold":1,"jammedThreshold":2},
        "state":{"day":1,"endsAt":0,"agents":[{"kind":0,"pos":p,"fuel":3} for p in (16,19,7)],"others":[],"traffics":[]},
        "ledger":{"brands":[],"totalDailyDistinct":0,"totalServings":0}}

class Contract(unittest.TestCase):
    def setUp(self):self.p=subprocess.Popen([str(DEFAULT)],stdin=subprocess.PIPE,stdout=subprocess.PIPE,stderr=subprocess.PIPE,text=True)
    def tearDown(self):
        self.p.stdin.close();self.p.wait(timeout=10);error=self.p.stderr.read();self.p.stdout.close();self.p.stderr.close()
        self.assertEqual(self.p.returncode,0);self.assertEqual(error,"")
    def ask(self,q):
        self.p.stdin.write(json.dumps(q)+"\n");self.p.stdin.flush();return json.loads(self.p.stdout.readline())
    def test_full_root_against_frozen_forward_dp(self):
        for stock in (1,2,3):
            q=fixture()
            for spot in q["setup"]["spots"]:spot["stocks"]=stock
            r=self.ask(q);ref=self.ask(dict(q,op="reference"));self.assertTrue(r["ok"],r);self.assertTrue(ref["ok"],ref)
            self.assertEqual(r["score"],ref["score"]);self.assertTrue(r["complete"]);self.assertEqual(len(r["days"]),4)
    def test_actual_fuel_and_ledger_not_reset(self):
        q=fixture();q["state"]["day"]=3
        q["state"]["agents"]=[{"kind":0,"pos":p,"fuel":0} for p in (17,18,7)]
        q["ledger"]={"brands":[10],"totalDailyDistinct":3,"totalServings":5}
        r=self.ask(q);self.assertTrue(r["ok"],r);self.assertEqual(r["score"],[2,7,9])
        self.assertEqual(r["days"][-1]["agents"],q["state"]["agents"])
        q["state"]["agents"].reverse();s=self.ask(q);self.assertEqual(r["score"],s["score"])
        self.assertEqual(s["days"][-1]["agents"],q["state"]["agents"])
    def test_scope(self):
        q=fixture();q["state"]["agents"][0]["kind"]=1;self.assertFalse(self.ask(q)["ok"])
        q=fixture();q["setup"]["map"]["cells"][2][0]=1;self.assertFalse(self.ask(q)["ok"])
        q=fixture();q["state"]["agents"][0]["fuel"]=20;self.assertFalse(self.ask(q)["ok"])

if __name__=="__main__":unittest.main()
