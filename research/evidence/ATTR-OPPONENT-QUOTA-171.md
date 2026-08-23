# ATTR-OPPONENT-QUOTA-171 — lethinh26/Hexudon-2026 comparison

Date: 2026-08-19

## Verdict

Current canonical UDON-SHIELD `828ea78586fab97c05c1de47b07a374f3b0487cb`
clears this medium-strength open-source opponent. Against peer commit
`49e78066ea8b25a9d3cbe4baf56b390d4419eb75`, the corrected frozen paired
matrix is `47/1/0` for UDON by official lexicographic score:

| Split | W/T/L | First difference |
|---|---:|---|
| All 48 pairs | 47/1/0 | 22 tier-2 wins, 25 tier-3 wins, 1 exact tie |
| Fixed one-tanker-last | 23/1/0 | 14 tier-2 wins, 9 tier-3 wins, 1 tie |
| Native roles | 24/0/0 | 8 tier-2 wins, 16 tier-3 wins |
| Generated 8x8 | 12/0/0 | fixed and native both 6/0/0 |
| BTC-like low fuel | 12/0/0 | fixed and native both 6/0/0 |
| BTC-like default fuel | 11/1/0 | fixed 5/1/0; native 6/0/0 |
| BTC-like high fuel | 12/0/0 | fixed and native both 6/0/0 |

The only tie is BTC-like default/high-stock seed `171214`, fixed lane,
`6/60/525` for both. The closest native win is low-fuel
threshold-corridor seed `171110`, `6/60/362` versus `6/60/356`: UDON wins by
six servings. The compact authoritative rows are in
`research/evidence/ATTR-OPPONENT-QUOTA-171-pairs.csv`.

There is no losing counterexample against current HEAD in this matrix. The tie
and the +6 native margin are useful challenge fixtures, but they do not satisfy
the contract's independent current-HEAD gap requirement and do not reopen a
logic branch by themselves.

## Common evaluator and hardware-independent peer envelope

- Both sources were pinned before comparison. Peer source stayed clean.
- Every emitted action was judged by UDON `ExactStepSimulator` plus
  `IndependentDayValidator`; neither rank nor peer private score selected the
  paired verdict.
- UDON used its canonical logical `5000 ms` research budget. No local elapsed
  number has performance, BTC hard-cap, or competition-readiness authority.
- Peer was deliberately over-granted three independent deterministic work
  tracks using its public `PlannerConfig::nowUs` injection: fake-clock ticks
  `20/10/5`, corresponding to `200000/400000/800000` soft-bound deadline
  observations. Each track owned its planner/history. The oracle-best final
  common score among the three was used, which is stronger than the peer's
  actual single runtime configuration and independent of local CPU load.
- Development replay was field-exact across all 24 peer rows. Holdout contained
  48 UDON rows and 144 peer track rows, with zero invalid plans, zero UDON
  emergency days, and zero peer repairs.
- Peer private daily score disagreed with the common judge on 226 of 410 days
  across the 48 oracle-selected tracks; 42/48 selected tracks had at least one
  mismatch. This confirms that peer private score cannot be comparison
  authority. Root-cause isolation is outside this read-only opponent audit.

## Method corrections retained for provenance

`ATTR-OPPONENT-169` was stopped because its HTTP-style restart count depended
on local wall time and repeated score changed with machine load.

`ATTR-OPPONENT-CEILING-170` removed the cutoff and produced deterministic small
results, but peer exact search did not finish on the first 12-spot large case;
unbounded completion is not a finite full-domain evaluator.

The first 171 stream exposed an adapter violation: UDON fixed rows had
`role_mask=0` because `Options::fixedRoleMask` retained its default, while the
manifest and peer fixed lane required one tanker last. All all-patrol UDON fixed
rows were invalidated. The adapter was corrected to mask `8` for four agents and
`128` for eight agents, and all 24 UDON fixed holdout rows plus four fixed
development rows were rerun. Native and peer rows were unchanged. Only corrected
mask rows appear in the compact paired CSV.

Corrected development is `8/0/0` for UDON. Corrected holdout is `47/1/0`.

## Architecture attribution

The peer's public description is directionally accurate but broader than its
default runtime path:

- Native role selection is a static centrality/fuel-pressure heuristic
  (`planner.hpp:281`), whereas UDON role search wins all 24 native pairs.
- Two-day horizon evaluation exists but is default-off (`planner.hpp:56`).
- Small useful-spot sets choose per-agent exact routing at threshold 14
  (`planner.hpp:392-393`); results are truncated to `maxCandidates`
  (`exact.hpp:141`) before team combination, so this is not a global action-space
  optimality proof.
- The default beam call passes a null deadline (`planner.hpp:411-412`), and the
  beam cap flag is cleared before base combination (`planner.hpp:429`). These are
  peer implementation/certification concerns, not UDON improvement mechanisms.
- LNS is a narrow forced-first-spot patrol regeneration, while mid/end-day
  refuel schedulers are present (`planner.hpp:478-480`). None yields a common
  score counterexample after the fixed-mask correction.

Nothing here is a new general mechanism that UDON lacks and should port. Exact
orienteering, team-level master selection, multi-day value, refuel feasibility,
independent validation, and native role search already exist in stronger or more
integrated form in UDON. The peer's quota non-monotonicity and evaluator mismatch
are warnings, not research ideas. Therefore this comparison closes with no
production candidate and no source commit.

## Evidence integrity

- Manifest SHA256:
  `01A78FBCAC3627242AC51FB29E261965D6AB6C5B84255FD7CDD8BF8AE4EECD3D`
- Compact paired CSV SHA256:
  `398A2EA80351B41A2B7993043D46C106389706CA6C79112999092E1D3E1809A8`
- Final adapter SHA256 before removal:
  `36E6901CC8F5CAD71CC12395C6BD7B0CD0232869739DFD7F1F9D7216CC3480A0`
- Raw independent holdout SHA256:
  `6A62D830DD8C29D871AAFF33A4B49BC53B058D41ABB51F9DF66033BFF20A0A47`
- Corrected fixed-mask rows SHA256:
  `CEE91EC2167B2C86EDDCBD3165D7597BD405FAFA0E41C8676F0C3E6E6E405AAB`

The raw files remain outside the repository under the temporary opponent work
area. The compact paired CSV and this report are the durable canonical evidence.
UDON production paths under `include/`, `src/`, `strategies/`, `old/harness/`,
and tests match HEAD after the comparison.
