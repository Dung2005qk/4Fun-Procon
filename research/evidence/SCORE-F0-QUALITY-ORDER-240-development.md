# SCORE-F0-QUALITY-ORDER-240 development

The frozen local development run completed exactly 120 result rows, 120
matching `case_complete` rows and one `run_complete` marker. Runner stderr was
empty and all frozen hashes matched before aggregation.

Candidate versus parent is `14/36/10` across 60 pairs. Every first difference
is tier 3. Aggregate official score delta is `0/0/+95`; gain sum/tail is
`+251/+132`, loss sum/tail is `-156/-105`. Invalid, emergency, checkpoint,
midday, terminal and public-window failure counters are zero on both sides.

The result is broadly positive but not clean. The 5000-ms public-window lane is
`2/26/4`, net `-10`; the 15000-ms lane is `12/10/6`, net `+105`. Medium is
`2/10/4`, net `-10`, while very-hard is `6/4/4`, net `-40`, dominated by one
`-105` low-fuel threshold-corridor tail. The largest independent gain is `+132`
on hard low-fuel overnight. Low fuel is `1/12/3`, net `+17`; high fuel is
`6/9/5`, net zero; default is `7/15/2`, net `+78`.

This clears only the preregistered development screening gate: wins exceed
losses, aggregate benefit exceeds aggregate downside, the largest gain exceeds
the largest loss, no tier-1/tier-2 or safety regression exists, and positive
effects span both role modes and both public-window lanes. The negative 5000-ms
and medium strata prohibit promotion from local evidence and make the sealed VM
holdout mandatory.

At development closure no authorized UDON research VM existed. After the user
continued the experiment, the dedicated Spot VM `udon-f0-240-0829` was created
for the mandatory sealed holdout. The unrelated terminated VMs
`cace-canary187` and `aegis-canary-316a0b3a` remain untouched.
