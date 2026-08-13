# ATTR-PROFILE-DOMINANCE-102

Parent: `f77c101`

This read-only attribution used only consumed CEILING-CYCLE-PATROL-095
cycle-balanced/low seed 1600000, day 1. The exact oracle plan and the
parent-selected plan were both dual-valid. Each received a separate 5000-ms
peer-favorable profile window with the canonical deterministic-no-road
ScenarioManifest, static risk policy, candidate-specific FastViability upper,
FutureWitnessRepairer, exact simulator, independent validator and production
LexicographicRiskComparator. Local elapsed was not used as performance evidence.

The exact plan had current score `6/6/9`, certified lower `6/20/29` and valid
upper `6/24/30`. The parent plan had current score `6/6/12`, certified lower
`6/19/29` and valid upper `6/23/29`. Both single deterministic scenario profiles
were fully certified. Neither strictly certified-dominated the other.

The exact-to-parent proof first failed at threshold `6/23/29`: the exact
certified outcome carried weight 0 at that threshold while the parent valid
upper allowed weight 10000. The reverse proof failed at threshold `6/24/30` in
the same way. Thus candidate-specific valid uppers already distinguish the
terminal states correctly; the exact plan's upper is one daily distinct above
the parent upper. The blocker is the exact plan's W1 lower: it certifies only
`6/20/29`, four daily distinct below the known exact full-match result
`6/24/30`.

102 closes as accepted attribution. Strict dominance must not be weakened and
the master-retention cap must not be retuned. The next authorized read-only gap
is the W1 continuation path from the exact day-1 state: determine whether the
known oracle day-2..4 actions are absent from the generated future portfolios,
evaluated then lost, or available but not selected by repair. Consumed seed
1600000 retains attribution authority only.
