# SCORE-PENULTIMATE-EXACT-181

Date: 2026-08-21  
Parent: `828ea78`  
Status: rejected on consumed capability attribution  

Experiment 180 proves that a day-1 bound certificate can diverge from realized
closed-loop replanning. Experiment 181 therefore keeps the complete protected
parent and exact sidecar but permits takeover only on the public penultimate day.
The existing final-day exact path then has one immediate replan to realize the
terminal resource value. No family, seed, map, bot or fuel-profile guard exists.

Incomplete exact generation, incomplete future certification, invalid plans,
missing bound closure or deadline exhaustion returns the exact completed parent.
Internal compute remains capped at `5000 ms`, with the BTC `1600 ms` network
reserve and `25 ms` validation floor unchanged.

Frozen manifest: `research/holdouts/SCORE-PENULTIMATE-EXACT-181.csv`  
SHA256: `7979A4B372D82A4B4BB6F62A8CD1C626809C5521CA8978CFB94883456F5DC12F`

## Result

The exact generator completed on all three penultimate anchor states, but no
bound-closed takeover occurred and all final scores tied parent. On `1720000`,
the coordinated team-bundle projection exposed only one non-parent candidate at
current `6/9/9`, with certified lower/upper `6/11/11`. It omitted the registered
mask-`111` exact route whose forced profile is `6/12/12 .. 6/13/13`.

Thus the penultimate horizon is not falsified, but coordinated bundle projection
cannot expose the known witness. Fresh and sealed rows were not opened. Reopen
only through a complete per-agent frontier projection that preserves the parent
and does not construct a Cartesian multi-agent solver.
