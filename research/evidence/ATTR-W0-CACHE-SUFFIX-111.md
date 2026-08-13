# ATTR-W0-CACHE-SUFFIX-111

Parent: `f77c101`

On consumed ATTR-W0-CACHE-RETENTION-110 development, 36 of 54 next-day
transitions originated from a selected profile containing a certified witness
with more than one future plan. Eighteen carried three plans and eighteen
carried two. In all 36 transitions, the source witness's final certified
official score was strictly greater than the single-day current score retained
by cache repair.

This is a systematic horizon-certificate truncation, not an F0 identity issue.
`record_submitted` stores only the first plan and `repair_cached_contingencies`
reconstructs a certified witness at current score, discarding both the residual
plans and their proven final score.

No candidate is authorized yet. Attribution 112 must exact-replay the stored
suffix from the authoritative next-day roadless state and reproduce the source
final score in every case. A suffix that fails authoritative replay cannot be
used as a lower certificate.
