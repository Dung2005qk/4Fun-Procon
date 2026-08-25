# SCORE-ROLE-SHORT-HORIZON-221 — accepted-production

Date: 2026-08-25. Parent: d80bf4e. Gap: ATTR-ROLE-SHORT-HORIZON-220.
Production binary SHA256
`4EB926039A50D28F2202BFBE840866D770FD1928119183441C0034377BAA2FE4`.
All artifact hashes: `SCORE-ROLE-SHORT-HORIZON-221-artifacts-sha256.txt` (40 files).

## Final mechanism (v3)

`apply_incomplete_long_horizon_role_fallback` gains a default-false
`includeShortHorizon` parameter (engine setter
`set_short_horizon_role_fallback`, MatchSession forwarder, harness/btc
`--short-role-fallback`). When enabled and `day_count() <= 5`:

- If the incumbent beam front already has 0 or 1 tankers
  (`patrolCount >= agent_count()-1`): **do nothing** — incomplete rollouts
  cannot rank those parents reliably (early guard).
- Otherwise (tanker-heavy front, the class 220 proved broken): rotate the
  best single-tanker assignment to the front — the identical floor the
  accepted long-horizon fallback has used in production since 8caea45.

`day_count() > 5` behavior is byte-identical under both flag values.
Production wiring: `session.set_short_horizon_role_fallback(true)` in the
btc http and sandbox paths; replay-roles/replay-counterfactual honor the CLI
flag for offline A/B.

## Design lineage (all revisions pre-registered in the ledger)

- **v1** floor + all-patrol lex-dominance upgrade → dev found a -26 outlier
  (7300012) that same-flag calibration then proved to be pre-existing
  bistability, but the design question stood.
- **v2** act only on tanker-heavy fronts, keep the upgrade → holdout
  surfaced the fuel-tight all-patrol hazard class; interleaves proved the
  observed case pre-existing, but the upgrade's motivation needed audit.
- **v3** drop the upgrade: its sole motivation (m-4195 mask4 counterfactual
  `8/37/117`) was a one-off day-solve draw — reruns give `8/40/130`,
  `8/40/130`; masks 2/8/1 also 128-130. The upgrade carried a theoretical
  fuel-tight hazard (truncated rollouts overrate all-patrol exactly where
  multi-day fuel exhaustion is invisible) for no reproducible benefit, and
  v3 scores higher on m-4196 (130-class vs 118).

## Evidence

**Frozen-replay gates (v3, log `…-frozen-replay-gates-v3.log`)**: flag-off
replay-check byte-reproduces m-4043/44/45 (6/42/127, 6/60/364, 6/60/144);
flag-on beam[0]: m-4195 PPTP, m-4196 PPPT (220-matrix counterfactual 130
typical — the live losses were 100/94 vs winners 114/113); m-4149/m-4155
keep PPPP, m-3810/m-3907 keep PPP; long-horizon fronts flag-independent.

**Dev (frozen split `…-221-dev.csv` SHA256 `EB14A799…4E96B`, logs
`…-dev-v3-*`)**: easy 1/18/4, medium (7-day control) 0/3/2 with zero mask
changes, general 3/6/2, brand8 4/6/5. Every flagged difference was
interleave-attributed (off/on ×3) to pre-existing bistability — seeds
7300007, 7300008, 7300012, 7340005 all show identical or
both-flags-reachable states; the only flag-on tier-2 difference (7340004)
is a WIN (+3 daily). Zero flag-attributable regression.

**Sealed holdout (fresh v3 split `…-221-v3.csv` SHA256 `71D0D797…4768`,
opened once, logs `…-holdout-v3-*`)**: easy 2/11/2, general 2/5/0,
brand8 2/4/1; both flagged easy seeds interleave-clean (8400006: six runs
all single-tanker with full daily under both flags; 8400014: six identical
mask4/165 runs). No flag-attributable tier or servings regression.

**Production verification (log `…-production-verification.log`)**: binary
`4EB92603…2FE4` — tests all pass; replay-check exact on m-4043/44/45;
production-equivalent replay-roles: m-4195→PPTP, m-4196→PPPT,
m-4149/m-4155→PPPP, m-3810→PPP.

## Instrument findings recorded for successors

1. Production role selection is **bistable** under wall-clock rollout
   truncation: same-flag reruns flip compositions worth ±36 servings
   (7300012: 178↔152; 7300015: 160↔196), including a fuel-tight class where
   an unlucky all-patrol draw loses tier-2 daily (7300017 26/30,
   8340005 27/40, 8400006 26/30). 221 does not touch this class.
2. Synthetic harness maps never produced the live 2-tanker front pathology
   (60+ runs) — live-replay gates are the only activation instrument.
3. Single-tanker agent placement jitters run-to-run (m-4043 PPTP/PPPT across
   reruns) — pre-existing, flag-independent.

## BTC debt

Fresh short-horizon practice matches (12x12 five-day class): expect
composition ≥3 patrols and rank-1-competitive servings (counterfactual 130
vs winners' 114/113 on the consumed losses).

## Addendum (post-acceptance, 2026-08-25 late): the m-4195 mask4 117-tail localized

Repeated counterfactuals of the production-picked composition PPTP (mask4)
on m-4195 give {117 x4, 130 x5} over 9 runs; production's pick is stable
(PPTP 5/5 on m-4195, PPTP 3/3 on m-4196; m-4196 mask4 is 130 in 4/4 runs).
Day-line attribution of every 117 run shows the same signature: the day-3
solve ends with ALL FOUR agents parked on cell 2 at full fuel (day 3 itself
still scores 8/26), and the day-4 solve from that poisoned start can only
reach 5 brands (daily=5/13, -3 tier-2). mask8/mask2 samples (130,130,130)
never exhibit it. This is pre-existing day-solve suffix-poisoning variance
(the profile predicted 8/40/130 after day 3 — the terminal-position choice
is invisible to it), not a 221 selection error: 221 moves the m-4195 class
from a deterministic rank-4 loss (100) to ~5/9 rank-1 at 130 with a ~4/9
tail at 117, and m-4196-class to a stable 130. Successor axis: mid-match
terminal-position suffix safety (relates to the 209 position-then-sweep and
216 scheduling findings). Long-match inertness extended to 7 replays: 
m-4043/44/45/4110/4153/4154/4194 all replay-check exact on the production
binary.
