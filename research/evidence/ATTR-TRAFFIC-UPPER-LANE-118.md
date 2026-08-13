# ATTR-TRAFFIC-UPPER-LANE-118

Parent: `5dccb0f`

On consumed seed1700000 day 1, the public minimum-traffic terminal selector from
117 produced a sub-portfolio of only `7|7|1` columns: exact safe wait plus at
most one independent served route per public terminal spot. Without injecting
the known plan, the recorded oracle route mask was `111`.

The unchanged master cap32/diversity8 evaluated the projected portfolio in 98
nodes and retained both the exact oracle joint plan and its equivalent outcome.
The oracle candidate's unchanged candidate-specific FastViability upper was
`6/23/29`, rank 5. The same projected lane naturally exposed a different
candidate with upper `6/24/26`, current score `6/6/8`, and terminal agents
`20@4|36@8|0@12`.

This completes the causal chain: exact full-match counterexample -> missing
generic routes -> bounded public route projection -> joint master candidate ->
existing upper signal. Verdict: accepted attribution. A fresh score candidate
may add this lane only after canonical generation/master remains intact, may
protect at most one upper-ranked lane candidate without evicting canonical F0
candidates, and must remain inside the 5000-ms internal hard cap.
