# CEILING-MULTI-PATROL-085 development

Date: 2026-08-12

Parent/champion: `00711ba`.

Frozen manifest SHA256:
`A0EA8A417B4F63B4B5F2E0069A1948EB2C7CB9C827DC6C80A31DC1D6EE188D7B`.

The research-only oracle exhausts every fuel-feasible daily outcome of each of
two active patrols, composes their stock-aware joint outcomes, and performs a
complete four-day DP over canonical joint position/fuel state, lifetime brand
mask and the official accumulated score. Only monotone dominance at identical
physical state is used. The selected full-match witness is replayed through both
the exact simulator and independent validator. The third patrol is an isolated
WAIT control. Production source is unchanged and local elapsed is ignored.

## Development result

Across all 18 registered development fixtures, oracle-vs-champion W/T/L was
`4/14/0`, with zero invalid. Every first difference was tier 2:

| Family | Fuel | Champion | Exact oracle | Gain |
|---|---|---:|---:|---:|
| balanced | low | 5/17/24 | 5/20/26 | +3 |
| duplicate-brand | low | 4/15/26 | 4/16/26 | +1 |
| fuel-allocation | low | 5/19/21 | 5/20/24 | +1 |
| terminal-separation | low | 4/15/24 | 4/16/26 | +1 |

Low-fuel stock-contention and coverage-trap tied. All six default and all six
high-fuel fixtures tied. The largest retained exact frontier was 6399 states;
the representative low-balanced counterexample used 624 states and 105 cached
single-patrol day enumerations. Result hash: `c7324c01ff35e2e8`.

## Gate decision

This is a recurrent low-fuel multi-patrol horizon-value gap, independent of the
one-active-patrol 071 fixtures. The 54-case holdout remains sealed. The only
authorized next step was read-only trace attribution on the four opened
development wins to locate the first plan/state/value divergence.

## Attribution result

Attribution is now closed. In every winning fixture, the exact path deliberately
trades one or more current-day servings for a different terminal position/fuel
state and later gains tier-2 daily distinct. The earliest divergence is day 1
for balanced, duplicate-brand and fuel-allocation, and day 2 for
terminal-separation. The exact candidate's admissible no-tanker viability upper
ranked joint first (`upper_rank=1`) at every attributed day, so the existing
future bound can recognize the useful state once it reaches F0.

The loss occurs earlier. `RouteMaster`'s beam reserves 25% of its slots for
diversity, but `retain_beam_diversity` groups only by tanker terminal cells.
All 085 agents are patrols, so that diversity signature is always empty and the
reserved slots collapse to more current-score order. On the opened paths, the
oracle outcome required master caps from 32 through 96 even though the
production cap is 32. Merely changing the number of diversity slots was not
robust, and widening the fixed cap was rejected as cap tuning.

The exact routes were usually already present in the production portfolio. The
fuel-allocation day-1 route for one patrol first appeared at 32 columns rather
than 16, but inserting that route alone still did not retain the joint outcome;
the same current-score beam loss remained. This isolates one shared structural
gap: the intended diversity lane ignores all-patrol physical terminal state.
The 54-case holdout remains sealed. A successor may change only this general
diversity signature, preserving the quality share and every operation/deadline
cap; it must first improve the unchanged 18-case development split without a
loss before the frozen holdout can open.
