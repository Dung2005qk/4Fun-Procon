# ATTR-COVERAGE-REGIME-208 registered synthetic grid (SYN-COLGEN-208).
# 2 seeds x 3 spot counts x 2 day-steps x 2 brand modes x 2 player counts = 48 matches.
$exe = "C:\Users\LMC\Desktop\4Fun\build-release\udonshield_colgen_starvation_probe.exe"
$log = "C:\Users\LMC\Desktop\4Fun\research\evidence\ATTR-COVERAGE-REGIME-208-syn.log"
"experiment=ATTR-COVERAGE-REGIME-208" | Out-File $log -Encoding utf8
"split=synthetic-development" | Out-File $log -Append -Encoding utf8
"manifest=research/holdouts/ATTR-COVERAGE-REGIME-208.csv" | Out-File $log -Append -Encoding utf8
foreach ($seed in 6100002, 6100004) {
    foreach ($spots in 24, 30, 32) {
        foreach ($steps in 100, 200) {
            foreach ($brand in "mod6", "distinct") {
                foreach ($players in 2, 4) {
                    & $exe $seed $spots $steps $brand $players 3375 2>&1 |
                        Out-File $log -Append -Encoding utf8
                    if ($LASTEXITCODE -ne 0) {
                        "case_failed,seed=$seed,spots=$spots,steps=$steps,brand=$brand,players=$players,exit=$LASTEXITCODE" |
                            Out-File $log -Append -Encoding utf8
                    }
                }
            }
        }
    }
}
"grid_complete" | Out-File $log -Append -Encoding utf8
