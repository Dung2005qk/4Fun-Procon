# ATTR-TRAFFIC-TERMINAL-PROJECTION-117

Parent: `5dccb0f`

The unchanged generic width64 pool on consumed seed1700000 day 1 contains both
missing patrol routes. Agent 0's oracle route is global rank 54 and rank 13 of
13 at terminal spot 34; agent 1's route is global rank 14 and rank 2 of 10 at
terminal spot 19. Both routes are independent, dual-timeline columns with exact
own-road footprint `18:0|35:0`, and neither is resource-dominated by a route
with a spot superset, no less remaining fuel and no larger per-road footprint.

Keeping the full per-terminal nondominated frontier is not practical: it contains
51 routes for agent 0 and 46 for agent 1, with up to 11 and 12 routes at one
terminal. That projection is rejected.

A smaller public projection succeeds on both missing routes: for each terminal
spot, select one independent served route lexicographically by total exact road
stays, maximum stays on one road, remaining fuel, current distinct contribution,
current servings and deterministic source order. Both oracle routes are unique
rank 1 under this minimum-traffic terminal selector. The projection is bounded
by the public spot count per agent and does not refer to seed, family, opponent
or expected final score.

Verdict: accepted attribution. This does not yet authorize production source.
The next consumed-witness attribution must build only the selected terminal
sub-portfolio, run the unchanged master, and prove that a useful joint candidate
survives and is visible to the unchanged candidate-specific upper. No known
route may be injected.
