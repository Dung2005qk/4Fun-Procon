# SCORE-LONG-WITNESS-168

Date: 2026-08-19

Parent: `828ea78` plus the unpromoted deadline-policy prerequisite from 166.

Frozen manifest: `research/holdouts/SCORE-LONG-WITNESS-168.csv`

SHA256: `F0EBA90F9A0E00B2BB1C80BB1AC7A13809549AD26C4346DA34A3F7499D678965`

## Hypothesis

For an explicit trusted Long deadline, add two bounded full-horizon lower-witness trajectories after the unchanged W1 repair: current-score greedy and existing FastViability-upper guided. A trajectory may replace a scenario witness only if it reaches the final day, passes the exact simulator and independent validator, and strictly improves the official lexicographic final score. The existing valid upper and `may_submit` comparator remain unchanged.

## Development evidence

- Unit suite passed after the candidate was built.
- The protected 5000-ms equivalence lane used six fresh general fixtures, 25 decisions. Candidate and frozen parent matched every final score, every daily exact score, and all 25 action hashes; invalid and emergency counts were zero. All Long-only counters remained zero.
- Fresh protected general development at 15000 ms used 18 fixtures with the first valid 5000-ms plan protected. Applied candidate versus frozen parent was `0/18/0`; 75 refinements produced zero certified resubmits, zero invalid plans, and zero emergencies.
- The new paths made `123/123` greedy attempts complete and `123/123` guided attempts complete. They improved existing lower witnesses 48 times, while 37 scenarios were already closed by their registered valid upper. None of the improvements strictly dominated the protected plan's certified profile.
- On the first fresh BTC-like default development fixture, the exact score was `6/60/460`, with zero invalid/emergency, ten refinements and zero resubmits. The optional wide paths did not start because the unchanged baseline certification consumed the available certification phase. Remaining BTC-like development, every sealed holdout and BTC target-host validation stayed unopened after the promotion premise had already failed.
- Local elapsed time and cutoff behavior were not used as performance evidence.

## Attribution

The earlier completion ambiguity is closed: the wide cap-16 trajectories can complete and can improve a lower witness. The unresolved obstacle is proof, not trajectory construction. Existing valid uppers still prevent strict certified dominance.

The stronger post-ACK search is complete only over its generated route portfolios. It is not a sound global upper over all legal actions and therefore cannot replace or tighten the scenario valid upper used by `may_submit`. Relaxing the comparator or treating that scoped proof as global would be unsound.

## Verdict

Rejected. The candidate is safe on the tested protected path but score-inert and cannot satisfy the promotion premise. No holdout or BTC target-host gate was opened. All candidate, harness and deadline-policy prerequisite source changes were reverted; canonical production remains `828ea78`.

Reopen only with a new sound general upper/proof representation or a new public-deadline counterexample. Do not reopen by changing cap rank, deadline percentage, seed, map family, fuel lane, opponent, or comparator strictness.
