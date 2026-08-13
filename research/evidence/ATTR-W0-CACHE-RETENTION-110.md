# ATTR-W0-CACHE-RETENTION-110

Parent: `f77c101`

Frozen manifest SHA256:
`866DAC7112B8D634687269AE493B04189A019083D4F7B5A307AD60BC43487B01`.

Across 18 fresh development matches and 54 next-day decisions, all 54 unique
certified contingencies were cached, eligible and exact-revalidated; none was
rejected. Fifty-one survived post-F0 retention and 14 were selected.

Three revalidated plans did not survive F0:

- seed 1900000 day 2: cached current/upper `6/11/17,6/22/28`; selected
  `6/12/17,6/24/30`.
- seed 1900200 day 2: cached current/upper `4/8/11,4/16/23`; selected
  `4/8/12,4/16/24`.
- seed 1920400 day 3: cached and selected both current/upper
  `6/18/27,6/24/36`.

Thus no horizon-relevant cached plan was evicted: two were strictly dominated
by the selected candidate and one was bound-equivalent. F0 cache identity does
not justify a candidate. Holdout remains sealed.

Static tracing exposes the next independent gap: `record_submitted` stores only
`witness.futurePlans.front()` in `CachedContingency`; the suffix and final
certified score are absent from the type. `repair_cached_contingencies` then
constructs a new certified witness whose score is only the revalidated current
day. Attribution 111 may quantify this certificate truncation on consumed 110
development without changing production.
