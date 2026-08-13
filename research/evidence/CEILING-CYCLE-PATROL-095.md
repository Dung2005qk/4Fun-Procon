# CEILING-CYCLE-PATROL-095

Parent: `f77c101`

Frozen manifest: `research/holdouts/CEILING-CYCLE-PATROL-095.csv`

SHA256: `C20467B8E28226710D6F310A3E24B166A8BD1582B955FD5B5E9F102BA8E4C866`

The research-only adapter uses a new 3x3 cyclic roadless component, two active
patrols, one isolated control, six spots and a non-spot mountain at the center.
The first adapter attempt placed spots on mountain cells and was rejected by the
configuration parser before any score was produced. The topology was corrected
before opening development; the manifest, split, seeds and hypothesis did not
change.

The unchanged complete joint full-match DP finished all 18 development fixtures.
Every oracle witness and every HEAD result agreed with the exact simulator and
independent validator. Oracle-vs-HEAD was `4/14/0`, invalid `0`, tier-1 `0`,
tier-2 `3`, tier-3 `1`, maximum first-tier gain `3`, result hash
`6c160390445423ed`.

Exact wins:

- cycle-balanced low, seed 1600000: `6/24/30` vs `6/22/29`;
- cycle-duplicate low, seed 1600100: `5/20/30` vs `5/17/29`;
- fuel-circuit low, seed 1600400: `6/24/30` vs `6/22/25`;
- cycle-duplicate default, seed 1610100: `5/20/48` vs `5/20/47`.

Read-only attribution on seed 1600000 found the first exact day plan in every
agent's 16-column portfolio (`expanded16_mask=111`). Its admissible full-match
upper is joint-best (`upper_rank=1`). The 32-candidate master population with
eight diversity slots discards the exact day-1 outcome, while the unchanged
48-candidate population with twelve diversity slots retains it
(`first_master_cap=48`). The search combination cap, route portfolio, exact
validation and F0 logic were unchanged during this attribution.

The 54-fixture holdout remains sealed. This evidence authorizes only a separately
registered general master-population candidate; it does not authorize tuning the
095 families or opening the holdout early.
