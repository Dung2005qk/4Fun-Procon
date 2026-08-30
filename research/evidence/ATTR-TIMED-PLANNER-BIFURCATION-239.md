# ATTR-TIMED-PLANNER-BIFURCATION-239

Date: 2026-08-29

Parent production source: `18ecdd3`

Frozen tournament binary SHA256:
`43ED5815DA0880652819BF589787C11CAFDC92F4D7D313899C3256E37D570389`

Consumed replay SHA256:
`7AE05FC74B426B1E7802F392AB4783D6DAC576ACD69AEA360B84FEDCD68CC301`

This is read-only attribution. It changes no source and has no promotion or
holdout authority.

## Reproduction

The original current-floor mask-1 dump scored `4/28/131` and selected on day 1
a `4/4/18` candidate ending at cells `93,564,258,206`. Its SHA256 is
`5BB7768366BFC12908AF94C8E9DBC7A73D2E5201D39835385F9643BC64D0F1B3`.
A later same-binary run returned `4/28/134` with the same day-1 candidate; dump
SHA256 is
`B5F1A79BF7FF01D95FEF3EF4E7C624073050D23A82B8E8CE86879B0822BF8859`.
The immediately repeated mask-1 run instead returned `4/28/97`; dump SHA256 is
`0AADE75230EB9D9D974696A48A0B59EEBF4FCAF070C400B8E7F00F6A8C05C980`.
An earlier observation also returned `4/28/74` under both public response
budgets 5000 and 10000 ms. The intervening mask-4 control remained
`4/28/102`; its new dump SHA256 is
`B702C181051E5DBFEC414FDF970D115C0C945AB5E6EB45CAA7BBA45175D87E18`.
All runs used the same binary, replay, role mask and
`certified-undominated-current-floor` selection.

## First causal boundary

The `131`, `134` and `97` mask-1 runs share 13 of 16 day-1 candidate stable IDs.
The high and low day-1 selected candidates are present in both pools and tie at
the exact current score `4/4/18`. The difference occurs before certification:

- in the high basin, the eventual high candidate is provisional floor leader,
  improves from `4/4/18` to provisional `4/11/36`, repairs to certified
  `4/20/67`, and is selected;
- in the low basin, the same candidate remains present but receives no useful
  provisional work, stays at `4/4/18`, and is `not-shortlisted`;
- the low-basin floor leader repairs from `4/4/18` to `4/17/60` and selects a
  different terminal state `511,564,206,564`;
- the frozen incumbent `4/4/8` is certified in both basins and cannot protect a
  tie among new `4/4/18` candidates.

Source tracing explains the bifurcation. Candidate intake first sorts by the
official current score, terminal slack, traffic safety and stable ID. After the
16 F0 IDs are selected, `src/decision.cpp` sorts them again by stable ID only.
`provisional_profile` then evaluates them sequentially under one shared absolute
F0 deadline. In the low basin the first stable-ID candidate scores only `3/3/4`
but consumes the available provisional window and rises to `3/5/6`; the high
`4/4/18` candidate is second and receives only its current-score floor. In the
high basin the good `4/4/18` candidate happens to be first and receives the
future witness. Upstream cutoff variation changes which extra diversity IDs are
present and therefore which stable ID is first.

## Verdict

Accepted attribution, no source change. The general defect is deadline work
allocated by a representation key after a complete deterministic quality order
already exists. It is not a tanker-identity certificate and it is not fixed by
more wall-clock time, role beams, current-floor relaxation or resubmission.

This independently satisfies the reopen condition of rejected
`SCORE-TRAFFIC-F0-UPPER-126`: a retained challenger with a different certified
and closed-loop value is systematically denied W1 repair/selection. A source
successor may preserve the original deterministic quality order through
provisional evaluation. It may not add candidates, widen caps, alter the
comparator, route by fixture, or tune a subwindow from this replay.
