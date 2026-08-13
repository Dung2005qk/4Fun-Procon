# ATTR-TRAFFIC-F0-NO-GAIN-127

Date: 2026-08-13  
Parent: `5dccb0f`  
Candidate: frozen rejected `SCORE-TRAFFIC-F0-UPPER-126` binary  
Fixture: consumed development seed `2420400` only

## Question

`SCORE-TRAFFIC-F0-UPPER-126` completed its upper-aware F0 pass on every
development decision-day and changed only this fixture's plan hash, while all
18 paired scores tied. This attribution asks whether a later planner stage
systematically neutralized a useful retained challenger.

## Paired trace

The candidate and source-frozen parent had identical certified envelopes:

- day 1: selected/current/lower `6/6/6`, upper `6/24/24`, floor-leader;
- day 2: selected/current/lower `6/12/12`, upper `6/24/24`;
- day 3: selected/current/lower `6/18/18`, upper `6/24/24`, floor-leader;
- day 4: selected/current/lower `6/24/24`, upper `6/24/24`, floor-leader.

The only causal difference was day 2 identity/disposition: the candidate made
its changed plan the W1 incumbent, while the parent kept an equivalent-envelope
plan as the upside challenger. Both ended at exact score `6/24/24` with zero
invalid/emergency outcome.

## Verdict

Accepted negative attribution. The changed F0 member was not a suppressed
score improvement: it was an alternative representation of the same certified
envelope and final score. There is no downstream stage bug to repair from this
witness. F0 membership/order/subwindow is closed; reopen only if independent
evidence shows different certified value or exact final score after a retained
upper challenger reaches profile.
