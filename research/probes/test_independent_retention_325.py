import json
import subprocess
import unittest
from independent_retention_325 import ROOT,bundle_admissible,load
from build_pre_prune_capture_325 import HOOKS,transform

class RetentionTests(unittest.TestCase):
    def test_normalized_source_roundtrip_and_raw_call_scope(self):
        for name,hooks in HOOKS.items():
            original=(ROOT/f"src/{name}.cpp").read_text()
            transformed=transform(original,hooks)
            for index,(_,body) in enumerate(hooks):
                transformed=transformed.replace(f"// BEGIN CAPTURE324 {index}\n"+body+f"// END CAPTURE324 {index}\n","")
            self.assertEqual(original,transformed)
        source=transform((ROOT/"src/decision.cpp").read_text(),HOOKS["decision"])
        self.assertEqual(source.count("capture325::start"),2)
        self.assertEqual(source.count("capture325::end"),2)

    def test_all_125_modes_against_canonical_master_consumer(self):
        request=load(ROOT/"research/holdouts/ATTR-DAY2-OPTION-PROVENANCE-322.json")["cases"][0]["request"]
        result=subprocess.run([str(ROOT/"artifacts/research/325/bundle_contract.exe")],input=json.dumps(request)+"\n",
            capture_output=True,text=True,timeout=15,creationflags=getattr(subprocess,"CREATE_NO_WINDOW",0))
        self.assertEqual(result.returncode,0,result.stderr)
        self.assertFalse(result.stderr)
        rows=json.loads(result.stdout);self.assertEqual(len(rows),125)
        for row in rows:self.assertEqual(bundle_admissible(row["modes"]),row["admissible"],row)
        self.assertFalse(bundle_admissible([-1,1,1]))
        self.assertTrue(bundle_admissible([-2,-1,-2]))
        self.assertTrue(bundle_admissible([1,1,1]))

if __name__=="__main__":unittest.main()
