# PERF-SYNC-033 evidence

- Parent: `afcd2da` planner/decision line, rebuilt from the frozen `08771f1`
  worktree whose planner/decision source is identical.
- Candidate: reversible incremental DFS partial-synchronization state. The beam
  rescan and complete leaf validator stayed unchanged.
- Unit suite: passed, including a provider/non-provider sibling rollback case.
- Fresh fixed-role screen, seeds `936000..936005`: all 27 day plan hashes,
  scores, combination counts and role masks were identical; invalid/emergency
  were zero. The remaining frozen split was not opened.
- Final causal replay, `m-1285` day 10 at the unchanged 5000 ms cap:
  parent `6/60/321`, 5119 combinations, 195961 partial checks; candidate
  `6/60/320`, 1129 combinations, 9685 partial checks. Both were exact-valid,
  validator-agreed and deadline-bound, with guidance `6/60/322`.
- Local elapsed time is not performance evidence. The official-score regression
  and failure to create useful search headroom on the preregistered development
  counterexample reject the mechanism before BTC or broader holdout.
- Source and tests were fully restored byte-identical to `afcd2da`; no commit.
