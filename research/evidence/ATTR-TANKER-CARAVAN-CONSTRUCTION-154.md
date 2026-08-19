# ATTR-TANKER-CARAVAN-CONSTRUCTION-154

Date: 2026-08-14
Production parent: `cf7e4b4`

This is read-only attribution on consumed exact anchor `3200100`. It validates
one compact convoy plan derived from public movement/refuel semantics: patrol 0
joins the tanker at the first action boundary, patrol 1 joins at the next
reachable boundary, and both follow the same suffix to one common terminal.
The exact simulator and independent validator must agree. No production source,
candidate threshold, seed split or holdout changes.

## Result

The exact simulator and independent validator agreed. The compact convoy scored
day 1 `3 distinct / 4 servings`; final agents were `26:2|26:2|26:2`. Its plan was
`3.2.2.0.5.5.-4|-2.4.2.0.5.5.-4|5.2.2.0.5.5.-4`.

Verdict: accepted attribution. Join-and-follow is expressive enough. Candidate
153 failed before exact selection, so the next mechanism must address bounded
preseed wiring/scheduling rather than add detach logic or retune geometry.
