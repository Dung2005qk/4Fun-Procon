# SCORE-REFUEL-NOWAIT-206

Parent: `690728a`. Verdict: **rejected** at the development gate; the sealed
holdout (4890000/4891000/4892000/4893000) was never opened. Candidate source
reverted (`git checkout -- src/planner.cpp`); reverted tree rebuilds green
(`all tests passed`). The 205 research probe and its CMake target are retained
as accepted research infrastructure.

Frozen manifest: `research/holdouts/SCORE-REFUEL-NOWAIT-206.csv`, SHA256
`22E3E21819E34B1ABFEA46258117D519ABA8984124AA031EA74B5C588C2E73D4`.
Parent binary `historical_tournament_parent_206.exe` SHA256
`9051831EFF2810166CFD7AF54CA97194FF1888CF1E43DE6830137BD3B1DE4E2C`; candidate
binary `historical_tournament_candidate_206.exe` SHA256
`D4018514893D56E6BA76D105092F3499E95BE774BA2F10019CF78993D769BCC6`.

## Candidate

Additive zero-dwell rendezvous variant: alongside every constructed WAIT(1)
escort pair, a second escort pair (new group, duplicated tanker column with
its unchanged one-step hold) whose patrol departs immediately at
`rendezvousStep` with a one-step-larger departure window and the same
`{rendezvous, rendezvousStep+1}` refuel event satisfied mid-move (semantics
proven server-side by 205). WAIT(1) pairs fully retained; all variants passed
the same master/simulator/validator/full-trace gates. Unit suite green; smoke
case tied the parent with zero invalid.

## Development result (60 fresh paired closed-loop matches, sides sequential)

Logs: `SCORE-REFUEL-NOWAIT-206-development-parent.log` SHA256
`07C56D3DA02083990CBEC15986480D1395859E687C12C2A050148ED0D0429F81`,
`SCORE-REFUEL-NOWAIT-206-development-candidate.log` SHA256
`55E522322EB0F02566A7192D2FE450F81AFBDEBFC1A8526864D3BCAFC49170CF`.

- W/T/L = **16/26/18**; zero invalid, zero emergency on both sides.
- Tier 1: no changes. Tier 2: **one loss** (`stratified-easy` seed 4880007,
  low fuel, fuel-tight family: `-2` daily distinct, `-35` servings). Tier 3:
  +16 / -17 with 140 servings gained on wins versus 162 lost on losses.
- Tail downside: -36 (4883005 rare-brand), -28 (4882005 fuel-tight),
  -24 (4883003 overnight), -22 (4883002 high-stock).
- Lanes: default/fixed 1/3/6 (systematic), default/native 3/6/1,
  high/fixed 2/4/4, high/native 3/4/3, low/fixed 4/3/3, low/native 3/6/1.
- Window split: long 9/11/10, short 7/15/8 — the effect is not a
  protected-window artifact.

## Interpretation

The 205 semantics are real, but enlarging the escort candidate pool
redistributes deadline-sampled search (the 194/195/198 phenomenon): the
reclaimed step's value is smaller than the sampling shift it induces, losses
concentrate in default-fuel fixed lanes and the very-hard tier, and one
tier-2 regression disqualifies outright. Additive-column exploitation of
zero-dwell refuel is closed.

## Reopen condition

Reopen only via a mechanism that captures the reclaimed rendezvous step
**without growing the deadline-sampled candidate pool** — e.g. an
incumbent-protected transform in the accepted 187/197 refiner lane that
rewrites a certified incumbent's rendezvous WAIT(1) into an immediate
departure plus trailing wait under strict componentwise dominance (equal road
footprint, ledger dominance, exact simulator and independent validator
agreement), letting the existing WAIT-detour refiner exploit the freed step.
Never widen the escort pool again and never tune on the consumed 488xxxx
development seeds.
