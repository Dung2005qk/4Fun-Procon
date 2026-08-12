# PERF-MASTER-BUNDLE-092 offline gate

## Frozen artifacts

- Parent commit: `f574d4e`.
- Parent executable SHA256: `B6DF85B0410EF81E77454DBE5BCE9BF0650A0558014185EE6C1124EF1F9E1E1F`.
- Candidate executable SHA256: `247C143CBB6FE09AF4479DD23A472F4A3DFF5161CF5C3EEE4B828C584871CB7D`.
- Manifest SHA256: `47A01C938522D5382D83D10FCA5B3B329C7C555733273C44301C544F8395A5E6`.

## Source proof

`activeBundle` is set from the first selected route column. A nonnegative active
bundle is compatible only with that identical bundle. A negative active bundle
is compatible with every negative bundle, exactly matching the old pairwise
predicate. Exhaustive enumeration over bundle ids `-2,-1,0,1` and every valid
two-column prefix checked 24 next-column cases with zero mismatch. Filtering a
sequence before `stable_sort` preserves the sorted relative order of its retained
subsequence, so every compatible recursive branch keeps the old order when the
operation completes.

The candidate removes no route, bound, cap, search stage or candidate. It only
avoids constructing and sorting columns the unchanged compatibility relation will
reject. The full unit executable passed once.

## Frozen replay solve screen

The only three extant hash-authoritative BTC replays were solved independently by
the frozen parent and candidate on all 30 recorded day states at `5000 ms`.
Every invocation exited successfully with normal, exact-valid output. Aggregate
candidate-vs-parent official-score W/T/L was `0/26/4`; serialized actions matched
on `23/30` states. The four losses were tier-3 only: `m-1986` days 6 and 7,
`m-2029` day 6 and `m-2034` day 9. Final recorded-day totals tied on all three
replays.

Because these are deadline-interrupted local solves, the four differences were
replayed only for attribution in A/B/B/A order. They did not follow the binary:

- `m-1986` day 6: every run tied `6/36/142`, 443 combinations.
- `m-1986` day 7: parent-first scored `6/42/156`; both candidate runs and the
  parent-last run scored `6/42/158`.
- `m-2029` day 6: every run tied `6/36/148`, 983 combinations.
- `m-2034` day 9: both binaries varied from `6/54/299` to `6/54/304`; the two
  candidate runs themselves differed by four servings.

This falsifies a reproducible candidate regression and demonstrates local
wall-clock cutoff noise. It does not prove a performance gain: local elapsed and
visited-combination counts have no promotion authority. Candidate source and
binary are frozen; BTC target-host telemetry is the next gate.
