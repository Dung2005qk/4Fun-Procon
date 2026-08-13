# SCORE-TANKER-MOBILE-WINDOWS-140

Date: 2026-08-13  
Production parent: `5dccb0f`  
Frozen manifest: `research/holdouts/SCORE-TANKER-MOBILE-WINDOWS-140.csv`  
SHA256: `859D233298AFAB22A7AD58BB3FA9E4F69C921F72E258054730A5AFE0EDDE53B3`

The existing independent planner models `HubPlan` as a tanker action plan plus
a vector of rendezvous windows, and its route builder can refuel repeatedly.
However, hub generation creates exactly one static window. This candidate adds
a bounded two-window mobile-tanker set over public spot/start cells while
retaining every static hub. Patrol waiting must cover the official consecutive
co-location refuel step before departure.

Acceptance starts on 18 fresh development fixtures across six families and
low/default/high fuel. Holdout remains sealed. No local elapsed time is
performance evidence; any promoted version still requires BTC target-host
hard-cap validation.

Fresh development candidate-vs-parent was `4/14/0`, zero invalid. Gains were
tier-3 `+1,+1,+2,+1` across rendezvous, stock-contention, rare-brand and
delayed-claim at low fuel; all default/high and the remaining low families
tied. The sealed holdout was then opened once and produced `12/42/0`, zero
invalid: all three seeds in the same four low-fuel families gained `+2` or
`+3`, with no loss in any stratum.
